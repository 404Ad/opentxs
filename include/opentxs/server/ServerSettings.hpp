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

#ifndef OPENTXS_SERVER_SERVERSETTINGS_HPP
#define OPENTXS_SERVER_SERVERSETTINGS_HPP

#include "opentxs/Forward.hpp"

#include <cstdint>
#include <string>

namespace opentxs
{
namespace server
{

struct ServerSettings {
    static std::int64_t GetMinMarketScale() { return __min_market_scale; }

    static void SetMinMarketScale(int64_t value) { __min_market_scale = value; }

    static std::int32_t GetHeartbeatNoRequests()
    {
        return __heartbeat_no_requests;
    }

    static void SetHeartbeatNoRequests(int32_t value)
    {
        __heartbeat_no_requests = value;
    }

    static std::int32_t GetHeartbeatMsBetweenBeats()
    {
        return __heartbeat_ms_between_beats;
    }

    static void SetHeartbeatMsBetweenBeats(int32_t value)
    {
        __heartbeat_ms_between_beats = value;
    }

    static const std::string& GetOverrideNymID() { return __override_nym_id; }

    static void SetOverrideNymID(const std::string& id)
    {
        __override_nym_id = id;
    }

    static std::int64_t __min_market_scale;

    static std::int32_t __heartbeat_no_requests;
    static std::int32_t __heartbeat_ms_between_beats;

    // The Nym who's allowed to do certain commands even if they are turned off.
    static std::string __override_nym_id;
    // Are usage credits REQUIRED in order to use this server?
    static bool __admin_usage_credits;
    // Is server currently locked to non-override Nyms?
    static bool __admin_server_locked;

    static bool __cmd_usage_credits;
    static bool __cmd_issue_asset;
    static bool __cmd_get_contract;
    static bool __cmd_check_notary_id;

    static bool __cmd_create_user_acct;
    static bool __cmd_del_user_acct;
    static bool __cmd_check_nym;
    static bool __cmd_get_requestnumber;
    static bool __cmd_get_trans_nums;
    static bool __cmd_send_message;
    static bool __cmd_get_nymbox;
    static bool __cmd_process_nymbox;

    static bool __cmd_create_asset_acct;
    static bool __cmd_del_asset_acct;
    static bool __cmd_get_acct;
    static bool __cmd_get_inbox;
    static bool __cmd_get_outbox;
    static bool __cmd_process_inbox;

    static bool __cmd_issue_basket;
    static bool __transact_exchange_basket;

    static bool __cmd_notarize_transaction;
    static bool __transact_process_inbox;
    static bool __transact_transfer;
    static bool __transact_withdrawal;
    static bool __transact_deposit;
    static bool __transact_withdraw_voucher;
    static bool __transact_deposit_cheque;
    static bool __transact_pay_dividend;

    static bool __cmd_get_mint;
    static bool __transact_withdraw_cash;
    static bool __transact_deposit_cash;

    static bool __cmd_get_market_list;
    static bool __cmd_get_market_offers;
    static bool __cmd_get_market_recent_trades;
    static bool __cmd_get_nym_market_offers;

    static bool __transact_market_offer;
    static bool __transact_payment_plan;
    static bool __transact_cancel_cron_item;
    static bool __transact_smart_contract;
    static bool __cmd_trigger_clause;

    static bool __cmd_register_contract;

    static bool __cmd_request_admin;
};
}  // namespace server
}  // namespace opentxs

#endif  // OPENTXS_SERVER_SERVERSETTINGS_HPP
