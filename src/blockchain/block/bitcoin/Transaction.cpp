// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                              // IWYU pragma: associated
#include "1_Internal.hpp"                            // IWYU pragma: associated
#include "blockchain/block/bitcoin/Transaction.hpp"  // IWYU pragma: associated

#include <boost/endian/buffers.hpp>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <map>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "blockchain/bitcoin/CompactSize.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/blockchain/bitcoin/Bitcoin.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Contacts.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/blockchain/block/bitcoin/Input.hpp"
#include "opentxs/blockchain/block/bitcoin/Output.hpp"
#include "opentxs/blockchain/block/bitcoin/Outputs.hpp"
#include "opentxs/blockchain/block/bitcoin/Transaction.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/iterator/Bidirectional.hpp"
#include "opentxs/protobuf/BlockchainTransaction.pb.h"
#include "opentxs/protobuf/BlockchainTransactionInput.pb.h"
#include "opentxs/protobuf/BlockchainTransactionOutput.pb.h"
#include "opentxs/protobuf/ContactEnums.pb.h"
#include "util/Container.hpp"

namespace be = boost::endian;

#define OT_METHOD                                                              \
    "opentxs::blockchain::block::bitcoin::implementation::Transaction::"

namespace opentxs::factory
{
using ReturnType = blockchain::block::bitcoin::implementation::Transaction;

auto BitcoinTransaction(
    const api::client::Manager& api,
    const blockchain::Type chain,
    const bool isGeneration,
    const Time& time,
    blockchain::bitcoin::EncodedTransaction&& parsed) noexcept
    -> std::unique_ptr<blockchain::block::bitcoin::internal::Transaction>
{
    try {
        auto inputBytes = std::size_t{};
        auto instantiatedInputs = std::vector<
            std::unique_ptr<blockchain::block::bitcoin::internal::Input>>{};
        {
            auto counter = int{0};
            const auto& inputs = parsed.inputs_;
            instantiatedInputs.reserve(inputs.size());

            for (auto i{0u}; i < inputs.size(); ++i) {
                const auto& input = inputs.at(i);
                const auto& op = input.outpoint_;
                const auto& seq = input.sequence_;
                auto witness = std::vector<Space>{};

                if (0 < parsed.witnesses_.size()) {
                    const auto& encodedWitness = parsed.witnesses_.at(i);

                    for (const auto& [cs, bytes] : encodedWitness.items_) {
                        witness.emplace_back(bytes);
                    }
                }

                instantiatedInputs.emplace_back(
                    factory::BitcoinTransactionInput(
                        api,
                        chain,
                        ReadView{
                            reinterpret_cast<const char*>(&op), sizeof(op)},
                        input.cs_,
                        reader(input.script_),
                        ReadView{
                            reinterpret_cast<const char*>(&seq), sizeof(seq)},
                        isGeneration && (0 == counter),
                        std::move(witness)));
                ++counter;
                inputBytes += input.size();
            }

            const auto cs = blockchain::bitcoin::CompactSize{inputs.size()};
            inputBytes += cs.Size();
            instantiatedInputs.shrink_to_fit();
        }

        auto outputBytes = std::size_t{};
        auto instantiatedOutputs = std::vector<
            std::unique_ptr<blockchain::block::bitcoin::internal::Output>>{};
        {
            instantiatedOutputs.reserve(parsed.outputs_.size());
            auto counter = std::uint32_t{0};

            for (const auto& output : parsed.outputs_) {
                instantiatedOutputs.emplace_back(
                    factory::BitcoinTransactionOutput(
                        api,
                        chain,
                        counter++,
                        output.value_.value(),
                        output.cs_,
                        reader(output.script_)));
                outputBytes += output.size();
            }

            const auto cs =
                blockchain::bitcoin::CompactSize{parsed.outputs_.size()};
            outputBytes += cs.Size();
            instantiatedOutputs.shrink_to_fit();
        }

        auto outputs =
            std::unique_ptr<const blockchain::block::bitcoin::Outputs>{};

        return std::make_unique<ReturnType>(
            api,
            ReturnType::default_version_,
            isGeneration,
            parsed.version_.value(),
            parsed.segwit_flag_.value_or(std::byte{0x0}),
            parsed.lock_time_.value(),
            api.Factory().Data(parsed.txid_),
            api.Factory().Data(parsed.wtxid_),
            time,
            std::string{},
            factory::BitcoinTransactionInputs(
                std::move(instantiatedInputs), inputBytes),
            factory::BitcoinTransactionOutputs(
                std::move(instantiatedOutputs), outputBytes),
            std::vector<blockchain::Type>{chain});
    } catch (const std::exception& e) {
        LogOutput("opentxs::factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}

auto BitcoinTransaction(
    const api::client::Manager& api,
    const proto::BlockchainTransaction& in) noexcept
    -> std::unique_ptr<blockchain::block::bitcoin::internal::Transaction>
{
    auto chains = std::vector<blockchain::Type>{};
    std::transform(
        std::begin(in.chain()),
        std::end(in.chain()),
        std::back_inserter(chains),
        [](const auto& type) -> auto {
            return Translate(static_cast<proto::ContactItemType>(type));
        });

    if (0 == chains.size()) {
        LogOutput("opentxs::factory::")(__FUNCTION__)(": Invalid chains")
            .Flush();

        return {};
    }

    const auto& chain = chains.at(0);

    try {
        auto inputs = std::vector<
            std::unique_ptr<blockchain::block::bitcoin::internal::Input>>{};

        {
            auto map = std::map<
                std::uint32_t,
                std::unique_ptr<blockchain::block::bitcoin::internal::Input>>{};

            for (const auto& input : in.input()) {
                const auto index = input.index();
                map.emplace(
                    index,
                    factory::BitcoinTransactionInput(
                        api,
                        chain,
                        input,
                        (0u == index) && in.is_generation()));
            }

            std::transform(
                std::begin(map), std::end(map), std::back_inserter(inputs), [
                ](auto& in) -> auto { return std::move(in.second); });
        }

        auto outputs = std::vector<
            std::unique_ptr<blockchain::block::bitcoin::internal::Output>>{};

        {
            auto map = std::map<
                std::uint32_t,
                std::unique_ptr<
                    blockchain::block::bitcoin::internal::Output>>{};

            for (const auto& output : in.output()) {
                const auto index = output.index();
                map.emplace(
                    index,
                    factory::BitcoinTransactionOutput(api, chain, output));
            }

            std::transform(
                std::begin(map), std::end(map), std::back_inserter(outputs), [
                ](auto& in) -> auto { return std::move(in.second); });
        }

        return std::make_unique<ReturnType>(
            api,
            in.version(),
            in.is_generation(),
            static_cast<std::int32_t>(in.txversion()),
            std::byte{static_cast<std::uint8_t>(in.segwit_flag())},
            in.locktime(),
            api.Factory().Data(in.txid(), StringStyle::Raw),
            api.Factory().Data(in.wtxid(), StringStyle::Raw),
            Clock::from_time_t(in.time()),
            in.memo(),
            factory::BitcoinTransactionInputs(std::move(inputs)),
            factory::BitcoinTransactionOutputs(std::move(outputs)),
            std::move(chains));
    } catch (const std::exception& e) {
        LogOutput("opentxs::factory::")(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    }
}
}  // namespace opentxs::factory

namespace opentxs::blockchain::block::bitcoin::implementation
{
const VersionNumber Transaction::default_version_{1};

Transaction::Transaction(
    const api::client::Manager& api,
    const VersionNumber serializeVersion,
    const bool isGeneration,
    const std::int32_t version,
    const std::byte segwit,
    const std::uint32_t lockTime,
    const pTxid&& txid,
    const pTxid&& wtxid,
    const Time& time,
    const std::string& memo,
    std::unique_ptr<internal::Inputs> inputs,
    std::unique_ptr<internal::Outputs> outputs,
    std::vector<blockchain::Type>&& chains) noexcept(false)
    : api_(api)
    , serialize_version_(serializeVersion)
    , is_generation_(isGeneration)
    , version_(version)
    , segwit_flag_(segwit)
    , lock_time_(lockTime)
    , txid_(std::move(txid))
    , wtxid_(std::move(wtxid))
    , time_(time)
    , inputs_(std::move(inputs))
    , outputs_(std::move(outputs))
    , normalized_id_()
    , size_()
    , normalized_size_()
    , memo_(memo)
    , chains_(std::move(chains))
{
    if (0 == chains_.size()) { throw std::runtime_error("missing chains"); }

    if (false == bool(inputs_)) { throw std::runtime_error("invalid inputs"); }

    if (false == bool(outputs_)) {
        throw std::runtime_error("invalid outputs");
    }

    dedup(chains_);
}

Transaction::Transaction(const Transaction& rhs) noexcept
    : api_(rhs.api_)
    , serialize_version_(rhs.serialize_version_)
    , is_generation_(rhs.is_generation_)
    , version_(rhs.version_)
    , segwit_flag_(rhs.segwit_flag_)
    , lock_time_(rhs.lock_time_)
    , txid_(rhs.txid_)
    , wtxid_(rhs.wtxid_)
    , time_(rhs.time_)
    , inputs_(rhs.inputs_->clone())
    , outputs_(rhs.outputs_->clone())
    , normalized_id_(rhs.normalized_id_)
    , size_(rhs.size_)
    , normalized_size_(rhs.normalized_size_)
    , memo_(rhs.memo_)
    , chains_(rhs.chains_)
{
}

auto Transaction::AssociatedLocalNyms() const noexcept -> std::vector<OTNymID>
{
    auto output = std::vector<OTNymID>{};
    inputs_->AssociatedLocalNyms(output);
    outputs_->AssociatedLocalNyms(output);
    dedup(output);

    return output;
}

auto Transaction::AssociatedRemoteContacts(
    const identifier::Nym& nym) const noexcept -> std::vector<OTIdentifier>
{
    auto output = std::vector<OTIdentifier>{};
    inputs_->AssociatedRemoteContacts(output);
    outputs_->AssociatedRemoteContacts(output);
    dedup(output);
    const auto mask = api_.Contacts().ContactID(nym);
    output.erase(std::remove(output.begin(), output.end(), mask), output.end());

    return output;
}

auto Transaction::calculate_size(const bool normalize) const noexcept
    -> std::size_t
{
    auto& output = normalize ? normalized_size_ : size_;
    const bool isSegwit =
        (false == normalize) && (std::byte{0x00} != segwit_flag_);

    if (false == output.has_value()) {
        output = sizeof(version_) + inputs_->CalculateSize(normalize) +
                 outputs_->CalculateSize() +
                 (isSegwit ? calculate_witness_size() : std::size_t{0}) +
                 sizeof(lock_time_);
    }

    return output.value();
}

auto Transaction::calculate_witness_size(const Space& in) noexcept
    -> std::size_t
{
    return blockchain::bitcoin::CompactSize{in.size()}.Total();
}

auto Transaction::calculate_witness_size(const std::vector<Space>& in) noexcept
    -> std::size_t
{
    const auto cs = blockchain::bitcoin::CompactSize{in.size()};

    return std::accumulate(
        std::begin(in),
        std::end(in),
        cs.Size(),
        [](const auto& previous, const auto& input) -> std::size_t {
            return previous + calculate_witness_size(input);
        });
}

auto Transaction::calculate_witness_size() const noexcept -> std::size_t
{
    return std::accumulate(
        std::begin(*inputs_),
        std::end(*inputs_),
        std::size_t{2},  // NOTE: marker byte and segwit flag byte
        [](const auto& previous, const auto& input) -> std::size_t {
            return previous + calculate_witness_size(input.Witness());
        });
}

auto Transaction::IDNormalized() const noexcept -> const Identifier&
{
    if (false == normalized_id_.has_value()) {
        auto preimage = Space{};
        const auto serialized = serialize(writer(preimage), true);

        OT_ASSERT(serialized);

        normalized_id_ = api_.Factory().Identifier();
        auto& output = normalized_id_.value().get();
        output.CalculateDigest(reader(preimage), ID::sha256);
    }

    return normalized_id_.value();
}

auto Transaction::ExtractElements(const filter::Type style) const noexcept
    -> std::vector<Space>
{
    auto output = inputs_->ExtractElements(style);
    LogTrace(OT_METHOD)(__FUNCTION__)(": extracted ")(output.size())(
        " input elements")
        .Flush();
    auto temp = outputs_->ExtractElements(style);
    LogTrace(OT_METHOD)(__FUNCTION__)(": extracted ")(temp.size())(
        " output elements")
        .Flush();
    output.insert(
        output.end(),
        std::make_move_iterator(temp.begin()),
        std::make_move_iterator(temp.end()));

    if (filter::Type::Extended_opentxs == style) {
        const auto* data = static_cast<const std::byte*>(txid_->data());
        output.emplace_back(data, data + txid_->size());
    }

    LogTrace(OT_METHOD)(__FUNCTION__)(": extracted ")(output.size())(
        " total elements")
        .Flush();

    return output;
}

auto Transaction::FindMatches(
    const FilterType style,
    const Patterns& txos,
    const Patterns& elements) const noexcept -> Matches
{
    auto output = inputs_->FindMatches(txid_->Bytes(), style, txos, elements);
    auto temp = outputs_->FindMatches(txid_->Bytes(), style, elements);
    output.insert(
        output.end(),
        std::make_move_iterator(temp.begin()),
        std::make_move_iterator(temp.end()));

    return output;
}

auto Transaction::GetPatterns() const noexcept -> std::vector<PatternID>
{
    auto output = inputs_->GetPatterns();
    const auto oPatterns = outputs_->GetPatterns();
    output.reserve(output.size() + oPatterns.size());
    output.insert(output.end(), oPatterns.begin(), oPatterns.end());
    dedup(output);

    return output;
}

auto Transaction::Memo() const noexcept -> std::string
{
    if (false == memo_.empty()) { return memo_; }

    for (const auto& output : *outputs_) {
        auto note = output.Note();

        if (false == note.empty()) { return note; }
    }

    return {};
}

auto Transaction::MergeMetadata(
    const blockchain::Type chain,
    const SerializeType& rhs) noexcept -> void
{
    if (txid_->asHex() != rhs.txid()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong transaction").Flush();

        return;
    }

    if (static_cast<std::size_t>(rhs.input().size()) != inputs_->size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong number of inputs").Flush();

        return;
    }

    if (static_cast<std::size_t>(rhs.output().size()) != outputs_->size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Wrong number of outputs").Flush();

        return;
    }

    memo_ = rhs.memo();
    chains_.emplace_back(chain);
    dedup(chains_);

    for (const auto& input : rhs.input()) {
        try {
            inputs_->MergeMetadata(input);
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid input").Flush();

            return;
        }
    }

    for (const auto& output : rhs.output()) {
        try {
            outputs_->MergeMetadata(output);
        } catch (...) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output").Flush();

            return;
        }
    }
}

auto Transaction::NetBalanceChange(const identifier::Nym& nym) const noexcept
    -> opentxs::Amount
{
    return inputs_->NetBalanceChange(nym) + outputs_->NetBalanceChange(nym);
}

auto Transaction::serialize(
    const AllocateOutput destination,
    const bool normalize) const noexcept -> std::optional<std::size_t>
{
    if (!destination) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return std::nullopt;
    }

    const auto size = calculate_size(normalize);
    auto output = destination(size);

    if (false == output.valid(size)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to allocate output bytes")
            .Flush();

        return std::nullopt;
    }

    const auto version = be::little_int32_buf_t{version_};
    const auto lockTime = be::little_uint32_buf_t{lock_time_};
    auto remaining{output.size()};
    auto it = static_cast<std::byte*>(output.data());

    if (remaining < sizeof(version)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to serialize version. Need at least ")(sizeof(version))(
            " bytes but only have ")(remaining)
            .Flush();

        return std::nullopt;
    }

    std::memcpy(static_cast<void*>(it), &version, sizeof(version));
    std::advance(it, sizeof(version));
    remaining -= sizeof(version);
    const bool isSegwit =
        (false == normalize) && (std::byte{0x00} != segwit_flag_);

    if (isSegwit) {
        if (remaining < sizeof(std::byte)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to serialize marker byte. Need at least ")(
                sizeof(std::byte))(" bytes but only have ")(remaining)
                .Flush();

            return std::nullopt;
        }

        *it = std::byte{0x00};
        std::advance(it, sizeof(std::byte));
        remaining -= sizeof(std::byte);

        if (remaining < sizeof(segwit_flag_)) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to serialize segwit flag. Need at least ")(
                sizeof(segwit_flag_))(" bytes but only have ")(remaining)
                .Flush();

            return std::nullopt;
        }

        *it = segwit_flag_;
        std::advance(it, sizeof(segwit_flag_));
        remaining -= sizeof(segwit_flag_);
    }

    const auto inputs =
        normalize ? inputs_->SerializeNormalized(preallocated(remaining, it))
                  : inputs_->Serialize(preallocated(remaining, it));

    if (inputs.has_value()) {
        std::advance(it, inputs.value());
        remaining -= inputs.value();
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize inputs")
            .Flush();

        return std::nullopt;
    }

    const auto outputs = outputs_->Serialize(preallocated(remaining, it));

    if (outputs.has_value()) {
        std::advance(it, outputs.value());
        remaining -= outputs.value();
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize outputs")
            .Flush();

        return std::nullopt;
    }

    if (isSegwit) {
        for (const auto& input : *inputs_) {
            const auto& witness = input.Witness();
            const auto pushCount =
                blockchain::bitcoin::CompactSize{witness.size()};

            if (false == pushCount.Encode(preallocated(remaining, it))) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to serialize push count")
                    .Flush();

                return std::nullopt;
            }

            std::advance(it, pushCount.Size());
            remaining -= pushCount.Size();

            for (const auto& push : witness) {
                const auto pushSize =
                    blockchain::bitcoin::CompactSize{push.size()};

                if (false == pushSize.Encode(preallocated(remaining, it))) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Failed to serialize push size")
                        .Flush();

                    return std::nullopt;
                }

                std::advance(it, pushSize.Size());
                remaining -= pushSize.Size();

                if (remaining < push.size()) {
                    LogOutput(OT_METHOD)(__FUNCTION__)(
                        ": Failed to serialize witness push. Need at least ")(
                        push.size())(" bytes but only have ")(remaining)
                        .Flush();

                    return std::nullopt;
                }

                std::memcpy(it, push.data(), push.size());
                std::advance(it, push.size());
                remaining -= push.size();
            }
        }
    }

    if (remaining != sizeof(lockTime)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to serialize lock time. Need exactly ")(sizeof(lockTime))(
            " bytes but have ")(remaining)
            .Flush();

        return std::nullopt;
    }

    std::memcpy(static_cast<void*>(it), &lockTime, sizeof(lockTime));
    std::advance(it, sizeof(lockTime));
    remaining -= sizeof(lockTime);

    return size;
}

auto Transaction::Serialize(const AllocateOutput destination) const noexcept
    -> std::optional<std::size_t>
{
    return serialize(destination, false);
}

auto Transaction::Serialize() const noexcept -> std::optional<SerializeType>
{
    auto output = SerializeType{};
    output.set_version(std::max(default_version_, serialize_version_));
    std::for_each(
        std::begin(chains_), std::end(chains_), [&](const auto& chain) {
            output.add_chain(Translate(chain));
        });
    output.set_txid(txid_->str());
    output.set_txversion(version_);
    output.set_locktime(lock_time_);

    if (false == Serialize(writer(*output.mutable_serialized()))) { return {}; }

    if (false == inputs_->Serialize(output)) { return {}; }

    if (false == outputs_->Serialize(output)) { return {}; }

    // TODO optional uint32 confirmations = 9;
    // TODO optional string blockhash = 10;
    // TODO optional uint32 blockindex = 11;
    // TODO optional uint64 fee = 12;
    output.set_time(Clock::to_time_t(time_));
    // TODO repeated string conflicts = 14;
    output.set_memo(memo_);
    output.set_segwit_flag(std::to_integer<std::uint32_t>(segwit_flag_));
    output.set_wtxid(wtxid_->str());
    output.set_is_generation(is_generation_);

    return std::move(output);
}
}  // namespace opentxs::blockchain::block::bitcoin::implementation
