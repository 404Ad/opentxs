// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACCOUNT_ACTIVITY_IMPLEMENTATION_HPP
#define OPENTXS_UI_ACCOUNT_ACTIVITY_IMPLEMENTATION_HPP

#include "Internal.hpp"

namespace opentxs::ui::implementation
{
template <>
struct make_blank<AccountActivityRowID> {
    static AccountActivityRowID value()
    {
        return {Identifier::Factory(), proto::PAYMENTEVENTTYPE_ERROR};
    }
};

using AccountActivityList = List<
    AccountActivityExternalInterface,
    AccountActivityInternalInterface,
    AccountActivityRowID,
    AccountActivityRowInterface,
    AccountActivityRowInternal,
    AccountActivityRowBlank,
    AccountActivitySortKey>;

/** Show the list of Workflows applicable to this account

    Each row is a BalanceItem which is associated with a Workflow state.

    Some Workflows will only have one entry in the AccountActivity based on
    their type, but others may have multiple entries corresponding to different
    states.
 */
class AccountActivity final : public AccountActivityList
{
public:
    Amount Balance() const override { return balance_.load(); }
    std::string DisplayBalance() const override;

    ~AccountActivity() = default;

private:
    friend opentxs::Factory;

    using EventRow =
        std::pair<AccountActivitySortKey, const proto::PaymentEvent*>;
    using RowKey = std::pair<proto::PaymentEventType, EventRow>;

    static const ListenerDefinitions listeners_;

    const api::client::Sync& sync_;
    const api::Wallet& wallet_;
    const api::client::Workflow& workflow_;
    const api::storage::Storage& storage_;
    const api::Legacy& legacy_;
    mutable std::atomic<Amount> balance_{0};
    const OTIdentifier account_id_;
    std::shared_ptr<const UnitDefinition> contract_{nullptr};

    static EventRow extract_event(
        const proto::PaymentEventType event,
        const proto::PaymentWorkflow& workflow);
    static std::vector<RowKey> extract_rows(
        const proto::PaymentWorkflow& workflow);

    void construct_row(
        const AccountActivityRowID& id,
        const AccountActivitySortKey& index,
        const CustomData& custom) const override;

    void process_balance(const network::zeromq::Message& message);
    void process_workflow(
        const Identifier& workflowID,
        std::set<AccountActivityRowID>& active);
    void process_workflow(const network::zeromq::Message& message);
    void startup();

    AccountActivity(
        const network::zeromq::Context& zmq,
        const network::zeromq::PublishSocket& publisher,
        const api::client::Sync& sync,
        const api::Wallet& wallet,
        const api::client::Workflow& workflow,
        const api::client::Contacts& contact,
        const api::storage::Storage& storage,
        const api::Legacy& legacy,
        const Identifier& nymID,
        const Identifier& accountID);
    AccountActivity() = delete;
    AccountActivity(const AccountActivity&) = delete;
    AccountActivity(AccountActivity&&) = delete;
    AccountActivity& operator=(const AccountActivity&) = delete;
    AccountActivity& operator=(AccountActivity&&) = delete;
};
}  // namespace opentxs::ui::implementation
#endif  // OPENTXS_UI_ACCOUNT_ACTIVITY_IMPLEMENTATION_HPP
