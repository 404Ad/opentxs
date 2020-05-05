// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CLIENT_BLOCKCHAIN_BALANCENODE_HPP
#define OPENTXS_API_CLIENT_BLOCKCHAIN_BALANCENODE_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include "opentxs/Proto.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/identifier/Nym.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace blockchain
{
class BalanceNode
{
public:
    struct Element {
        OPENTXS_EXPORT virtual std::string Address(
            const AddressStyle format) const noexcept = 0;
        OPENTXS_EXPORT virtual OTIdentifier Contact() const noexcept = 0;
        OPENTXS_EXPORT virtual ECKey Key() const noexcept = 0;
        OPENTXS_EXPORT virtual std::string Label() const noexcept = 0;
        OPENTXS_EXPORT virtual OTData PubkeyHash() const noexcept = 0;

        virtual ~Element() = default;

    protected:
        Element() noexcept = default;
    };

    /// Throws std::out_of_range for invalid index
    OPENTXS_EXPORT virtual const Element& BalanceElement(
        const Subchain type,
        const Bip32Index index) const noexcept(false) = 0;
    OPENTXS_EXPORT virtual const Identifier& ID() const noexcept = 0;
    OPENTXS_EXPORT virtual const BalanceTree& Parent() const noexcept = 0;
    OPENTXS_EXPORT virtual BalanceNodeType Type() const noexcept = 0;

    OPENTXS_EXPORT virtual ~BalanceNode() = default;

protected:
    BalanceNode() noexcept = default;

private:
    BalanceNode(const BalanceNode&) = delete;
    BalanceNode(BalanceNode&&) = delete;
    BalanceNode& operator=(const BalanceNode&) = delete;
    BalanceNode& operator=(BalanceNode&&) = delete;
};
}  // namespace blockchain
}  // namespace client
}  // namespace api
}  // namespace opentxs
#endif  // OPENTXS_API_CLIENT_BLOCKCHAIN_BALANCENODE_HPP
