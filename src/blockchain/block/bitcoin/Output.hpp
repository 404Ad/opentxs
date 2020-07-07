// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/container/flat_set.hpp>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/block/bitcoin/Output.hpp"
#include "opentxs/blockchain/block/bitcoin/Script.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
class Manager;
}  // namespace client
}  // namespace api
}  // namespace opentxs

namespace opentxs::blockchain::block::bitcoin::implementation
{
class Output final : public internal::Output
{
public:
    auto AssociatedLocalNyms(std::vector<OTNymID>& output) const noexcept
        -> void final;
    auto AssociatedRemoteContacts(
        std::vector<OTIdentifier>& output) const noexcept -> void final;
    auto CalculateSize() const noexcept -> std::size_t final;
    auto clone() const noexcept -> std::unique_ptr<internal::Output> final
    {
        return std::make_unique<Output>(*this);
    }
    auto ExtractElements(const filter::Type style) const noexcept
        -> std::vector<Space> final;
    auto FindMatches(
        const ReadView txid,
        const FilterType type,
        const Patterns& patterns) const noexcept -> Matches final;
    auto GetPatterns() const noexcept -> std::vector<PatternID> final;
    auto Keys() const noexcept -> std::vector<KeyID> final;
    auto NetBalanceChange(const identifier::Nym& nym) const noexcept
        -> opentxs::Amount final;
    auto Note() const noexcept -> std::string final;
    auto Payee() const noexcept -> ContactID final;
    auto Payer() const noexcept -> ContactID final;
    auto PrintScript() const noexcept -> std::string final
    {
        return script_->str();
    }
    auto Serialize(const AllocateOutput destination) const noexcept
        -> std::optional<std::size_t>;
    auto Serialize(SerializeType& destination) const noexcept -> bool;
    auto SigningSubscript() const noexcept
        -> std::unique_ptr<internal::Script> final
    {
        return script_->SigningSubscript(chain_);
    }
    auto Script() const noexcept -> const internal::Script& final
    {
        return *script_;
    }
    auto Value() const noexcept -> std::int64_t final { return value_; }

    auto ForTestingOnlyAddKey(const KeyID& key) noexcept -> void final
    {
        keys_.insert(key);
    }
    auto MergeMetadata(const SerializeType& rhs) noexcept -> void final;
    auto SetIndex(const std::uint32_t index) noexcept -> void final
    {
        const_cast<std::uint32_t&>(index_) = index;
    }
    auto SetValue(const std::int64_t value) noexcept -> void final
    {
        const_cast<std::int64_t&>(value_) = value;
    }

    Output(
        const api::client::Manager& api,
        const blockchain::Type chain,
        const std::uint32_t index,
        const std::int64_t value,
        const std::size_t size,
        const ReadView script,
        const VersionNumber version = default_version_) noexcept(false);
    Output(
        const api::client::Manager& api,
        const blockchain::Type chain,
        const std::uint32_t index,
        const std::int64_t value,
        std::unique_ptr<const internal::Script> script,
        boost::container::flat_set<KeyID>&& keys,
        const VersionNumber version = default_version_) noexcept(false);
    Output(
        const api::client::Manager& api,
        const blockchain::Type chain,
        const VersionNumber version,
        const std::uint32_t index,
        const std::int64_t value,
        std::unique_ptr<const internal::Script> script,
        std::optional<std::size_t> size,
        boost::container::flat_set<KeyID>&& keys,
        boost::container::flat_set<PatternID>&& pubkeyHashes,
        std::optional<PatternID>&& scriptHash,
        const bool indexed) noexcept(false);
    Output(const Output&) noexcept;

    ~Output() final = default;

private:
    static const VersionNumber default_version_;
    static const VersionNumber key_version_;

    const api::client::Manager& api_;
    const blockchain::Type chain_;
    const VersionNumber serialize_version_;
    const std::uint32_t index_;
    const std::int64_t value_;
    const std::unique_ptr<const internal::Script> script_;
    const boost::container::flat_set<PatternID> pubkey_hashes_;
    const std::optional<PatternID> script_hash_;
    mutable std::optional<std::size_t> size_;
    mutable OTIdentifier payee_;
    mutable OTIdentifier payer_;
    mutable boost::container::flat_set<KeyID> keys_;

    auto set_payee() const noexcept -> void;
    auto set_payer() const noexcept -> void;

    auto index_elements() noexcept -> void;

    Output() = delete;
    Output(Output&&) = delete;
    auto operator=(const Output&) -> Output& = delete;
    auto operator=(Output &&) -> Output& = delete;
};
}  // namespace opentxs::blockchain::block::bitcoin::implementation
