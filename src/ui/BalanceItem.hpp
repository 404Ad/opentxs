// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "1_Internal.hpp"
#include "internal/ui/UI.hpp"
#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/ui/BalanceItem.hpp"
#include "ui/Row.hpp"

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

namespace ui
{
class BalanceItem;
}  // namespace ui
}  // namespace opentxs

namespace opentxs::ui::implementation
{
using BalanceItemRow =
    Row<AccountActivityRowInternal,
        AccountActivityInternalInterface,
        AccountActivityRowID>;

class BalanceItem : public BalanceItemRow
{
public:
    static auto recover_event(CustomData& custom) noexcept
        -> const proto::PaymentEvent&;
    static auto recover_workflow(CustomData& custom) noexcept
        -> const proto::PaymentWorkflow&;

    auto Contacts() const noexcept -> std::vector<std::string> override
    {
        return contacts_;
    }
    auto DisplayAmount() const noexcept -> std::string override;
    auto Text() const noexcept -> std::string override;
    auto Timestamp() const noexcept -> Time final;
    auto Type() const noexcept -> StorageBox override { return type_; }

    void reindex(
        const implementation::AccountActivitySortKey& key,
        implementation::CustomData& custom) noexcept override;

#if OT_QT
    QVariant qt_data(const int column, const int role) const noexcept final;
#endif

    ~BalanceItem() override;

protected:
    const OTNymID nym_id_;
    const std::string workflow_;
    const StorageBox type_;
    std::string text_;
    Time time_;
    mutable OTUnitDefinition contract_;
    std::unique_ptr<std::thread> startup_;

    static auto extract_type(const proto::PaymentWorkflow& workflow) noexcept
        -> StorageBox;

    auto get_contact_name(const identifier::Nym& nymID) const noexcept
        -> std::string;

    BalanceItem(
        const AccountActivityInternalInterface& parent,
        const api::client::internal::Manager& api,
        const AccountActivityRowID& rowID,
        const AccountActivitySortKey& sortKey,
        CustomData& custom,
        const identifier::Nym& nymID,
        const Identifier& accountID,
        const std::string& text = {}) noexcept;

private:
    const OTIdentifier account_id_;
    const std::vector<std::string> contacts_;

    static auto extract_contacts(
        const api::client::internal::Manager& api,
        const proto::PaymentWorkflow& workflow) noexcept
        -> std::vector<std::string>;

    virtual auto effective_amount() const noexcept -> opentxs::Amount = 0;
    virtual auto get_contract() const noexcept -> bool = 0;

    BalanceItem(const BalanceItem&) = delete;
    BalanceItem(BalanceItem&&) = delete;
    auto operator=(const BalanceItem&) -> BalanceItem& = delete;
    auto operator=(BalanceItem &&) -> BalanceItem& = delete;
};
}  // namespace opentxs::ui::implementation

template class opentxs::SharedPimpl<opentxs::ui::BalanceItem>;
