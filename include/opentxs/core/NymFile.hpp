// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_NYMFILE_HPP
#define OPENTXS_CORE_NYMFILE_HPP

namespace opentxs
{
typedef proto::CredentialIndex serializedCredentialIndex;

class NymFile
{
public:
    EXPORT virtual bool CompareID(const Identifier& theIdentifier) const = 0;
    EXPORT virtual void DisplayStatistics(String& strOutput) const = 0;
    EXPORT virtual bool GetInboxHash(
        const std::string& acct_id,
        Identifier& theOutput) const = 0;  // client-side
    EXPORT virtual bool GetOutboxHash(
        const std::string& acct_id,
        Identifier& theOutput) const = 0;  // client-side
    EXPORT virtual std::shared_ptr<Message> GetOutpaymentsByIndex(
        const std::int32_t nIndex) const = 0;
    EXPORT virtual std::shared_ptr<Message> GetOutpaymentsByTransNum(
        const std::int64_t lTransNum,
        std::unique_ptr<OTPayment>* pReturnPayment = nullptr,
        std::int32_t* pnReturnIndex = nullptr) const = 0;
    EXPORT virtual std::int32_t GetOutpaymentsCount() const = 0;
    EXPORT virtual const std::int64_t& GetUsageCredits() const = 0;
    EXPORT virtual const Identifier& ID() const = 0;
    EXPORT virtual std::string PaymentCode() const = 0;
    EXPORT virtual bool SerializeNymFile(String& output) const = 0;

    // Whenever a Nym sends a payment, a copy is dropped std::into his
    // Outpayments. (Payments screen.) A payments message is the original
    // OTMessage that this Nym sent.
    EXPORT virtual void AddOutpayments(std::shared_ptr<Message> theMessage) = 0;
    // IMPORTANT NOTE: Not all outpayments have a transaction num!
    // Imagine if you sent a cash purse to someone, for example.
    // The cash withdrawal had a transNum, and the eventual cash
    // deposit will have a transNum, but the purse itself does NOT.
    // That's okay in your outpayments box since it's like an outmail
    // box. It's not a ledger, so the items inside don't need a txn#.
    EXPORT virtual std::set<std::string>& GetSetAssetAccounts() = 0;
    EXPORT virtual bool RemoveOutpaymentsByIndex(const std::int32_t nIndex) = 0;
    EXPORT virtual bool RemoveOutpaymentsByTransNum(
        const std::int64_t lTransNum) = 0;
    EXPORT virtual bool SetInboxHash(
        const std::string& acct_id,
        const Identifier& theInput) = 0;  // client-side
    EXPORT virtual bool SetOutboxHash(
        const std::string& acct_id,
        const Identifier& theInput) = 0;  // client-side
    EXPORT virtual void SetUsageCredits(const std::int64_t& lUsage) = 0;

    EXPORT virtual ~NymFile() = default;

protected:
    NymFile() = default;

private:
    NymFile(const NymFile&) = delete;
    NymFile(NymFile&&) = delete;
    NymFile& operator=(const NymFile&) = delete;
    NymFile& operator=(NymFile&&) = delete;
};
}  // namespace opentxs
#endif
