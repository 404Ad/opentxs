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

#include "opentxs/api/client/Pair.hpp"
#include "opentxs/api/client/ServerAction.hpp"
#include "opentxs/api/client/Wallet.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/Api.hpp"
#include "opentxs/api/ContactManager.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/client/NymData.hpp"
#include "opentxs/client/OT_API.hpp"
#include "opentxs/client/OTAPI_Exec.hpp"
#include "opentxs/client/OTWallet.hpp"
#include "opentxs/client/ServerAction.hpp"
#include "opentxs/client/Utility.hpp"
#include "opentxs/consensus/ServerContext.hpp"
#include "opentxs/contact/Contact.hpp"
#include "opentxs/contact/ContactData.hpp"
#include "opentxs/contact/ContactGroup.hpp"
#include "opentxs/contact/ContactItem.hpp"
#include "opentxs/core/crypto/OTPassword.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Cheque.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/ext/OTPayment.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/PublishSocket.hpp"

#include <chrono>

#include "Sync.hpp"

#define CONTACT_REFRESH_DAYS 1
#define CONTRACT_DOWNLOAD_SECONDS 10
#define MAIN_LOOP_SECONDS 5
#define NYM_REGISTRATION_SECONDS 10

#define SHUTDOWN()                                                             \
    {                                                                          \
        YIELD(50);                                                             \
    }

#define YIELD(a)                                                               \
    {                                                                          \
        if (!running_) {                                                       \
                                                                               \
            return;                                                            \
        }                                                                      \
                                                                               \
        Log::Sleep(std::chrono::milliseconds(a));                              \
    }

#define CHECK_NYM(a)                                                           \
    {                                                                          \
        if (a.empty()) {                                                       \
            otErr << OT_METHOD << __FUNCTION__ << ": Invalid " << #a           \
                  << std::endl;                                                \
                                                                               \
            return {};                                                         \
        }                                                                      \
    }

#define CHECK_SERVER(a, b)                                                     \
    {                                                                          \
        CHECK_NYM(a)                                                           \
                                                                               \
        if (b.empty()) {                                                       \
            otErr << OT_METHOD << __FUNCTION__ << ": Invalid " << #b           \
                  << std::endl;                                                \
                                                                               \
            return {};                                                         \
        }                                                                      \
    }

#define CHECK_ARGS(a, b, c)                                                    \
    {                                                                          \
        CHECK_SERVER(a, b)                                                     \
                                                                               \
        if (c.empty()) {                                                       \
            otErr << OT_METHOD << __FUNCTION__ << ": Invalid " << #c           \
                  << std::endl;                                                \
                                                                               \
            return {};                                                         \
        }                                                                      \
    }

#define INTRODUCTION_SERVER_KEY "introduction_server_id"
#define MASTER_SECTION "Master"
#define PROCESS_INBOX_RETRIES 3

#define OT_METHOD "opentxs::api::client::implementation::Sync::"

namespace opentxs::api::client::implementation
{

const std::string Sync::DEFAULT_INTRODUCTION_SERVER =
    R"(-----BEGIN OT ARMORED SERVER CONTRACT-----
Version: Open Transactions 0.99.1-113-g2b3acf5
Comment: http://opentransactions.org

CAESI290b20xcHFmREJLTmJLR3RiN0NBa0ZodFRXVFVOTHFIRzIxGiNvdHVkd3p4cWF0UHh4
bmh4VFV3RUo3am5HenE2RkhGYTRraiIMU3Rhc2ggQ3J5cHRvKr8NCAESI290dWR3enhxYXRQ
eHhuaHhUVXdFSjdqbkd6cTZGSEZhNGtqGAIoATJTCAEQAiJNCAESIQI9MywLxxKfOtai26pj
JbxKtCCPhM/DbvX08iwbW2qYqhoga6Ccvp6CABGAFj/RdWNjtg5uzIRHT5Dn+fUzdAM9SUSA
AQCIAQA6vAwIARIjb3R1ZHd6eHFhdFB4eG5oeFRVd0VKN2puR3pxNkZIRmE0a2oaI290dXdo
ZzNwb2kxTXRRdVkzR3hwYWpOaXp5bmo0NjJ4Z2RIIAIymgQIARIjb3R1d2hnM3BvaTFNdFF1
WTNHeHBhak5penluajQ2MnhnZEgYAiABKAIyI290dWR3enhxYXRQeHhuaHhUVXdFSjdqbkd6
cTZGSEZhNGtqQl0IARJTCAEQAiJNCAESIQI9MywLxxKfOtai26pjJbxKtCCPhM/DbvX08iwb
W2qYqhoga6Ccvp6CABGAFj/RdWNjtg5uzIRHT5Dn+fUzdAM9SUSAAQCIAQAaBAgBEAJKiAEI
ARACGioIARAEGAIgASogZ6MtTp4aTEDLxFfhnsGo+Esp5B4hkgjWEejNPt5J6C0aKggBEAQY
AiACKiAhqJjWf2Ugqbg6z6ps59crAx9lHwTuT6Eq4x6JmkBlGBoqCAEQBBgCIAMqII2Vps1F
C2YUMbB4kE9XsHt1jrVY6pMPV6KWc5sH3VvTem0IARIjb3R1d2hnM3BvaTFNdFF1WTNHeHBh
ak5penluajQ2MnhnZEgYASAFKkDQLsszAol/Ih56MomuBKV8zpKaw5+ry7Kse1+5nPwJlP8f
72OAgTegBlmv31K4JgLVs52EKJTBpjnV+v0pxzUOem0IARIjb3R1ZHd6eHFhdFB4eG5oeFRV
d0VKN2puR3pxNkZIRmE0a2oYAyAFKkAJZ0LTVM+XBrGbRdiZsEQSbvwqg+mqGwHD5MQ+D4h0
fPQaUrdB6Pp/HM5veox02LBKg05hVNQ64tcU+LAxK+VHQuQDCAESI290clA2dDJXY2hYMjYz
ZVpiclRuVzZyY2FCZVNQb2VqSzJnGAIgAigCMiNvdHVkd3p4cWF0UHh4bmh4VFV3RUo3am5H
enE2RkhGYTRrajonCAESI290dXdoZzNwb2kxTXRRdVkzR3hwYWpOaXp5bmo0NjJ4Z2RISogB
CAEQAhoqCAEQBBgCIAEqIDpwlCrxHNWvvtFt6k8ocB5NBo7vjkGO/mRuSOQ/j/9WGioIARAE
GAIgAiog6Dw0+AWok4dENWWc/3qhykA7NNybWecqMGs5fL8KLLYaKggBEAQYAiADKiD+s/iq
37NrYI4/xdHOYtO/ocR0YqDVz09IaDNGVEdBtnptCAESI290clA2dDJXY2hYMjYzZVpiclRu
VzZyY2FCZVNQb2VqSzJnGAEgBSpATbHtakma53Na35Be+rGvW+z1H6EtkHlljv9Mo8wfies3
in9el1Ejb4BDbGCN5ABl3lQpfedZnR+VYv2X6Y1yBnptCAESI290dXdoZzNwb2kxTXRRdVkz
R3hwYWpOaXp5bmo0NjJ4Z2RIGAEgBSpAeptEmgdqgkGUcOJCqG0MsiChEREUdDzH/hRj877u
WDIHoRHsf/k5dCOHfDct4TDszasVhGFhRdNunpgQJcp0DULnAwgBEiNvdHd6ZWd1dTY3cENI
RnZhYjZyS2JYaEpXelNvdlNDTGl5URgCIAIoAjIjb3R1ZHd6eHFhdFB4eG5oeFRVd0VKN2pu
R3pxNkZIRmE0a2o6JwgBEiNvdHV3aGczcG9pMU10UXVZM0d4cGFqTml6eW5qNDYyeGdkSEqL
AQgBEAIaKwgBEAMYAiABKiEC5p36Ivxs4Wb6CjKTnDA1MFtX3Mx2UBlrmloSt+ffXz0aKwgB
EAMYAiACKiECtMkEo4xsefeevzrBb62ll98VYZy8PipgbrPWqGUNxQMaKwgBEAMYAiADKiED
W1j2DzOZemB9OOZ/pPrFroKDfgILYu2IOtiRFfi0vDB6bQgBEiNvdHd6ZWd1dTY3cENIRnZh
YjZyS2JYaEpXelNvdlNDTGl5URgBIAUqQJYd860/Ybh13GtW+grxWtWjjmzPifHE7bTlgUWl
3bX+ZuWNeEotA4yXQvFNog4PTAOF6dbvCr++BPGepBEUEEx6bQgBEiNvdHV3aGczcG9pMU10
UXVZM0d4cGFqTml6eW5qNDYyeGdkSBgBIAUqQH6GXnKCCDDgDvcSt8dLWuVMlr75zVkHy85t
tccoy2oLHNevDvKrLfUk/zuICyaSIvDy0Kb2ytOuh/O17yabxQ8yHQgBEAEYASISb3Quc3Rh
c2hjcnlwdG8ubmV0KK03MiEIARADGAEiFnQ1NGxreTJxM2w1ZGt3bnQub25pb24orTcyRwgB
EAQYASI8b3ZpcDZrNWVycXMzYm52cjU2cmgzZm5pZ2JuZjJrZWd1cm5tNWZpYnE1NWtqenNv
YW54YS5iMzIuaTJwKK03Op8BTWVzc2FnaW5nLW9ubHkgc2VydmVyIHByb3ZpZGVkIGZvciB0
aGUgY29udmllbmllbmNlIG9mIFN0YXNoIENyeXB0byB1c2Vycy4gU2VydmljZSBpcyBwcm92
aWRlZCBhcyBpcyB3aXRob3V0IHdhcnJhbnR5IG9mIGFueSBraW5kLCBlaXRoZXIgZXhwcmVz
c2VkIG9yIGltcGxpZWQuQiCK4L5cnecfUFz/DQyvAklKC2pTmWQtxt9olQS5/0hUHUptCAES
I290clA2dDJXY2hYMjYzZVpiclRuVzZyY2FCZVNQb2VqSzJnGAUgBSpA1/bep0NTbisZqYns
MCL/PCUJ6FIMhej+ROPk41604x1jeswkkRmXRNjzLlVdiJ/pQMxG4tJ0UQwpxHxrr0IaBA==
-----END OT ARMORED SERVER CONTRACT-----)";

Sync::Sync(
    std::recursive_mutex& apiLock,
    const Flag& running,
    const OT_API& otapi,
    const opentxs::OTAPI_Exec& exec,
    const api::ContactManager& contacts,
    const api::Settings& config,
    const api::Api& api,
    const api::client::Wallet& wallet,
    const api::crypto::Encode& encoding,
    const opentxs::network::zeromq::Context& zmq)
    : api_lock_(apiLock)
    , running_(running)
    , ot_api_(otapi)
    , exec_(exec)
    , contacts_(contacts)
    , config_(config)
    , api_(api)
    , server_action_(api.ServerAction())
    , wallet_(wallet)
    , encoding_(encoding)
    , zmq_(zmq)
    , introduction_server_lock_()
    , nym_fetch_lock_()
    , task_status_lock_()
    , refresh_counter_(0)
    , operations_()
    , server_nym_fetch_()
    , missing_nyms_()
    , missing_servers_()
    , state_machines_()
    , introduction_server_id_()
    , task_status_()
    , nym_publisher_(zmq.PublishSocket())
{
    nym_publisher_->Start(
        opentxs::network::zeromq::Socket::NymDownloadEndpoint);
}

std::pair<bool, std::size_t> Sync::accept_incoming(
    const rLock& lock[[maybe_unused]],
    const std::size_t max,
    const Identifier& accountID,
    ServerContext& context) const
{
    std::pair<bool, std::size_t> output{false, 0};
    auto & [ success, remaining ] = output;
    const std::string account = accountID.str();
    auto processInbox = ot_api_.CreateProcessInbox(accountID, context);
    auto& response = std::get<0>(processInbox);
    auto& inbox = std::get<1>(processInbox);

    if (false == bool(response)) {
        if (nullptr == inbox) {
            // This is a new account which has never instantiated an inbox.
            success = true;

            return output;
        }

        otErr << OT_METHOD << __FUNCTION__
              << ": Error instantiating processInbox for account: " << account
              << std::endl;

        return output;
    }

    const std::size_t items =
        (inbox->GetTransactionCount() >= 0) ? inbox->GetTransactionCount() : 0;
    const std::size_t count = (items > max) ? max : items;
    remaining = items - count;

    if (0 == count) {
        otInfo << OT_METHOD << __FUNCTION__
               << ": No items to accept in this account." << std::endl;
        success = true;

        return output;
    }

    for (std::size_t i = 0; i < count; i++) {
        auto transaction = inbox->GetTransactionByIndex(i);

        OT_ASSERT(nullptr != transaction);

        const TransactionNumber number = transaction->GetTransactionNum();

        if (transaction->IsAbbreviated()) {
            inbox->LoadBoxReceipt(number);
            transaction = inbox->GetTransaction(number);

            if (nullptr == transaction) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": Unable to load item: " << number << std::endl;

                continue;
            }
        }

        const bool accepted = ot_api_.IncludeResponse(
            accountID, true, context, *transaction, *response);

        if (!accepted) {
            otErr << OT_METHOD << __FUNCTION__
                  << ": Failed to accept item: " << number << std::endl;

            return output;
        }
    }

    const bool finalized =
        ot_api_.FinalizeProcessInbox(accountID, context, *response, *inbox);

    if (false == finalized) {
        otErr << OT_METHOD << __FUNCTION__ << ": Unable to finalize response."
              << std::endl;

        return output;
    }

    auto action = server_action_.ProcessInbox(
        context.Nym()->ID(), context.Server(), accountID, response);
    action->Run();
    success = (SendResult::VALID_REPLY == action->LastSendResult());

    return output;
}

bool Sync::AcceptIncoming(
    const Identifier& nymID,
    const Identifier& accountID,
    const Identifier& serverID,
    const std::size_t max) const
{
    rLock apiLock(api_lock_);
    auto context = wallet_.mutable_ServerContext(nymID, serverID);
    std::size_t remaining{1};
    std::size_t retries{PROCESS_INBOX_RETRIES};

    while (0 < remaining) {
        const auto attempt =
            accept_incoming(apiLock, max, accountID, context.It());
        const auto & [ success, unprocessed ] = attempt;
        remaining = unprocessed;

        if (false == success) {
            if (0 == retries) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": Exceeded maximum retries." << std::endl;

                return false;
            }

            Utility utility(context.It(), ot_api_);
            const auto download = utility.getIntermediaryFiles(
                context.It().Server().str(),
                context.It().Nym()->ID().str(),
                accountID.str(),
                true);

            if (false == download) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": Failed to download account files." << std::endl;

                return false;
            } else {
                --retries;

                continue;
            }
        }

        if (0 != remaining) {
            otErr << OT_METHOD << __FUNCTION__ << ": Accepting " << remaining
                  << " more items." << std::endl;
        }
    }

    return true;
}

void Sync::add_task(const Identifier& taskID, const ThreadStatus status) const
{
    Lock lock(task_status_lock_);

    if (0 != task_status_.count(taskID)) {

        return;
    }

    task_status_[taskID] = status;
}

void Sync::associate_message_id(
    const Identifier& messageID,
    const Identifier& taskID) const
{
    Lock lock(task_status_lock_);
    task_message_id_.emplace(taskID, messageID);
}

Depositability Sync::can_deposit(
    const OTPayment& payment,
    const Identifier& recipient,
    const Identifier& accountIDHint,
    Identifier& depositServer,
    Identifier& depositAccount) const
{
    Identifier unitID{};
    Identifier nymID{};

    if (false == extract_payment_data(payment, nymID, depositServer, unitID)) {

        return Depositability::INVALID_INSTRUMENT;
    }

    auto output = valid_recipient(payment, nymID, recipient);

    if (Depositability::READY != output) {

        return output;
    }

    const bool registered =
        exec_.IsNym_RegisteredAtServer(recipient.str(), depositServer.str());

    if (false == registered) {
        schedule_download_nymbox(recipient, depositServer);
        otErr << OT_METHOD << __FUNCTION__ << ": Recipient nym "
              << String(recipient) << " not registered on server "
              << String(depositServer) << std::endl;

        return Depositability::NOT_REGISTERED;
    }

    output = valid_account(
        payment,
        recipient,
        depositServer,
        unitID,
        accountIDHint,
        depositAccount);

    switch (output) {
        case Depositability::ACCOUNT_NOT_SPECIFIED: {
            otErr << OT_METHOD << __FUNCTION__
                  << ": Multiple valid accounts exist. "
                  << "This payment can not be automatically deposited"
                  << std::endl;
        } break;
        case Depositability::WRONG_ACCOUNT: {
            otErr << OT_METHOD << __FUNCTION__
                  << ": The specified account is not valid for this payment."
                  << std::endl;
        } break;
        case Depositability::NO_ACCOUNT: {
            otErr << OT_METHOD << __FUNCTION__ << ": Recipient "
                  << String(recipient) << " needs an account for "
                  << String(unitID) << " on server " << String(depositServer)
                  << std::endl;
            schedule_register_account(recipient, depositServer, unitID);
        } break;
        case Depositability::READY: {
            otWarn << OT_METHOD << __FUNCTION__ << ": Payment can be deposited."
                   << std::endl;
        } break;
        default: {
            OT_FAIL
        }
    }

    return output;
}

Messagability Sync::can_message(
    const Identifier& senderNymID,
    const Identifier& recipientContactID,
    Identifier& recipientNymID,
    Identifier& serverID) const
{
    auto senderNym = wallet_.Nym(senderNymID);

    if (false == bool(senderNym)) {
        otErr << OT_METHOD << __FUNCTION__ << ": Unable to load sender nym "
              << String(senderNymID) << std::endl;

        return Messagability::MISSING_SENDER;
    }

    const bool canSign = senderNym->hasCapability(NymCapability::SIGN_MESSAGE);

    if (false == canSign) {
        otErr << OT_METHOD << __FUNCTION__ << ": Sender nym "
              << String(senderNymID)
              << " can not sign messages (no private key)." << std::endl;

        return Messagability::INVALID_SENDER;
    }

    const auto contact = contacts_.Contact(recipientContactID);

    if (false == bool(contact)) {
        otErr << OT_METHOD << __FUNCTION__ << ": Recipient contact "
              << String(recipientContactID) << " does not exist." << std::endl;

        return Messagability::MISSING_CONTACT;
    }

    const auto nyms = contact->Nyms();

    if (0 == nyms.size()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Recipient contact "
              << String(recipientContactID) << " does not have a nym."
              << std::endl;

        return Messagability::CONTACT_LACKS_NYM;
    }

    std::shared_ptr<const Nym> recipientNym{nullptr};

    for (const auto& it : nyms) {
        recipientNym = wallet_.Nym(it);

        if (recipientNym) {
            recipientNymID = it;
            break;
        }
    }

    if (false == bool(recipientNym)) {
        for (const auto& id : nyms) {
            missing_nyms_.Push(Identifier::Factory(), id);
        }

        otErr << OT_METHOD << __FUNCTION__ << ": Recipient contact "
              << String(recipientContactID) << " credentials not available."
              << std::endl;

        return Messagability::MISSING_RECIPIENT;
    }

    const auto claims = recipientNym->Claims();
    serverID = claims.PreferredOTServer();

    // TODO maybe some of the other nyms in this contact do specify a server
    if (serverID.empty()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Recipient contact "
              << String(recipientContactID) << ", nym "
              << String(recipientNymID)
              << ": credentials do not specify a server." << std::endl;
        missing_nyms_.Push(Identifier::Factory(), recipientNymID);

        return Messagability::NO_SERVER_CLAIM;
    }

    const bool registered =
        exec_.IsNym_RegisteredAtServer(senderNymID.str(), serverID.str());

    if (false == registered) {
        schedule_download_nymbox(senderNymID, serverID);
        otErr << OT_METHOD << __FUNCTION__ << ": Sender nym "
              << String(senderNymID) << " not registered on server "
              << String(serverID) << std::endl;

        return Messagability::UNREGISTERED;
    }

    return Messagability::READY;
}

Depositability Sync::CanDeposit(
    const Identifier& recipientNymID,
    const OTPayment& payment) const
{
    Identifier accountHint;

    return CanDeposit(recipientNymID, accountHint, payment);
}

Depositability Sync::CanDeposit(
    const Identifier& recipientNymID,
    const Identifier& accountIDHint,
    const OTPayment& payment) const
{
    Identifier serverID;
    Identifier accountID;

    return can_deposit(
        payment, recipientNymID, accountIDHint, serverID, accountID);
}

Messagability Sync::CanMessage(
    const Identifier& senderNymID,
    const Identifier& recipientContactID) const
{
    if (senderNymID.empty()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Invalid sender" << std::endl;

        return Messagability::INVALID_SENDER;
    }

    if (recipientContactID.empty()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Invalid recipient"
              << std::endl;

        return Messagability::MISSING_CONTACT;
    }

    Identifier nymID{};
    Identifier serverID{};
    start_introduction_server(senderNymID);

    return can_message(senderNymID, recipientContactID, nymID, serverID);
}

void Sync::check_nym_revision(
    const ServerContext& context,
    OperationQueue& queue) const
{
    if (context.StaleNym()) {
        const auto& nymID = context.Nym()->ID();
        otErr << OT_METHOD << __FUNCTION__ << ": Nym " << String(nymID)
              << " has is newer than version last registered version on server "
              << String(context.Server()) << std::endl;
        queue.register_nym_.Push(Identifier::Factory(), true);
    }
}

bool Sync::check_registration(
    const Identifier& nymID,
    const Identifier& serverID,
    std::shared_ptr<const ServerContext>& context) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())

    context = wallet_.ServerContext(nymID, serverID);
    RequestNumber request{0};

    if (context) {
        request = context->Request();
    } else {
        otErr << OT_METHOD << __FUNCTION__ << ": Nym " << String(nymID)
              << " has never registered on " << String(serverID) << std::endl;
    }

    if (0 != request) {
        OT_ASSERT(context)

        return true;
    }

    const auto output = register_nym(Identifier::Factory(), nymID, serverID);

    if (output) {
        context = wallet_.ServerContext(nymID, serverID);

        OT_ASSERT(context)
    }

    return output;
}

bool Sync::check_server_contract(const Identifier& serverID) const
{
    OT_ASSERT(false == serverID.empty())

    const auto serverContract = wallet_.Server(serverID);

    if (serverContract) {

        return true;
    }

    otErr << OT_METHOD << __FUNCTION__ << ": Server contract for "
          << String(serverID) << " is not in the wallet." << std::endl;
    missing_servers_.Push(Identifier::Factory(), serverID);

    return false;
}

bool Sync::deposit_cheque(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& accountID,
    const std::shared_ptr<const OTPayment>& payment,
    UniqueQueue<DepositPaymentTask>& retry) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == accountID.empty())
    OT_ASSERT(payment)

    if ((false == payment->IsCheque()) && (false == payment->IsVoucher())) {
        otErr << OT_METHOD << __FUNCTION__ << ": Unhandled payment type."
              << std::endl;

        return finish_task(taskID, false);
    }

    std::unique_ptr<Cheque> cheque = std::make_unique<Cheque>();
    const auto loaded = cheque->LoadContractFromString(payment->Payment());

    if (false == loaded) {
        otErr << OT_METHOD << __FUNCTION__ << ": Invalid cheque" << std::endl;

        return finish_task(taskID, false);
    }

    rLock lock(api_lock_);
    auto action =
        server_action_.DepositCheque(nymID, serverID, accountID, cheque);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__
                  << ": Failed to deposit cheque:\n"
                  << String(*cheque) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while depositing cheque "
              << " on server " << String(serverID) << std::endl;
    }

    retry.Push(taskID, {accountID, payment});

    return false;
}

Identifier Sync::DepositPayment(
    const Identifier& recipientNymID,
    const std::shared_ptr<const OTPayment>& payment) const
{
    Identifier notUsed;

    return DepositPayment(recipientNymID, notUsed, payment);
}

Identifier Sync::DepositPayment(
    const Identifier& recipientNymID,
    const Identifier& accountIDHint,
    const std::shared_ptr<const OTPayment>& payment) const
{
    OT_ASSERT(payment)

    if (recipientNymID.empty()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Invalid recipient"
              << std::endl;

        return {};
    }

    Identifier serverID{};
    Identifier accountID{};
    const auto status = can_deposit(
        *payment, recipientNymID, accountIDHint, serverID, accountID);

    switch (status) {
        case Depositability::READY:
        case Depositability::NOT_REGISTERED:
        case Depositability::NO_ACCOUNT: {
            start_introduction_server(recipientNymID);
            auto& queue = get_operations({recipientNymID, serverID});
            const auto taskID(Identifier::Random());

            return start_task(
                taskID,
                queue.deposit_payment_.Push(taskID, {accountIDHint, payment}));
        } break;
        default: {
            otErr << OT_METHOD << __FUNCTION__
                  << ": Unable to queue payment for download" << std::endl;
        }
    }

    return {};
}

bool Sync::download_account(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& accountID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == accountID.empty())

    const auto success =
        server_action_.DownloadAccount(nymID, serverID, accountID, false);

    return finish_task(taskID, success);
}

bool Sync::download_contract(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& contractID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == contractID.empty())

    rLock lock(api_lock_);
    auto action = server_action_.DownloadContract(nymID, serverID, contractID);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            api_.Pair().Update();

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Server "
                  << String(serverID) << " does not have the contract "
                  << String(contractID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while downloading contract "
              << String(contractID) << " from server " << String(serverID)
              << std::endl;
    }

    return finish_task(taskID, false);
}

bool Sync::download_nym(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& targetNymID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == targetNymID.empty())

    rLock lock(api_lock_);
    auto action = server_action_.DownloadNym(nymID, serverID, targetNymID);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            api_.Pair().Update();
            nym_publisher_->Publish(targetNymID.str());

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Server "
                  << String(serverID) << " does not have nym "
                  << String(targetNymID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while downloading nym "
              << String(targetNymID) << " from server " << String(serverID)
              << std::endl;
    }

    return finish_task(taskID, false);
}

bool Sync::download_nymbox(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())

    const auto success = server_action_.DownloadNymbox(nymID, serverID);

    return finish_task(taskID, success);
}

bool Sync::extract_payment_data(
    const OTPayment& payment,
    Identifier& nymID,
    Identifier& serverID,
    Identifier& unitID) const
{
    if (false == payment.GetRecipientNymID(nymID)) {
        otErr << OT_METHOD << __FUNCTION__
              << ": Unable to load recipient nym from instrument" << std::endl;

        return false;
    }

    if (false == payment.GetNotaryID(serverID)) {
        otErr << OT_METHOD << __FUNCTION__
              << ": Unable to load recipient nym from instrument" << std::endl;

        return false;
    }

    OT_ASSERT(false == serverID.empty())

    if (false == payment.GetInstrumentDefinitionID(unitID)) {
        otErr << OT_METHOD << __FUNCTION__
              << ": Unable to load recipient nym from instrument" << std::endl;

        return false;
    }

    OT_ASSERT(false == unitID.empty())

    return true;
}

bool Sync::find_nym(
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& targetID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == targetID.empty())

    const auto nym = wallet_.Nym(targetID);

    if (nym) {
        missing_nyms_.CancelByValue(targetID);

        return true;
    }

    if (download_nym({}, nymID, serverID, targetID)) {
        missing_nyms_.CancelByValue(targetID);

        return true;
    }

    return false;
}

bool Sync::find_server(
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& targetID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == targetID.empty())

    const auto serverContract = wallet_.Server(targetID);

    if (serverContract) {
        missing_servers_.CancelByValue(targetID);

        return true;
    }

    if (download_contract({}, nymID, serverID, targetID)) {
        missing_servers_.CancelByValue(targetID);

        return true;
    }

    return false;
}

Identifier Sync::FindNym(const Identifier& nymID) const
{
    CHECK_NYM(nymID)

    const auto taskID(Identifier::Random());

    return start_task(taskID, missing_nyms_.Push(taskID, nymID));
}

Identifier Sync::FindNym(
    const Identifier& nymID,
    const Identifier& serverIDHint) const
{
    CHECK_NYM(nymID)

    auto& serverQueue = get_nym_fetch(serverIDHint);
    const auto taskID(Identifier::Random());

    return start_task(taskID, serverQueue.Push(taskID, nymID));
}

Identifier Sync::FindServer(const Identifier& serverID) const
{
    CHECK_NYM(serverID)

    const auto taskID(Identifier::Random());

    return start_task(taskID, missing_servers_.Push(taskID, serverID));
}

bool Sync::finish_task(const Identifier& taskID, const bool success) const
{
    if (success) {
        update_task(taskID, ThreadStatus::FINISHED_SUCCESS);
    } else {
        update_task(taskID, ThreadStatus::FINISHED_FAILED);
    }

    return success;
}

bool Sync::get_admin(
    const Identifier& nymID,
    const Identifier& serverID,
    const OTPassword& password) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())

    bool success{false};

    {
        const std::string serverPassword(password.getPassword());
        rLock lock(api_lock_);
        auto action =
            server_action_.RequestAdmin(nymID, serverID, serverPassword);
        action->Run();
        lock.unlock();

        if (SendResult::VALID_REPLY == action->LastSendResult()) {
            auto reply = action->Reply();

            OT_ASSERT(reply)

            success = reply->m_bSuccess;
        }
    }

    auto mContext = wallet_.mutable_ServerContext(nymID, serverID);
    auto& context = mContext.It();
    context.SetAdminAttempted();

    if (success) {
        otErr << OT_METHOD << __FUNCTION__ << ": Got admin on server "
              << String(serverID) << std::endl;
        context.SetAdminSuccess();
    }

    return success;
}

Identifier Sync::get_introduction_server(const Lock& lock) const
{
    OT_ASSERT(verify_lock(lock, introduction_server_lock_))

    bool keyFound = false;
    String serverID;
    rLock apiLock(api_lock_);
    const bool config = config_.Check_str(
        MASTER_SECTION, INTRODUCTION_SERVER_KEY, serverID, keyFound);

    if (!config || !keyFound || !serverID.Exists()) {

        return import_default_introduction_server(lock);
    }

    return Identifier(String(serverID.Get()));
}

UniqueQueue<Identifier>& Sync::get_nym_fetch(const Identifier& serverID) const
{
    Lock lock(nym_fetch_lock_);

    return server_nym_fetch_[serverID];
}

Sync::OperationQueue& Sync::get_operations(const ContextID& id) const
{
    Lock lock(lock_);
    auto& queue = operations_[id];
    auto& thread = state_machines_[id];

    if (false == bool(thread)) {
        thread.reset(new std::thread(
            [id, &queue, this]() { state_machine(id, queue); }));
    }

    return queue;
}

Identifier Sync::import_default_introduction_server(const Lock& lock) const
{
    OT_ASSERT(verify_lock(lock, introduction_server_lock_))

    const auto serialized = proto::StringToProto<proto::ServerContract>(
        DEFAULT_INTRODUCTION_SERVER.c_str());
    const auto instantiated = wallet_.Server(serialized);

    OT_ASSERT(instantiated)

    return set_introduction_server(lock, *instantiated);
}

const Identifier& Sync::IntroductionServer() const
{
    Lock lock(introduction_server_lock_);

    if (false == bool(introduction_server_id_)) {
        load_introduction_server(lock);
    }

    OT_ASSERT(introduction_server_id_)

    return *introduction_server_id_;
}

void Sync::load_introduction_server(const Lock& lock) const
{
    OT_ASSERT(verify_lock(lock, introduction_server_lock_))

    introduction_server_id_.reset(
        new Identifier(get_introduction_server(lock)));
}

bool Sync::message_nym(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& targetNymID,
    const std::string& text) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == targetNymID.empty())

    rLock lock(api_lock_);
    auto action =
        server_action_.SendMessage(nymID, serverID, targetNymID, text);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            const auto messageID = action->MessageID();

            if (false == messageID.empty()) {
                otInfo << OT_METHOD << __FUNCTION__ << ": Sent message  "
                       << messageID.str() << std::endl;
                associate_message_id(messageID, taskID);
            }

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Server  "
                  << String(serverID) << " does not accept message for "
                  << String(targetNymID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while messaging nym "
              << String(targetNymID) << " on server " << String(serverID)
              << std::endl;
    }

    return finish_task(taskID, false);
}

bool Sync::pay_nym(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& targetNymID,
    std::shared_ptr<const OTPayment>& payment) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == targetNymID.empty())

    rLock lock(api_lock_);
    auto action =
        server_action_.SendPayment(nymID, serverID, targetNymID, payment);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            const auto messageID = action->MessageID();

            if (false == messageID.empty()) {
                otInfo << OT_METHOD << __FUNCTION__
                       << ": Sent (payment) "
                          "message "
                       << messageID.str() << std::endl;
            }

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Server  "
                  << String(serverID)
                  << " does not accept (payment) message "
                     "for "
                  << String(targetNymID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while messaging (a payment) to nym "
              << String(targetNymID) << " on server " << String(serverID)
              << std::endl;
    }

    return finish_task(taskID, false);
}

#if OT_CASH
bool Sync::pay_nym_cash(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& targetNymID,
    std::shared_ptr<const Purse>& recipientCopy,
    std::shared_ptr<const Purse>& senderCopy) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == targetNymID.empty())

    rLock lock(api_lock_);
    auto action = server_action_.SendCash(
        nymID, serverID, targetNymID, recipientCopy, senderCopy);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            const auto messageID = action->MessageID();

            if (false == messageID.empty()) {
                otInfo << OT_METHOD << __FUNCTION__ << ": Sent (cash) message  "
                       << messageID.str() << std::endl;
            }

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Server  "
                  << String(serverID) << " does not accept (cash) message for "
                  << String(targetNymID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while messaging (cash) to nym "
              << String(targetNymID) << " on server " << String(serverID)
              << std::endl;
    }

    return finish_task(taskID, false);
}
#endif  // OT_CASH

Identifier Sync::MessageContact(
    const Identifier& senderNymID,
    const Identifier& contactID,
    const std::string& message) const
{
    CHECK_SERVER(senderNymID, contactID)

    start_introduction_server(senderNymID);
    Identifier serverID;
    Identifier recipientNymID;
    const auto canMessage =
        can_message(senderNymID, contactID, recipientNymID, serverID);

    if (Messagability::READY != canMessage) {

        return {};
    }

    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == recipientNymID.empty())

    auto& queue = get_operations({senderNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(
        taskID, queue.send_message_.Push(taskID, {recipientNymID, message}));
}

std::pair<ThreadStatus, Identifier> Sync::MessageStatus(
    const Identifier& taskID) const
{
    std::pair<ThreadStatus, Identifier> output{};
    auto & [ threadStatus, messageID ] = output;
    Lock lock(task_status_lock_);
    threadStatus = status(lock, taskID);

    if (threadStatus == ThreadStatus::FINISHED_SUCCESS) {
        auto it = task_message_id_.find(taskID);

        if (task_message_id_.end() != it) {
            messageID = it->second;
            task_message_id_.erase(it);
        }
    }

    return output;
}

Identifier Sync::PayContact(
    const Identifier& senderNymID,
    const Identifier& contactID,
    std::shared_ptr<const OTPayment>& payment) const
{
    CHECK_SERVER(senderNymID, contactID)

    start_introduction_server(senderNymID);
    Identifier serverID;
    Identifier recipientNymID;
    const auto canMessage =
        can_message(senderNymID, contactID, recipientNymID, serverID);

    if (Messagability::READY != canMessage) {

        return {};
    }

    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == recipientNymID.empty())

    auto& queue = get_operations({senderNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(
        taskID,
        queue.send_payment_.Push(
            taskID,
            {recipientNymID, std::shared_ptr<const OTPayment>(payment)}));
}

#if OT_CASH
Identifier Sync::PayContactCash(
    const Identifier& senderNymID,
    const Identifier& contactID,
    std::shared_ptr<const Purse>& recipientCopy,
    std::shared_ptr<const Purse>& senderCopy) const
{
    CHECK_SERVER(senderNymID, contactID)

    start_introduction_server(senderNymID);
    Identifier serverID;
    Identifier recipientNymID;
    const auto canMessage =
        can_message(senderNymID, contactID, recipientNymID, serverID);

    if (Messagability::READY != canMessage) {

        return {};
    }

    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == recipientNymID.empty())

    auto& queue = get_operations({senderNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(
        taskID,
        queue.send_cash_.Push(
            taskID,
            {recipientNymID,
             std::shared_ptr<const Purse>(recipientCopy),
             std::shared_ptr<const Purse>(senderCopy)}));
}
#endif  // OT_CASH

bool Sync::publish_server_registration(
    const Identifier& nymID,
    const Identifier& serverID,
    const bool forcePrimary) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())

    auto nym = wallet_.mutable_Nym(nymID);

    return nym.AddPreferredOTServer(serverID.str(), forcePrimary);
}

void Sync::Refresh() const
{
    api_.Pair().Update();
    refresh_accounts();

    SHUTDOWN()

    refresh_contacts();
    ++refresh_counter_;
}

std::uint64_t Sync::RefreshCount() const { return refresh_counter_.load(); }

void Sync::refresh_accounts() const
{
    otInfo << OT_METHOD << __FUNCTION__ << ": Begin" << std::endl;
    const auto serverList = wallet_.ServerList();
    const auto accounts = ot_api_.Accounts();

    for (const auto server : serverList) {
        SHUTDOWN()

        const auto serverID = Identifier(server.first);
        otWarn << OT_METHOD << __FUNCTION__ << ": Considering server "
               << String(serverID) << std::endl;

        for (const auto& nymID : ot_api_.LocalNymList()) {
            SHUTDOWN()
            otWarn << OT_METHOD << __FUNCTION__ << ": Nym " << String(nymID)
                   << " ";
            const bool registered =
                ot_api_.IsNym_RegisteredAtServer(nymID, serverID);

            if (registered) {
                otWarn << "is ";
                auto& queue = get_operations({nymID, serverID});
                const auto taskID(Identifier::Random());
                queue.download_nymbox_.Push(taskID, true);
            } else {
                otWarn << "is not ";
            }

            otWarn << "registered here." << std::endl;
        }
    }

    SHUTDOWN()

    for (const auto & [ accountID, nymID, serverID, unitID ] : accounts) {
        SHUTDOWN()

        const auto& notUsed[[maybe_unused]] = unitID;
        otWarn << OT_METHOD << __FUNCTION__ << ": Account " << String(accountID)
               << ":\n"
               << "  * Owned by nym: " << String(nymID) << "\n"
               << "  * On server: " << String(serverID) << std::endl;
        auto& queue = get_operations({nymID, serverID});
        const auto taskID(Identifier::Random());
        queue.download_account_.Push(taskID, accountID);
    }

    otInfo << OT_METHOD << __FUNCTION__ << ": End" << std::endl;
}

void Sync::refresh_contacts() const
{
    for (const auto& it : contacts_.ContactList()) {
        SHUTDOWN()

        const auto& contactID = it.first;
        otInfo << OT_METHOD << __FUNCTION__
               << ": Considering contact: " << contactID << std::endl;
        const auto contact = contacts_.Contact(Identifier(contactID));

        OT_ASSERT(contact);

        const auto now = std::time(nullptr);
        const std::chrono::seconds interval(now - contact->LastUpdated());
        const std::chrono::hours limit(24 * CONTACT_REFRESH_DAYS);
        const auto nymList = contact->Nyms();

        if (nymList.empty()) {
            otInfo << OT_METHOD << __FUNCTION__
                   << ": No nyms associated with this contact." << std::endl;

            continue;
        }

        for (const auto& nymID : nymList) {
            SHUTDOWN()

            const auto nym = wallet_.Nym(nymID);
            otInfo << OT_METHOD << __FUNCTION__
                   << ": Considering nym: " << String(nymID) << std::endl;

            if (nym) {
                contacts_.Update(nym->asPublicNym());
            } else {
                otInfo << OT_METHOD << __FUNCTION__
                       << ": We don't have credentials for this nym. "
                       << " Will search on all servers." << std::endl;
                const auto taskID(Identifier::Random());
                missing_nyms_.Push(taskID, nymID);

                continue;
            }

            if (interval > limit) {
                otInfo << OT_METHOD << __FUNCTION__
                       << ": Hours since last update (" << interval.count()
                       << ") exceeds the limit (" << limit.count() << ")"
                       << std::endl;
                // TODO add a method to Contact that returns the list of
                // servers
                const auto data = contact->Data();

                if (false == bool(data)) {

                    continue;
                }

                const auto serverGroup = data->Group(
                    proto::CONTACTSECTION_COMMUNICATION,
                    proto::CITEMTYPE_OPENTXS);

                if (false == bool(serverGroup)) {

                    const auto taskID(Identifier::Random());
                    missing_nyms_.Push(taskID, nymID);
                    continue;
                }

                for (const auto & [ claimID, item ] : *serverGroup) {
                    SHUTDOWN()
                    OT_ASSERT(item)

                    const auto& notUsed[[maybe_unused]] = claimID;
                    const Identifier serverID(item->Value());

                    if (serverID.empty()) {

                        continue;
                    }

                    otInfo << OT_METHOD << __FUNCTION__
                           << ": Will download nym " << String(nymID)
                           << " from server " << String(serverID) << std::endl;
                    auto& serverQueue = get_nym_fetch(serverID);
                    const auto taskID(Identifier::Random());
                    serverQueue.Push(taskID, nymID);
                }
            } else {
                otInfo << OT_METHOD << __FUNCTION__
                       << ": No need to update this nym." << std::endl;
            }
        }
    }
}

bool Sync::register_account(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID,
    const Identifier& unitID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())
    OT_ASSERT(false == unitID.empty())

    rLock lock(api_lock_);
    auto action = server_action_.RegisterAccount(nymID, serverID, unitID);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            api_.Pair().Update();

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__
                  << ": Failed to register account for " << String(unitID)
                  << " on server " << String(serverID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while registering account "
              << " on server " << String(serverID) << std::endl;
    }

    return finish_task(taskID, false);
}

bool Sync::register_nym(
    const Identifier& taskID,
    const Identifier& nymID,
    const Identifier& serverID) const
{
    OT_ASSERT(false == nymID.empty())
    OT_ASSERT(false == serverID.empty())

    set_contact(nymID, serverID);
    rLock lock(api_lock_);
    auto action = server_action_.RegisterNym(nymID, serverID);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            api_.Pair().Update();

            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Server "
                  << String(serverID) << " did not accept registration for nym "
                  << String(nymID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while registering nym " << String(nymID)
              << " on server " << String(serverID) << std::endl;
    }

    return finish_task(taskID, false);
}

Identifier Sync::RegisterNym(
    const Identifier& nymID,
    const Identifier& serverID,
    const bool setContactData) const
{
    CHECK_SERVER(nymID, serverID)

    start_introduction_server(nymID);

    if (setContactData) {
        publish_server_registration(nymID, serverID, false);
    }

    return ScheduleRegisterNym(nymID, serverID);
}

Identifier Sync::SetIntroductionServer(const ServerContract& contract) const
{
    Lock lock(introduction_server_lock_);

    return set_introduction_server(lock, contract);
}

Identifier Sync::schedule_download_nymbox(
    const Identifier& localNymID,
    const Identifier& serverID) const
{
    CHECK_SERVER(localNymID, serverID)

    start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(taskID, queue.download_nymbox_.Push(taskID, true));
}

Identifier Sync::schedule_register_account(
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& unitID) const
{
    CHECK_ARGS(localNymID, serverID, unitID)

    start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(taskID, queue.register_account_.Push(taskID, unitID));
}

Identifier Sync::ScheduleDownloadAccount(
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& accountID) const
{
    CHECK_ARGS(localNymID, serverID, accountID)

    start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(taskID, queue.download_account_.Push(taskID, accountID));
}

Identifier Sync::ScheduleDownloadContract(
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& contractID) const
{
    CHECK_ARGS(localNymID, serverID, contractID)

    start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(
        taskID, queue.download_contract_.Push(taskID, contractID));
}

Identifier Sync::ScheduleDownloadNym(
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& targetNymID) const
{
    CHECK_ARGS(localNymID, serverID, targetNymID)

    start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(taskID, queue.check_nym_.Push(taskID, targetNymID));
}

Identifier Sync::ScheduleDownloadNymbox(
    const Identifier& localNymID,
    const Identifier& serverID) const
{
    return schedule_download_nymbox(localNymID, serverID);
}

Identifier Sync::ScheduleRegisterAccount(
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& unitID) const
{
    return schedule_register_account(localNymID, serverID, unitID);
}

Identifier Sync::ScheduleRegisterNym(
    const Identifier& localNymID,
    const Identifier& serverID) const
{
    CHECK_SERVER(localNymID, serverID)

    start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(taskID, queue.register_nym_.Push(taskID, true));
}

bool Sync::send_transfer(
    const Identifier& taskID,
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& sourceAccountID,
    const Identifier& targetAccountID,
    const int64_t value,
    const std::string& memo) const
{
    rLock lock(api_lock_);
    auto action = server_action_.SendTransfer(
        localNymID, serverID, sourceAccountID, targetAccountID, value, memo);
    action->Run();
    lock.unlock();

    if (SendResult::VALID_REPLY == action->LastSendResult()) {
        OT_ASSERT(action->Reply());

        if (action->Reply()->m_bSuccess) {
            return finish_task(taskID, true);
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Failed to send transfer "
                  << "to " << String(serverID) << " for account "
                  << String(targetAccountID) << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Communication error while sending transfer to account "
              << String(targetAccountID) << " on server " << String(serverID)
              << std::endl;
    }

    return finish_task(taskID, false);
}

Identifier Sync::SendTransfer(
    const Identifier& localNymID,
    const Identifier& serverID,
    const Identifier& sourceAccountID,
    const Identifier& targetAccountID,
    const int64_t value,
    const std::string& memo) const
{
    CHECK_ARGS(localNymID, serverID, targetAccountID)
    CHECK_NYM(sourceAccountID)

    auto sourceAccount = ot_api_.GetWallet()->GetAccount(sourceAccountID);
    if (false == bool(sourceAccount)) {
        otErr << OT_METHOD << __FUNCTION__ << ": Invalid source account"
              << std::endl;

        return {};
    }
    auto targetAccount = ot_api_.GetWallet()->GetAccount(targetAccountID);
    if (false == bool(targetAccount)) {
        otErr << OT_METHOD << __FUNCTION__ << ": Invalid target account"
              << std::endl;

        return {};
    }
    if (sourceAccount->GetNymID() != targetAccount->GetNymID()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Source and target account"
              << " owner ids don't match" << std::endl;

        return {};
    }
    if (sourceAccount->GetRealNotaryID() != targetAccount->GetRealNotaryID()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Source and target account"
              << " notary ids don't match" << std::endl;

        return {};
    }
    if (sourceAccount->GetInstrumentDefinitionID() !=
        targetAccount->GetInstrumentDefinitionID()) {
        otErr << OT_METHOD << __FUNCTION__ << ": Source and target account"
              << " instrument definition ids don't match" << std::endl;

        return {};
    }

    // start_introduction_server(localNymID);
    auto& queue = get_operations({localNymID, serverID});
    const auto taskID(Identifier::Random());

    return start_task(
        taskID,
        queue.send_transfer_.Push(
            taskID, {sourceAccountID, targetAccountID, value, memo}));
}

void Sync::set_contact(const Identifier& nymID, const Identifier& serverID)
    const
{
    auto nym = wallet_.mutable_Nym(nymID);
    const auto server = nym.PreferredOTServer();

    if (server.empty()) {
        nym.AddPreferredOTServer(serverID.str(), true);
    }
}

Identifier Sync::set_introduction_server(
    const Lock& lock,
    const ServerContract& contract) const
{
    OT_ASSERT(verify_lock(lock, introduction_server_lock_));

    auto instantiated = wallet_.Server(contract.PublicContract());

    if (false == bool(instantiated)) {

        return {};
    }

    const auto& id = instantiated->ID();
    introduction_server_id_.reset(new Identifier(id));

    OT_ASSERT(introduction_server_id_)
    bool dontCare = false;
    rLock apiLock(api_lock_);
    const bool set = config_.Set_str(
        MASTER_SECTION, INTRODUCTION_SERVER_KEY, String(id), dontCare);

    OT_ASSERT(set)
    config_.Save();
    apiLock.unlock();

    return id;
}

void Sync::start_introduction_server(const Identifier& nymID) const
{
    auto& serverID = IntroductionServer();

    if (serverID.empty()) {

        return;
    }

    auto& queue = get_operations({nymID, serverID});
    const auto taskID(Identifier::Random());
    start_task(taskID, queue.download_nymbox_.Push(taskID, true));
}

Identifier Sync::start_task(const Identifier& taskID, bool success) const
{
    if (taskID.empty()) {

        return {};
    }

    if (false == success) {

        return {};
    }

    add_task(taskID, ThreadStatus::RUNNING);

    return taskID;
}

void Sync::StartIntroductionServer(const Identifier& localNymID) const
{
    start_introduction_server(localNymID);
}

void Sync::state_machine(const ContextID id, OperationQueue& queue) const
{
    const auto & [ nymID, serverID ] = id;

    // Make sure the server contract is available
    while (running_) {
        if (check_server_contract(serverID)) {
            otInfo << OT_METHOD << __FUNCTION__ << ": Server contract "
                   << String(serverID) << " exists." << std::endl;

            break;
        }

        YIELD(CONTRACT_DOWNLOAD_SECONDS);
    }

    SHUTDOWN()

    std::shared_ptr<const ServerContext> context{nullptr};

    // Make sure the nym has registered for the first time on the server
    while (running_) {
        if (check_registration(nymID, serverID, context)) {
            otInfo << OT_METHOD << __FUNCTION__ << ": Nym " << String(nymID)
                   << " has registered on server " << String(serverID)
                   << " at least once." << std::endl;

            break;
        }

        YIELD(NYM_REGISTRATION_SECONDS);
    }

    SHUTDOWN()
    OT_ASSERT(context)

    bool queueValue{false};
    bool needAdmin{false};
    bool registerNym{false};
    bool registerNymQueued{false};
    bool downloadNymbox{false};
    auto taskID = Identifier::Factory();
    Identifier accountID{};
    Identifier unitID{};
    Identifier contractID{};
    Identifier targetNymID{};
    Identifier nullID{};
    OTPassword serverPassword;
    MessageTask message;
    PaymentTask payment;
#if OT_CASH
    PayCashTask cash_payment;
#endif  // OT_CASH
    DepositPaymentTask deposit;
    UniqueQueue<DepositPaymentTask> depositPaymentRetry;
    SendTransferTask transfer;

    // Primary loop
    while (running_) {
        SHUTDOWN()

        // If the local nym has updated since the last registernym operation,
        // schedule a registernym
        check_nym_revision(*context, queue);

        SHUTDOWN()

        // Register the nym, if scheduled. Keep trying until success
        registerNymQueued = queue.register_nym_.Pop(taskID, queueValue);
        registerNym |= queueValue;

        if (registerNymQueued || registerNym) {
            if (register_nym(taskID, nymID, serverID)) {
                registerNym = false;
                queueValue = false;
            } else {
                registerNym = true;
            }
        }

        SHUTDOWN()

        // If this server was added by a pairing operation that included
        // a server password then request admin permissions on the server
        needAdmin =
            context->HaveAdminPassword() && (false == context->isAdmin());

        if (needAdmin) {
            serverPassword.setPassword(context->AdminPassword());
            get_admin(nymID, serverID, serverPassword);
        }

        SHUTDOWN()

        // This is a list of servers for which we do not have a contract.
        // We ask all known servers on which we are registered to try to find
        // the contracts.
        const auto servers = missing_servers_.Copy();

        for (const auto & [ targetID, taskID ] : servers) {
            SHUTDOWN()

            if (targetID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty serverID get in here?"
                      << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__
                       << ": Searching for server contract for "
                       << String(targetID) << std::endl;
            }

            const auto& notUsed[[maybe_unused]] = taskID;
            find_server(nymID, serverID, targetID);
        }

        // This is a list of contracts (server and unit definition) which a
        // user of this class has requested we download from this server.
        while (queue.download_contract_.Pop(taskID, contractID)) {
            SHUTDOWN()

            if (contractID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty contract ID get in here?"
                      << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__
                       << ": Searching for unit definition contract for "
                       << String(contractID) << std::endl;
            }

            download_contract(taskID, nymID, serverID, contractID);
        }

        // This is a list of nyms for which we do not have credentials..
        // We ask all known servers on which we are registered to try to find
        // their credentials.
        const auto nyms = missing_nyms_.Copy();

        for (const auto & [ targetID, taskID ] : nyms) {
            SHUTDOWN()

            if (targetID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty nymID get in here?" << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__ << ": Searching for nym "
                       << String(targetID) << std::endl;
            }

            const auto& notUsed[[maybe_unused]] = taskID;
            find_nym(nymID, serverID, targetID);
        }

        // This is a list of nyms which haven't been updated in a while and
        // are known or suspected to be available on this server
        auto& nymQueue = get_nym_fetch(serverID);

        while (nymQueue.Pop(taskID, targetNymID)) {
            SHUTDOWN()

            if (targetNymID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty nymID get in here?" << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__ << ": Refreshing nym "
                       << String(targetNymID) << std::endl;
            }

            download_nym(taskID, nymID, serverID, targetNymID);
        }

        // This is a list of nyms which a user of this class has requested we
        // download from this server.
        while (queue.check_nym_.Pop(taskID, targetNymID)) {
            SHUTDOWN()

            if (targetNymID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty nymID get in here?" << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__ << ": Searching for nym "
                       << String(targetNymID) << std::endl;
            }

            download_nym(taskID, nymID, serverID, targetNymID);
        }

        // This is a list of messages which need to be delivered to a nym
        // on this server
        while (queue.send_message_.Pop(taskID, message)) {
            SHUTDOWN()

            const auto & [ recipientID, text ] = message;

            if (recipientID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty recipient nymID get in here?"
                      << std::endl;

                continue;
            }

            message_nym(taskID, nymID, serverID, recipientID, text);
        }

        // This is a list of payments which need to be delivered to a nym
        // on this server
        while (queue.send_payment_.Pop(taskID, payment)) {
            SHUTDOWN()

            auto & [ recipientID, pPayment ] = payment;

            if (recipientID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty recipient nymID get in here?"
                      << std::endl;

                continue;
            }

            pay_nym(taskID, nymID, serverID, recipientID, pPayment);
        }

#if OT_CASH
        // This is a list of cash payments which need to be delivered to a nym
        // on this server
        while (queue.send_cash_.Pop(taskID, cash_payment)) {
            SHUTDOWN()

            auto & [ recipientID, pRecipientPurse, pSenderPurse ] =
                cash_payment;

            if (recipientID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty recipient nymID get in here?"
                      << std::endl;

                continue;
            }

            pay_nym_cash(
                taskID,
                nymID,
                serverID,
                recipientID,
                pRecipientPurse,
                pSenderPurse);
        }
#endif

        // Download the nymbox, if this operation has been scheduled
        if (queue.download_nymbox_.Pop(taskID, downloadNymbox)) {
            otWarn << OT_METHOD << __FUNCTION__ << ": Downloading nymbox for "
                   << String(nymID) << " on " << String(serverID) << std::endl;
            registerNym |= !download_nymbox(taskID, nymID, serverID);
        }

        SHUTDOWN()

        // Download any accounts which have been scheduled for download
        while (queue.download_account_.Pop(taskID, accountID)) {
            SHUTDOWN()

            if (accountID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty account ID get in here?"
                      << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__ << ": Downloading account "
                       << String(accountID) << " for " << String(nymID)
                       << " on " << String(serverID) << std::endl;
            }

            registerNym |=
                !download_account(taskID, nymID, serverID, accountID);
        }

        SHUTDOWN()

        // Register any accounts which have been scheduled for creation
        while (queue.register_account_.Pop(taskID, unitID)) {
            SHUTDOWN()

            if (unitID.empty()) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": How did an empty unit ID get in here?" << std::endl;

                continue;
            } else {
                otWarn << OT_METHOD << __FUNCTION__ << ": Creating account for "
                       << String(unitID) << " on " << String(serverID)
                       << std::endl;
            }

            registerNym |= !register_account(taskID, nymID, serverID, unitID);
        }

        SHUTDOWN()

        // Deposit any queued payments
        while (queue.deposit_payment_.Pop(taskID, deposit)) {
            auto & [ accountIDHint, payment ] = deposit;

            SHUTDOWN()
            OT_ASSERT(payment)

            const auto status =
                can_deposit(*payment, nymID, accountIDHint, nullID, accountID);

            switch (status) {
                case Depositability::READY: {
                    registerNym |= !deposit_cheque(
                        taskID,
                        nymID,
                        serverID,
                        accountID,
                        payment,
                        depositPaymentRetry);
                } break;
                case Depositability::NOT_REGISTERED:
                case Depositability::NO_ACCOUNT: {
                    otWarn << OT_METHOD << __FUNCTION__
                           << ": Temporary failure trying to deposit payment"
                           << std::endl;
                    depositPaymentRetry.Push(taskID, deposit);
                } break;
                default: {
                    otErr << OT_METHOD << __FUNCTION__
                          << ": Permanent failure trying to deposit payment"
                          << std::endl;
                }
            }
        }

        // Requeue all payments which will be retried
        while (depositPaymentRetry.Pop(taskID, deposit)) {
            SHUTDOWN()

            queue.deposit_payment_.Push(taskID, deposit);
        }

        SHUTDOWN()

        // This is a list of transfers which need to be delivered to a nym
        // on this server
        while (queue.send_transfer_.Pop(taskID, transfer)) {
            SHUTDOWN()

            const auto & [ sourceAccountID, targetAccountID, value, memo ] =
                transfer;

            send_transfer(
                taskID,
                nymID,
                serverID,
                sourceAccountID,
                targetAccountID,
                value,
                memo);
        }

        SHUTDOWN()

        YIELD(MAIN_LOOP_SECONDS);
    }
}

ThreadStatus Sync::status(const Lock& lock, const Identifier& taskID) const
{
    OT_ASSERT(verify_lock(lock, task_status_lock_))

    if (!running_) {

        return ThreadStatus::SHUTDOWN;
    }

    auto it = task_status_.find(taskID);

    if (task_status_.end() == it) {

        return ThreadStatus::ERROR;
    }

    const auto output = it->second;
    const bool success = (ThreadStatus::FINISHED_SUCCESS == output);
    const bool failed = (ThreadStatus::FINISHED_FAILED == output);
    const bool finished = (success || failed);

    if (finished) {
        task_status_.erase(it);
    }

    return output;
}

ThreadStatus Sync::Status(const Identifier& taskID) const
{
    Lock lock(task_status_lock_);

    return status(lock, taskID);
}

void Sync::update_task(const Identifier& taskID, const ThreadStatus status)
    const
{
    if (taskID.empty()) {

        return;
    }

    Lock lock(task_status_lock_);

    if (0 == task_status_.count(taskID)) {

        return;
    }

    task_status_[taskID] = status;
}

Depositability Sync::valid_account(
    const OTPayment& payment,
    const Identifier& recipient,
    const Identifier& paymentServerID,
    const Identifier& paymentUnitID,
    const Identifier& accountIDHint,
    Identifier& depositAccount) const
{
    const auto accounts = ot_api_.Accounts();
    std::set<Identifier> matchingAccounts{};

    for (const auto & [ accountID, nymID, serverID, unitID ] : accounts) {
        if (nymID != recipient) {

            continue;
        }

        if (serverID != paymentServerID) {

            continue;
        }

        if (unitID != paymentUnitID) {

            continue;
        }

        matchingAccounts.emplace(accountID);
    }

    if (accountIDHint.empty()) {
        if (0 == matchingAccounts.size()) {

            return Depositability::NO_ACCOUNT;
        } else if (1 == matchingAccounts.size()) {
            depositAccount = *matchingAccounts.begin();

            return Depositability::READY;
        } else {

            return Depositability::ACCOUNT_NOT_SPECIFIED;
        }
    }

    if (0 == matchingAccounts.size()) {

        return Depositability::NO_ACCOUNT;
    } else if (1 == matchingAccounts.count(accountIDHint)) {
        depositAccount = accountIDHint;

        return Depositability::READY;
    } else {

        return Depositability::WRONG_ACCOUNT;
    }
}

Depositability Sync::valid_recipient(
    const OTPayment& payment,
    const Identifier& specified,
    const Identifier& recipient) const
{
    if (specified.empty()) {
        otErr << OT_METHOD << __FUNCTION__
              << ": Payment can be accepted by any nym" << std::endl;

        return Depositability::READY;
    }

    if (recipient == specified) {

        return Depositability::READY;
    }

    return Depositability::WRONG_RECIPIENT;
}

Sync::~Sync()
{
    for (auto & [ id, thread ] : state_machines_) {
        const auto& notUsed[[maybe_unused]] = id;

        OT_ASSERT(thread)

        if (thread->joinable()) {
            thread->join();
        }
    }
}
}  // namespace opentxs::api::implementation
