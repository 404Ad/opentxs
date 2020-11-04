// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                  // IWYU pragma: associated
#include "1_Internal.hpp"                // IWYU pragma: associated
#include "blockchain/client/Wallet.hpp"  // IWYU pragma: associated

#include <chrono>
#include <optional>
#include <set>
#include <vector>

#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/protobuf/BlockchainTransaction.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposal.pb.h"
#include "util/ScopeGuard.hpp"

#define OT_METHOD                                                              \
    "opentxs::blockchain::client::implementation::Wallet::Proposals::"

namespace opentxs::blockchain::client::implementation
{
Wallet::Proposals::Proposals(
    const api::Core& api,
    const api::client::Blockchain& blockchain,
    const internal::Network& network,
    const internal::WalletDatabase& db,
    const Type chain) noexcept
    : api_(api)
    , blockchain_(blockchain)
    , network_(network)
    , db_(db)
    , chain_(chain)
    , lock_()
    , pending_()
    , confirming_()
{
    for (const auto& serialized : db_.LoadProposals()) {
        const auto id = api_.Factory().Identifier(serialized.id());

        if (serialized.has_finished()) {
            confirming_.emplace(std::move(id), Time{});
        } else {
            pending_[id];
        }
    }
}

auto Wallet::Proposals::Add(const Proposal& tx) const noexcept
    -> std::future<block::pTxid>
{
    const auto id = api_.Factory().Identifier(tx.id());
    Lock lock{lock_};

    if (0 < pending_.count(id)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Proposal already exists").Flush();

        return {};
    }

    if (false == db_.AddProposal(id, tx)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Database error").Flush();

        return {};
    }

    return pending_[id].get_future();
}

auto Wallet::Proposals::build_transaction_bitcoin(
    const Identifier& id,
    Proposal& proposal,
    std::promise<block::pTxid>& promise) const noexcept -> BuildResult
{
    auto builder = BitcoinTransactionBuilder{
        api_, blockchain_, db_, id, chain_, network_.FeeRate()};

    if (false == builder.CreateOutputs(proposal)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to create outputs")
            .Flush();

        return BuildResult::PermanentFailure;
    }

    if (false == builder.AddChange()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to allocate change output")
            .Flush();

        return BuildResult::PermanentFailure;
    }

    while (false == builder.IsFunded()) {
        auto utxo = db_.ReserveUTXO(id);

        if (false == utxo.has_value()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Insufficient funds").Flush();

            return BuildResult::PermanentFailure;
        }

        if (false == builder.AddInput(utxo.value())) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to add input").Flush();

            return BuildResult::PermanentFailure;
        }
    }

    OT_ASSERT(builder.IsFunded());

    builder.FinalizeOutputs();

    if (false == builder.SignInputs()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to sign inputs").Flush();

        return BuildResult::PermanentFailure;
    }

    auto pTransaction = builder.FinalizeTransaction();

    if (false == bool(pTransaction)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to instantiate transaction")
            .Flush();

        return BuildResult::PermanentFailure;
    }

    const auto& transaction = *pTransaction;
    auto proto = transaction.Serialize(blockchain_);

    if (false == proto.has_value()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to serialize transaction")
            .Flush();

        return BuildResult::PermanentFailure;
    }

    *proposal.mutable_finished() = proto.value();

    if (!db_.AddProposal(id, proposal)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Database error (proposal)")
            .Flush();

        return BuildResult::PermanentFailure;
    }

    if (!db_.AddOutgoingTransaction(chain_, id, proposal, transaction)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Database error (transaction)")
            .Flush();

        return BuildResult::PermanentFailure;
    }

    promise.set_value(transaction.ID());
    network_.BroadcastTransaction(transaction);

    return BuildResult::Success;
}

auto Wallet::Proposals::cleanup(const Lock& lock) noexcept -> void
{
    const auto finished = db_.CompletedProposals();

    for (const auto& id : finished) {
        pending_.erase(id);
        confirming_.erase(id);
    }

    db_.ForgetProposals(finished);
}

auto Wallet::Proposals::get_builder() const noexcept -> Builder
{
    switch (chain_) {
        case Type::Bitcoin:
        case Type::Bitcoin_testnet3:
        case Type::BitcoinCash:
        case Type::BitcoinCash_testnet3:
        case Type::Litecoin:
        case Type::Litecoin_testnet4:
        case Type::PKT:
        case Type::PKT_testnet:
        case Type::UnitTest: {

            return [this](
                       const Identifier& id,
                       Proposal& in,
                       std::promise<block::pTxid>& out) -> auto
            {
                return build_transaction_bitcoin(id, in, out);
            };
        }
        case Type::Unknown:
        case Type::Ethereum_frontier:
        case Type::Ethereum_ropsten:
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported chain").Flush();

            return {};
        }
    }
}

auto Wallet::Proposals::is_expired(const Proposal& tx) noexcept -> bool
{
    return Clock::now() > Clock::from_time_t(tx.expires());
}

auto Wallet::Proposals::rebroadcast(const Lock& lock) noexcept -> void
{
    // TODO monitor inv messages from peers and wait until a peer
    // we didn't transmit the transaction to tells us about it
    constexpr auto limit = std::chrono::minutes(1);

    for (auto& [id, time] : confirming_) {
        if ((Clock::now() - time) < limit) { continue; }

        auto proposal = db_.LoadProposal(id);

        if (false == proposal.has_value()) { continue; }

        auto pTx = factory::BitcoinTransaction(
            api_, blockchain_, proposal.value().finished());

        if (false == bool(pTx)) { continue; }

        const auto& tx = *pTx;
        network_.BroadcastTransaction(tx);
    }
}

auto Wallet::Proposals::Run() noexcept -> bool
{
    Lock lock{lock_};
    send(lock);
    rebroadcast(lock);
    cleanup(lock);

    return 0 < pending_.size();
}

auto Wallet::Proposals::send(const Lock& lock) noexcept -> void
{
    for (auto it{pending_.begin()}; it != pending_.end();) {
        auto erase{false};
        auto loop = ScopeGuard{[&]() {
            if (erase) {
                it = pending_.erase(it);
            } else {
                ++it;
            }
        }};
        auto& [id, promise] = *it;
        auto serialized = db_.LoadProposal(id);

        if (false == serialized.has_value()) { continue; }

        auto& data = *serialized;

        if (is_expired(data)) {
            db_.CancelProposal(id);
            db_.DeleteProposal(id);
            erase = true;

            continue;
        }

        if (auto builder = get_builder(); builder) {
            const auto result = builder(id, data, promise);

            if (BuildResult::TemporaryFailure != result) { erase = true; }

            if (BuildResult::Success == result) {
                confirming_.emplace(std::move(id), Clock::now());
            }
        }
    }
}
}  // namespace opentxs::blockchain::client::implementation
