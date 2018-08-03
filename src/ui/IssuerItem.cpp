// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/api/client/Issuer.hpp"
#include "opentxs/api/client/Wallet.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/api/Legacy.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Lockable.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/SubscribeSocket.hpp"
#include "opentxs/ui/IssuerItem.hpp"

#include "AccountSummaryItemBlank.hpp"
#include "InternalUI.hpp"
#include "List.hpp"
#include "Row.hpp"

#include <atomic>

#include "IssuerItem.hpp"

template class opentxs::SharedPimpl<opentxs::ui::IssuerItem>;

#define OT_METHOD "opentxs::ui::implementation::IssuerItem::"

namespace opentxs
{
ui::implementation::AccountSummaryRowInternal* Factory::IssuerItem(
    const ui::implementation::AccountSummaryInternalInterface& parent,
    const network::zeromq::Context& zmq,
    const network::zeromq::PublishSocket& publisher,
    const api::client::Contacts& contact,
    const ui::implementation::AccountSummaryRowID& rowID,
    const ui::implementation::AccountSummarySortKey& sortKey,
    const ui::implementation::CustomData& custom,
    const api::client::Wallet& wallet,
    const api::storage::Storage& storage,
    const api::Legacy& legacy,
    const proto::ContactItemType currency)
{
    return new ui::implementation::IssuerItem(
        parent,
        zmq,
        publisher,
        contact,
        rowID,
        sortKey,
        custom,
        wallet,
        storage,
        legacy,
        currency);
}
}  // namespace opentxs

namespace opentxs::ui::implementation
{
const Widget::ListenerDefinitions IssuerItem::listeners_{
    {network::zeromq::Socket::AccountUpdateEndpoint,
     new MessageProcessor<IssuerItem>(&IssuerItem::process_account)},
};

IssuerItem::IssuerItem(
    const AccountSummaryInternalInterface& parent,
    const network::zeromq::Context& zmq,
    const network::zeromq::PublishSocket& publisher,
    const api::client::Contacts& contact,
    const AccountSummaryRowID& rowID,
    const AccountSummarySortKey& sortKey,
    [[maybe_unused]] const CustomData& custom,
    const api::client::Wallet& wallet,
    const api::storage::Storage& storage,
    const api::Legacy& legacy,
    const proto::ContactItemType currency)
    : IssuerItemList(parent.WidgetID(), parent.NymID(), zmq, publisher, contact)
    , IssuerItemRow(parent, Identifier::Factory(rowID), true)
    , wallet_{wallet}
    , storage_{storage}
    , legacy_{legacy}
    , key_{sortKey}
    , name_{std::get<1>(key_)}
    , connection_{std::get<0>(key_)}
    , issuer_{wallet.Issuer(parent.NymID(), rowID)}
    , currency_{currency}
{
    OT_ASSERT(issuer_)

    init();
    setup_listeners(listeners_);
    startup_.reset(new std::thread(&IssuerItem::startup, this));

    OT_ASSERT(startup_)
}

void IssuerItem::construct_row(
    const IssuerItemRowID& id,
    const IssuerItemSortKey& index,
    const CustomData& custom) const
{
    items_[index].emplace(
        id,
        Factory::AccountSummaryItem(
            *this,
            zmq_,
            publisher_,
            contact_manager_,
            id,
            index,
            custom,
            wallet_,
            storage_));
    names_.emplace(id, index);
}

std::string IssuerItem::Name() const
{
    sLock lock(shared_lock_);

    return name_;
}

void IssuerItem::process_account(const Identifier& accountID)
{
    const auto account = wallet_.Account(legacy_.ClientDataFolder(), accountID);

    if (false == bool(account)) { return; }

    String name{""};
    account.get().GetName(name);
    const IssuerItemRowID rowID{accountID, currency_};
    const IssuerItemSortKey sortKey{name.Get()};
    CustomData custom{};
    custom.emplace_back(new Amount(account.get().GetBalance()));
    add_item(rowID, sortKey, custom);
}

void IssuerItem::process_account(const network::zeromq::Message& message)
{
    wait_for_startup();

    OT_ASSERT(2 == message.Body().size())

    const auto accountID = Identifier::Factory(message.Body().at(0));
    const IssuerItemRowID rowID{accountID, {storage_.AccountUnit(accountID)}};

    if (1 == names_.count(rowID)) { process_account(accountID); }
}

void IssuerItem::refresh_accounts()
{
    const auto blank = Identifier::Factory();
    const auto accounts = issuer_->AccountList(currency_, blank);
    otWarn << OT_METHOD << __FUNCTION__ << ": Loading " << accounts.size()
           << " accounts." << std::endl;

    for (const auto& id : accounts) { process_account(id); }

    std::set<IssuerItemRowID> active{};
    std::transform(
        accounts.begin(),
        accounts.end(),
        std::inserter(active, active.end()),
        [&](const OTIdentifier& in) -> IssuerItemRowID {
            return {in, currency_};
        });
    delete_inactive(active);
}

void IssuerItem::reindex(const AccountSummarySortKey& key, const CustomData&)
{
    eLock lock(shared_lock_);
    key_ = key;
    connection_.store(std::get<0>(key_));
    lock.unlock();
    refresh_accounts();
}

void IssuerItem::startup()
{
    refresh_accounts();
    startup_complete_->On();
}
}  // namespace opentxs::ui::implementation
