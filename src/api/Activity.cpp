/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#include "opentxs/stdafx.hpp"

#include "Activity.hpp"

#include "opentxs/api/client/Wallet.hpp"
#include "opentxs/api/storage/Storage.hpp"
#include "opentxs/api/ContactManager.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/core/contract/peer/PeerObject.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/PublishSocket.hpp"

#include <thread>

#define OT_METHOD "opentxs::api::implementation::Activity::"

namespace opentxs::api::implementation
{
Activity::Activity(
    const ContactManager& contact,
    const storage::Storage& storage,
    const client::Wallet& wallet,
    const opentxs::network::zeromq::Context& zmq)
    : contact_(contact)
    , storage_(storage)
    , wallet_(wallet)
    , zmq_(zmq)
    , mail_cache_lock_()
    , mail_cache_()
    , publisher_lock_()
    , thread_publishers_()
{
}

void Activity::activity_preload_thread(
    const Identifier nym,
    const std::size_t count) const
{
    const std::string nymID = nym.str();
    auto threads = storage_.ThreadList(nymID, false);

    for (const auto& it : threads) {
        const auto& threadID = it.first;
        thread_preload_thread(nymID, threadID, 0, count);
    }
}

bool Activity::AddBlockchainTransaction(
    const Identifier& nymID,
    const Identifier& threadID,
    const StorageBox box,
    const proto::BlockchainTransaction& transaction) const
{
    const std::string sNymID = nymID.str();
    const std::string sthreadID = threadID.str();
    const auto threadList = storage_.ThreadList(sNymID, false);
    bool threadExists = false;

    for (const auto it : threadList) {
        const auto& id = it.first;

        if (id == sthreadID) {
            threadExists = true;
            break;
        }
    }

    if (false == threadExists) {
        storage_.CreateThread(sNymID, sthreadID, {sthreadID});
    }

    const bool saved = storage_.Store(
        sNymID, sthreadID, transaction.txid(), transaction.time(), {}, {}, box);

    if (saved) {
        publish(nymID, sthreadID);
    }

    return saved;
}

const opentxs::network::zeromq::PublishSocket& Activity::get_publisher(
    const Identifier& nymID) const
{
    std::string endpoint{};

    return get_publisher(nymID, endpoint);
}

const opentxs::network::zeromq::PublishSocket& Activity::get_publisher(
    const Identifier& nymID,
    std::string& endpoint) const
{
    endpoint =
        opentxs::network::zeromq::Socket::ThreadUpdateEndpoint + nymID.str();
    Lock lock(publisher_lock_);
    auto it = thread_publishers_.find(nymID);

    if (thread_publishers_.end() != it) {

        return it->second;
    }

    const auto & [ publisher, inserted ] =
        thread_publishers_.emplace(nymID, zmq_.PublishSocket());

    OT_ASSERT(inserted)

    auto& output = publisher->second.get();
    output.Start(endpoint);
    otWarn << OT_METHOD << __FUNCTION__ << ": Publisher started on " << endpoint
           << std::endl;

    return output;
}

bool Activity::MoveIncomingBlockchainTransaction(
    const Identifier& nymID,
    const Identifier& fromThreadID,
    const Identifier& toThreadID,
    const std::string& txid) const
{
    return storage_.MoveThreadItem(
        nymID.str(), fromThreadID.str(), toThreadID.str(), txid);
}

std::unique_ptr<Message> Activity::Mail(
    const Identifier& nym,
    const Identifier& id,
    const StorageBox& box) const
{
    std::string raw, alias;
    const bool loaded =
        storage_.Load(nym.str(), id.str(), box, raw, alias, true);

    std::unique_ptr<Message> output;

    if (!loaded) {
        otErr << OT_METHOD << __FUNCTION__ << ": Failed to load Message"
              << std::endl;

        return output;
    }

    if (raw.empty()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Empty message" << std::endl;

        return output;
    }

    output.reset(new Message);

    OT_ASSERT(output);

    if (false == output->LoadContractFromString(String(raw.c_str()))) {
        otErr << OT_METHOD << __FUNCTION__ << ": Failed to deserialized Message"
              << std::endl;

        output.reset();
    }

    return output;
}

std::string Activity::Mail(
    const Identifier& nym,
    const Message& mail,
    const StorageBox box) const
{
    const std::string nymID = nym.str();
    Identifier id{};
    mail.CalculateContractID(id);
    const std::string output = id.str();
    const String data(mail);
    std::string participantNymID{};
    const String localName(nym);

    if (localName == mail.m_strNymID2) {
        // This is an incoming message. The contact id is the sender's id.
        participantNymID = mail.m_strNymID.Get();
    } else {
        // This is an outgoing message. The contact id is the recipient's id.
        participantNymID = mail.m_strNymID2.Get();
    }

    const auto contact = nym_to_contact(participantNymID);

    OT_ASSERT(contact);

    std::string alias = contact->Label();
    const std::string contactID = contact->ID().str();
    const auto& threadID = contactID;
    const auto threadList = storage_.ThreadList(nymID, false);
    bool threadExists = false;

    for (const auto it : threadList) {
        const auto& id = it.first;

        if (id == threadID) {
            threadExists = true;
            break;
        }
    }

    if (false == threadExists) {
        storage_.CreateThread(nymID, threadID, {contactID});
    }

    const bool saved = storage_.Store(
        localName.Get(),
        threadID,
        output,
        mail.m_lTime,
        alias,
        data.Get(),
        box);

    if (saved) {
        std::thread preload(&Activity::preload, this, nym, id, box);
        preload.detach();
        publish(nym, threadID);

        return output;
    }

    return "";
}

ObjectList Activity::Mail(const Identifier& nym, const StorageBox box) const
{
    return storage_.NymBoxList(nym.str(), box);
}

bool Activity::MailRemove(
    const Identifier& nym,
    const Identifier& id,
    const StorageBox box) const
{
    const std::string nymid = nym.str();
    const std::string mail = id.str();

    return storage_.RemoveNymBoxItem(nymid, box, mail);
}

std::shared_ptr<const std::string> Activity::MailText(
    const Identifier& nymID,
    const Identifier& id,
    const StorageBox& box) const
{
    Lock lock(mail_cache_lock_);
    auto it = mail_cache_.find(id);
    lock.unlock();

    if (mail_cache_.end() != it) {

        return it->second;
    }

    preload(nymID, id, box);
    lock.lock();
    it = mail_cache_.find(id);
    lock.unlock();

    if (mail_cache_.end() == it) {

        return {};
    }

    return it->second;
}

bool Activity::MarkRead(
    const Identifier& nymId,
    const Identifier& threadId,
    const Identifier& itemId) const
{
    const std::string nym = nymId.str();
    const std::string thread = threadId.str();
    const std::string item = itemId.str();

    return storage_.SetReadState(nym, thread, item, false);
}

bool Activity::MarkUnread(
    const Identifier& nymId,
    const Identifier& threadId,
    const Identifier& itemId) const
{
    const std::string nym = nymId.str();
    const std::string thread = threadId.str();
    const std::string item = itemId.str();

    return storage_.SetReadState(nym, thread, item, true);
}

void Activity::MigrateLegacyThreads() const
{
    std::set<std::string> contacts{};

    for (const auto& it : contact_.ContactList()) {
        contacts.insert(it.first);
    }

    const auto nymlist = storage_.NymList();

    for (const auto& it1 : nymlist) {
        const auto& nymID = it1.first;
        const auto threadList = storage_.ThreadList(nymID, false);

        for (const auto& it2 : threadList) {
            const auto& originalThreadID = it2.first;
            const bool isContactID = (1 == contacts.count(originalThreadID));

            if (isContactID) {

                continue;
            }

            auto contactID = contact_.ContactID(Identifier(originalThreadID));

            if (false == contactID.empty()) {
                storage_.RenameThread(nymID, originalThreadID, contactID.str());
            } else {
                std::shared_ptr<proto::StorageThread> thread;
                storage_.Load(nymID, originalThreadID, thread);

                OT_ASSERT(thread);

                const auto nymCount = thread->participant().size();

                if (1 == nymCount) {
                    auto newContact = contact_.NewContact(
                        "", Identifier(originalThreadID), PaymentCode(""));

                    OT_ASSERT(newContact);

                    storage_.RenameThread(
                        nymID, originalThreadID, newContact->ID().str());
                } else {
                    // Multi-party chats were not implemented prior to the
                    // update to contact IDs, so there is no need to handle
                    // this case
                }
            }
        }
    }
}

std::shared_ptr<const Contact> Activity::nym_to_contact(
    const std::string& id) const
{
    const Identifier nymID(id);
    auto contactID = contact_.ContactID(nymID);

    if (false == contactID.empty()) {

        return contact_.Contact(contactID);
    }

    // Contact does not yet exist. Create it.
    std::string label{};
    auto nym = wallet_.Nym(nymID);
    std::unique_ptr<PaymentCode> code;

    if (nym) {
        label = nym->Claims().Name();
        code.reset(new PaymentCode(nym->PaymentCode()));
    }

    if (false == bool(code)) {
        code.reset(new PaymentCode(""));
    }

    OT_ASSERT(code);

    return contact_.NewContact(label, nymID, *code);
}

void Activity::preload(
    const Identifier nymID,
    const Identifier id,
    const StorageBox box) const
{
    const auto message = Mail(nymID, id, box);

    if (!message) {
        otErr << OT_METHOD << __FUNCTION__ << ": Unable to load message "
              << String(id) << std::endl;

        return;
    }

    auto nym = wallet_.Nym(nymID);

    if (false == bool(nym)) {
        otErr << OT_METHOD << __FUNCTION__ << ": Unable to load recipent nym."
              << std::endl;

        return;
    }

    otErr << OT_METHOD << __FUNCTION__ << ": Decrypting message " << id.str()
          << std::endl;
    auto peerObject = PeerObject::Factory(nym, message->m_ascPayload);
    otErr << OT_METHOD << __FUNCTION__ << ": Message " << id.str()
          << " decrypted." << std::endl;

    if (!peerObject) {
        otErr << OT_METHOD << __FUNCTION__
              << ": Unable to instantiate peer object." << std::endl;

        return;
    }

    if (!peerObject->Message()) {
        otErr << OT_METHOD << __FUNCTION__
              << ": Peer object does not contain a message." << std::endl;

        return;
    }

    Lock lock(mail_cache_lock_);
    auto& output = mail_cache_[id];
    lock.unlock();
    output.reset(new std::string(*peerObject->Message()));
}

void Activity::PreloadActivity(const Identifier& nymID, const std::size_t count)
    const
{
    std::thread preload(&Activity::activity_preload_thread, this, nymID, count);
    preload.detach();
}

void Activity::PreloadThread(
    const Identifier& nymID,
    const Identifier& threadID,
    const std::size_t start,
    const std::size_t count) const
{
    const std::string nym = nymID.str();
    const std::string thread = threadID.str();
    std::thread preload(
        &Activity::thread_preload_thread, this, nym, thread, start, count);
    preload.detach();
}

void Activity::publish(const Identifier& nymID, const std::string& threadID)
    const
{
    auto& publisher = get_publisher(nymID);
    publisher.Publish(threadID);
}

std::shared_ptr<proto::StorageThread> Activity::Thread(
    const Identifier& nymID,
    const Identifier& threadID) const
{
    std::shared_ptr<proto::StorageThread> output;
    storage_.Load(nymID.str(), threadID.str(), output);

    return output;
}

void Activity::thread_preload_thread(
    const std::string nymID,
    const std::string threadID,
    const std::size_t start,
    const std::size_t count) const
{
    std::shared_ptr<proto::StorageThread> thread{};
    const bool loaded = storage_.Load(nymID, threadID, thread);

    if (false == loaded) {
        otErr << OT_METHOD << __FUNCTION__ << ": Unable to load thread "
              << threadID << " for nym " << nymID << std::endl;

        return;
    }

    const std::size_t size = thread->item_size();
    std::size_t cached{0};

    if (start > size) {
        otErr << OT_METHOD << __FUNCTION__ << ": Error: start larger than size "
              << "(" << std::to_string(start) << "/" << std::to_string(size)
              << ")" << std::endl;

        return;
    }

    for (auto i = (size - start); i > 0; --i) {
        if (cached >= count) {
            break;
        }

        const auto& item = thread->item(i - 1);
        const auto& box = static_cast<StorageBox>(item.box());

        switch (box) {
            case StorageBox::MAILINBOX:
            case StorageBox::MAILOUTBOX: {
                otErr << OT_METHOD << __FUNCTION__ << ": Preloading item "
                      << item.id() << " in thread " << threadID << std::endl;
                MailText(Identifier(nymID), Identifier(item.id()), box);
                ++cached;
            } break;
            default: {
                continue;
            }
        }
    }
}

std::string Activity::ThreadPublisher(const Identifier& nym) const
{
    std::string endpoint{};
    get_publisher(nym, endpoint);

    return endpoint;
}

ObjectList Activity::Threads(const Identifier& nym, const bool unreadOnly) const
{
    const std::string nymID = nym.str();
    auto output = storage_.ThreadList(nymID, unreadOnly);

    for (auto& it : output) {
        const auto& threadID = it.first;
        auto& label = it.second;

        if (label.empty()) {
            auto contact = contact_.Contact(Identifier(threadID));

            if (contact) {
                const auto& name = contact->Label();

                if (label != name) {
                    storage_.SetThreadAlias(nymID, threadID, name);
                    label = name;
                }
            }
        }
    }

    return output;
}

std::size_t Activity::UnreadCount(const Identifier& nymId) const
{
    const std::string nym = nymId.str();
    std::size_t output{0};

    const auto& threads = storage_.ThreadList(nym, true);

    for (const auto& it : threads) {
        const auto& threadId = it.first;
        output += storage_.UnreadCount(nym, threadId);
    }

    return output;
}
}  // namespace opentxs::api::implementation
