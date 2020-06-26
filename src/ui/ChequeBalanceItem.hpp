// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/ui/ChequeBalanceItem.cpp"

#pragma once

#include <memory>
#include <string>

#include "1_Internal.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Cheque.hpp"
#include "ui/BalanceItem.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
namespace internal
{
struct Manager;
}  // namespace internal
}  // namespace client
}  // namespace api

namespace identifier
{
class Nym;
}  // namespace identifier

namespace network
{
namespace zeromq
{
namespace socket
{
class Publish;
}  // namespace socket
}  // namespace zeromq
}  // namespace network

namespace proto
{
class PaymentEvent;
class PaymentWorkflow;
}  // namespace proto

class Identifier;
}  // namespace opentxs

namespace opentxs::ui::implementation
{
class ChequeBalanceItem final : public BalanceItem
{
public:
    auto Amount() const noexcept -> opentxs::Amount final
    {
        return effective_amount();
    }
    auto Memo() const noexcept -> std::string final;
    auto UUID() const noexcept -> std::string final;
    auto Workflow() const noexcept -> std::string final { return workflow_; }

    void reindex(
        const implementation::AccountActivitySortKey& key,
        implementation::CustomData& custom) noexcept final;

    ChequeBalanceItem(
        const AccountActivityInternalInterface& parent,
        const api::client::internal::Manager& api,
        const network::zeromq::socket::Publish& publisher,
        const AccountActivityRowID& rowID,
        const AccountActivitySortKey& sortKey,
        CustomData& custom,
        const identifier::Nym& nymID,
        const Identifier& accountID) noexcept;
    ~ChequeBalanceItem() = default;

private:
    std::unique_ptr<const opentxs::Cheque> cheque_;

    auto effective_amount() const noexcept -> opentxs::Amount final;
    auto get_contract() const noexcept -> bool final;

    void startup(
        const proto::PaymentWorkflow workflow,
        const proto::PaymentEvent event) noexcept;

    ChequeBalanceItem() = delete;
    ChequeBalanceItem(const ChequeBalanceItem&) = delete;
    ChequeBalanceItem(ChequeBalanceItem&&) = delete;
    auto operator=(const ChequeBalanceItem&) -> ChequeBalanceItem& = delete;
    auto operator=(ChequeBalanceItem &&) -> ChequeBalanceItem& = delete;
};
}  // namespace opentxs::ui::implementation
