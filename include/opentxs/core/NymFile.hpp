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

#ifndef OPENTXS_CORE_OTNYMFILE_HPP
#define OPENTXS_CORE_OTNYMFILE_HPP

namespace opentxs
{
typedef proto::CredentialIndex serializedCredentialIndex;

class NymFile
{
public:
    // Whenever a Nym sends a payment, a copy is dropped std::into his
    // Outpayments.
    // (Payments screen.)
    // A payments message is the original OTMessage that this Nym sent.
    EXPORT virtual void AddOutpayments(Message& theMessage) = 0;
    EXPORT virtual bool CompareID(const Identifier& theIdentifier) const = 0;
    EXPORT virtual void DisplayStatistics(String& strOutput) const = 0;
    EXPORT virtual bool GetInboxHash(
        const std::string& acct_id,
        Identifier& theOutput) const = 0;  // client-side
    EXPORT virtual bool GetOutboxHash(
        const std::string& acct_id,
        Identifier& theOutput) const = 0;  // client-side
    EXPORT virtual Message* GetOutpaymentsByIndex(
        const std::int32_t nIndex) const = 0;
    EXPORT virtual Message* GetOutpaymentsByTransNum(
        const std::int64_t lTransNum,
        std::unique_ptr<OTPayment>* pReturnPayment = nullptr,
        std::int32_t* pnReturnIndex = nullptr) const = 0;
    EXPORT virtual std::int32_t GetOutpaymentsCount() const = 0;
    EXPORT virtual std::set<std::string>& GetSetAssetAccounts() = 0;
    EXPORT virtual const std::int64_t& GetUsageCredits() const = 0;
    EXPORT virtual const Identifier& ID() const = 0;
    EXPORT virtual std::string PaymentCode() const = 0;
    // IMPORTANT NOTE: Not all outpayments have a transaction num!
    // Imagine if you sent a cash purse to someone, for example.
    // The cash withdrawal had a transNum, and the eventual cash
    // deposit will have a transNum, but the purse itself does NOT.
    // That's okay in your outpayments box since it's like an outmail
    // box. It's not a ledger, so the items inside don't need a txn#.
    EXPORT virtual bool RemoveOutpaymentsByIndex(
        const std::int32_t nIndex,
        bool bDeleteIt = true) = 0;
    EXPORT virtual bool RemoveOutpaymentsByTransNum(
        const std::int64_t lTransNum,
        bool bDeleteIt = true) = 0;
    // ** ResyncWithServer **
    //
    // Not for normal use! (Since you should never get out of sync with the
    // server in the first place.)
    // However, in testing, or if some bug messes up some data, or whatever, and
    // you absolutely need to
    // re-sync with a server, and you trust that server not to lie to you, then
    // this function will do the trick.
    // NOTE: Before calling this, you need to do a getNymbox() to download the
    // latest Nymbox, and you need to do
    // a registerNym() to download the server's copy of your Nym. You then
    // need to load that Nymbox from
    // local storage, and you need to load the server's message Nym out of the
    // registerNymResponse reply, so that
    // you can pass both of those objects std::into this function, which must
    // assume
    // that those pieces were already done
    // just prior to this call.
    EXPORT virtual bool ResyncWithServer(
        const Ledger& theNymbox,
        const Nym& theMessageNym) = 0;
    EXPORT virtual bool SetInboxHash(
        const std::string& acct_id,
        const Identifier& theInput) = 0;  // client-side
    EXPORT virtual bool SetOutboxHash(
        const std::string& acct_id,
        const Identifier& theInput) = 0;  // client-side
    EXPORT virtual void SetUsageCredits(const std::int64_t& lUsage) = 0;

    EXPORT virtual ~NymFile() = default;
};
}  // namespace opentxs

#endif  // OPENTXS_CORE_OTNYMFILE_HPP
