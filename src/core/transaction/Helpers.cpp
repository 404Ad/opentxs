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

#include "stdafx.hpp"

#include "opentxs/core/transaction/Helpers.hpp"

#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/util/OTFolders.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/NumList.hpp"
#include "opentxs/core/OTStorage.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/OTTransactionType.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/Types.hpp"

#include <irrxml/irrXML.hpp>

#include <cinttypes>
#include <cstdint>
#include <ostream>
#include <string>

namespace
{

// NOTE: The below strings correspond to the transaction types
// listed near the top of OTTransaction.hpp as enum transactionType.
char const* const TypeStrings[] = {
    "blank",    // freshly issued, not used yet  // comes from server, stored on
                // Nym. (Nymbox.)
    "message",  // in nymbox, message from one user to another.
    "notice",   // in nymbox, notice from the server. Probably contains an
                // updated
                // smart contract.
    "replyNotice",  // When you send a request to the server, sometimes its
                    // reply
                    // is so important,
    // that it drops a copy into your Nymbox to make you receive and process it.
    "successNotice",    // A transaction # has successfully been signed out.
                        // (Nymbox.)
    "pending",          // Pending transfer, in the inbox/outbox.
    "transferReceipt",  // the server drops this into your inbox, when someone
                        // accepts your transfer.
    "chequeReceipt",    // the server drops this into your inbox, when someone
                        // deposits your cheque.
    "voucherReceipt",   // the server drops this into your inbox, when someone
                        // deposits your voucher.
    "marketReceipt",   // server drops this into inbox periodically, if you have
                       // an offer on the market.
    "paymentReceipt",  // the server drops this into people's inboxes,
                       // periodically, if they have payment plans.
    "finalReceipt",    // the server drops this into your inbox(es), when a
                       // CronItem expires or is canceled.
    "basketReceipt",   // the server drops this into your inboxes, when a basket
                       // exchange is processed.
    "instrumentNotice",  // Receive these in paymentInbox (by way of Nymbox),
                         // and
                         // send in Outpayments (like outMail.) (When done, they
                         // go to recordBox or expiredBox to await deletion.)
    "instrumentRejection",  // When someone rejects your invoice from his
                            // paymentInbox, you get one of these in YOUR
                            // paymentInbox.
    "processNymbox",    // process nymbox transaction     // comes from client
    "atProcessNymbox",  // process nymbox reply             // comes from server
    "processInbox",     // process inbox transaction     // comes from client
    "atProcessInbox",   // process inbox reply             // comes from server
    "transfer",  // or "spend". This transaction is a transfer from one account
                 // to another
    "atTransfer",  // reply from the server regarding a transfer request
    "deposit",  // this transaction is a deposit of bearer tokens (from client)
    "atDeposit",         // reply from the server regarding a deposit request
    "withdrawal",        // this transaction is a withdrawal of bearer tokens
    "atWithdrawal",      // reply from the server regarding a withdrawal request
    "marketOffer",       // this transaction is a market offer
    "atMarketOffer",     // reply from the server regarding a market offer
    "paymentPlan",       // this transaction is a payment plan
    "atPaymentPlan",     // reply from the server regarding a payment plan
    "smartContract",     // this transaction is a smart contract
    "atSmartContract",   // reply from the server regarding a smart contract
    "cancelCronItem",    // this transaction is a cancellation of a cron item
                         // (payment plan etc)
    "atCancelCronItem",  // reply from the server regarding said cancellation.
    "exchangeBasket",    // this transaction is an exchange in/out of a basket
                         // currency.
    "atExchangeBasket",  // reply from the server regarding said exchange.
    "payDividend",       // this transaction is a dividend payment (to the
                         // shareholders.)
    "atPayDividend",  // reply from the server regarding said dividend payment.
    "error_state"};

char const* const OriginTypeStrings[] = {
    "not_applicable",
    "origin_market_offer",    // finalReceipt
    "origin_payment_plan",    // finalReceipt, paymentReceipt
    "origin_smart_contract",  // finalReceipt, paymentReceipt
    "origin_pay_dividend",    // SOME voucher receipts are from a payDividend.
    "origin_error_state"};

}  // namespace

namespace opentxs
{

const char* GetTransactionTypeString(
    int transactionTypeIndex)  // enum transactionType
{
    return TypeStrings[transactionTypeIndex];
}

const char* GetOriginTypeToString(int originTypeIndex)  // enum originType
{
    return OriginTypeStrings[originTypeIndex];
}

// Returns 1 if success, -1 if error.
std::int32_t LoadAbbreviatedRecord(
    irr::io::IrrXMLReader*& xml,
    std::int64_t& lNumberOfOrigin,
    int& theOriginType,
    std::int64_t& lTransactionNum,
    std::int64_t& lInRefTo,
    std::int64_t& lInRefDisplay,
    time64_t& the_DATE_SIGNED,
    int& theType,
    String& strHash,
    std::int64_t& lAdjustment,
    std::int64_t& lDisplayValue,
    std::int64_t& lClosingNum,
    std::int64_t& lRequestNum,
    bool& bReplyTransSuccess,
    NumList* pNumList)
{

    const String strOriginNum = xml->getAttributeValue("numberOfOrigin");
    const String strOriginType = xml->getAttributeValue("originType");
    const String strTransNum = xml->getAttributeValue("transactionNum");
    const String strInRefTo = xml->getAttributeValue("inReferenceTo");
    const String strInRefDisplay = xml->getAttributeValue("inRefDisplay");
    const String strDateSigned = xml->getAttributeValue("dateSigned");

    if (!strTransNum.Exists() || !strInRefTo.Exists() ||
        !strInRefDisplay.Exists() || !strDateSigned.Exists()) {
        otOut << "LoadAbbreviatedRecord: Failure: missing "
                 "strTransNum ("
              << strTransNum << ") or strInRefTo (" << strInRefTo
              << ") or strInRefDisplay (" << strInRefDisplay
              << ") or strDateSigned(" << strDateSigned
              << ") while loading abbreviated receipt. \n";
        return (-1);
    }
    lTransactionNum = strTransNum.ToLong();
    lInRefTo = strInRefTo.ToLong();
    lInRefDisplay = strInRefDisplay.ToLong();

    if (strOriginNum.Exists()) lNumberOfOrigin = strOriginNum.ToLong();
    if (strOriginType.Exists())
        theOriginType = static_cast<int>(
            OTTransactionType::GetOriginTypeFromString(strOriginType));

    the_DATE_SIGNED = parseTimestamp(strDateSigned.Get());

    // Transaction TYPE for the abbreviated record...
    theType = OTTransaction::error_state;  // default
    const String strAbbrevType =
        xml->getAttributeValue("type");  // the type of inbox receipt, or outbox
                                         // receipt, or nymbox receipt.
                                         // (Transaction type.)
    if (strAbbrevType.Exists()) {
        theType = OTTransaction::GetTypeFromString(strAbbrevType);

        if (OTTransaction::error_state == theType) {
            otErr << "LoadAbbreviatedRecord: Failure: "
                     "error_state was the found type (based on "
                     "string "
                  << strAbbrevType
                  << "), when loading abbreviated receipt for trans num: "
                  << lTransactionNum << " (In Reference To: " << lInRefTo
                  << ") \n";
            return (-1);
        }
    } else {
        otOut << "LoadAbbreviatedRecord: Failure: unknown "
                 "transaction type ("
              << strAbbrevType
              << ") when "
                 "loading abbreviated receipt for trans num: "
              << lTransactionNum << " (In Reference To: " << lInRefTo << ") \n";
        return (-1);
    }

    // RECEIPT HASH
    //
    strHash = xml->getAttributeValue("receiptHash");
    if (!strHash.Exists()) {
        otOut << "LoadAbbreviatedRecord: Failure: Expected "
                 "receiptHash while loading "
                 "abbreviated receipt for trans num: "
              << lTransactionNum << " (In Reference To: " << lInRefTo << ")\n";
        return (-1);
    }

    lAdjustment = 0;
    lDisplayValue = 0;
    lClosingNum = 0;

    const String strAbbrevAdjustment = xml->getAttributeValue("adjustment");
    if (strAbbrevAdjustment.Exists())
        lAdjustment = strAbbrevAdjustment.ToLong();
    // -------------------------------------
    const String strAbbrevDisplayValue = xml->getAttributeValue("displayValue");
    if (strAbbrevDisplayValue.Exists())
        lDisplayValue = strAbbrevDisplayValue.ToLong();

    if (OTTransaction::replyNotice == theType) {
        const String strRequestNum = xml->getAttributeValue("requestNumber");

        if (!strRequestNum.Exists()) {
            otOut << "LoadAbbreviatedRecord: Failed loading "
                     "abbreviated receipt: "
                     "expected requestNumber on replyNotice trans num: "
                  << lTransactionNum << " (In Reference To: " << lInRefTo
                  << ")\n";
            return (-1);
        }
        lRequestNum = strRequestNum.ToLong();

        const String strTransSuccess = xml->getAttributeValue("transSuccess");

        bReplyTransSuccess = strTransSuccess.Compare("true");
    }  // if replyNotice (expecting request Number)

    // If the transaction is a certain type, then it will also have a CLOSING
    // number.
    // (Grab that too.)
    //
    if ((OTTransaction::finalReceipt == theType) ||
        (OTTransaction::basketReceipt == theType)) {
        const String strAbbrevClosingNum = xml->getAttributeValue("closingNum");

        if (!strAbbrevClosingNum.Exists()) {
            otOut << "LoadAbbreviatedRecord: Failed loading "
                     "abbreviated receipt: "
                     "expected closingNum on trans num: "
                  << lTransactionNum << " (In Reference To: " << lInRefTo
                  << ")\n";
            return (-1);
        }
        lClosingNum = strAbbrevClosingNum.ToLong();
    }  // if finalReceipt or basketReceipt (expecting closing num)

    // These types carry their own internal list of numbers.
    //
    if ((nullptr != pNumList) && ((OTTransaction::blank == theType) ||
                                  (OTTransaction::successNotice == theType))) {
        const String strNumbers = xml->getAttributeValue("totalListOfNumbers");
        pNumList->Release();

        if (strNumbers.Exists()) pNumList->Add(strNumbers);
    }  // if blank or successNotice (expecting totalListOfNumbers.. no more
       // multiple blanks in the same ledger! They all go in a single
       // transaction.)

    return 1;
}

bool VerifyBoxReceiptExists(
    const Identifier& NOTARY_ID,
    const Identifier& NYM_ID,      // Unused here for now, but still convention.
    const Identifier& ACCOUNT_ID,  // If for Nymbox (vs inbox/outbox) then pass
                                   // NYM_ID in this field also.
    const std::int32_t nBoxType,   // 0/nymbox, 1/inbox, 2/outbox
    const std::int64_t& lTransactionNum)
{
    const std::int64_t lLedgerType = static_cast<int64_t>(nBoxType);

    const String strNotaryID(NOTARY_ID),
        strUserOrAcctID(0 == lLedgerType ? NYM_ID : ACCOUNT_ID);  // (For Nymbox
                                                                  // aka type 0,
                                                                  // the NymID
                                                                  // will be
                                                                  // here.)
    // --------------------------------------------------------------------
    String strFolder1name, strFolder2name, strFolder3name, strFilename;

    if (!SetupBoxReceiptFilename(
            lLedgerType,  // nBoxType is lLedgerType
            strUserOrAcctID,
            strNotaryID,
            lTransactionNum,
            "OTTransaction::VerifyBoxReceiptExists",
            strFolder1name,
            strFolder2name,
            strFolder3name,
            strFilename))
        return false;  // This already logs -- no need to log twice, here.
    // --------------------------------------------------------------------
    // See if the box receipt exists before trying to save over it...
    //
    const bool bExists = OTDB::Exists(
        strFolder1name.Get(),
        strFolder2name.Get(),
        strFolder3name.Get(),
        strFilename.Get());

    otWarn << "OTTransaction::"
           << (bExists ? "(Already have this one)"
                       : "(Need to download this one)")
           << ": " << __FUNCTION__ << ": " << strFolder1name
           << Log::PathSeparator() << strFolder2name << Log::PathSeparator()
           << strFolder3name << Log::PathSeparator() << strFilename << "\n";

    return bExists;
}

OTTransaction* LoadBoxReceipt(OTTransaction& theAbbrev, Ledger& theLedger)
{
    const std::int64_t lLedgerType = static_cast<int64_t>(theLedger.GetType());
    return LoadBoxReceipt(theAbbrev, lLedgerType);
}

OTTransaction* LoadBoxReceipt(
    OTTransaction& theAbbrev,
    std::int64_t lLedgerType)
{
    // See if the appropriate file exists, and load it up from
    // local storage, into a string.
    // Then, try to load the transaction from that string and see if successful.
    // If it verifies, then return it. Otherwise return nullptr.

    // Can only load abbreviated transactions (so they'll become their full
    // form.)
    //
    if (!theAbbrev.IsAbbreviated()) {
        otOut << __FUNCTION__ << ": Unable to load box receipt "
              << theAbbrev.GetTransactionNum()
              << ": "
                 "(Because argument 'theAbbrev' wasn't abbreviated.)\n";
        return nullptr;
    }

    // Next, see if the appropriate file exists, and load it up from
    // local storage, into a string.

    String strFolder1name, strFolder2name, strFolder3name, strFilename;

    if (!SetupBoxReceiptFilename(
            lLedgerType,
            theAbbrev,
            __FUNCTION__,  // "OTTransaction::LoadBoxReceipt",
            strFolder1name,
            strFolder2name,
            strFolder3name,
            strFilename))
        return nullptr;  // This already logs -- no need to log twice, here.

    // See if the box receipt exists before trying to load it...
    //
    if (!OTDB::Exists(
            strFolder1name.Get(),
            strFolder2name.Get(),
            strFolder3name.Get(),
            strFilename.Get())) {
        otWarn << __FUNCTION__
               << ": Box receipt does not exist: " << strFolder1name
               << Log::PathSeparator() << strFolder2name << Log::PathSeparator()
               << strFolder3name << Log::PathSeparator() << strFilename << "\n";
        return nullptr;
    }

    // Try to load the box receipt from local storage.
    //
    std::string strFileContents(OTDB::QueryPlainString(
        strFolder1name.Get(),  // <=== LOADING FROM DATA STORE.
        strFolder2name.Get(),
        strFolder3name.Get(),
        strFilename.Get()));
    if (strFileContents.length() < 2) {
        otErr << __FUNCTION__ << ": Error reading file: " << strFolder1name
              << Log::PathSeparator() << strFolder2name << Log::PathSeparator()
              << strFolder3name << Log::PathSeparator() << strFilename << "\n";
        return nullptr;
    }

    String strRawFile(strFileContents.c_str());

    if (!strRawFile.Exists()) {
        otErr << __FUNCTION__
              << ": Error reading file (resulting output "
                 "string is empty): "
              << strFolder1name << Log::PathSeparator() << strFolder2name
              << Log::PathSeparator() << strFolder3name << Log::PathSeparator()
              << strFilename << "\n";
        return nullptr;
    }

    // Finally, try to load the transaction from that string and see if
    // successful.
    //
    OTTransactionType* pTransType =
        OTTransactionType::TransactionFactory(strRawFile);

    if (nullptr == pTransType) {
        otErr << __FUNCTION__
              << ": Error instantiating transaction "
                 "type based on strRawFile: "
              << strFolder1name << Log::PathSeparator() << strFolder2name
              << Log::PathSeparator() << strFolder3name << Log::PathSeparator()
              << strFilename << "\n";
        return nullptr;
    }

    OTTransaction* pBoxReceipt = dynamic_cast<OTTransaction*>(pTransType);

    if (nullptr == pBoxReceipt) {
        otErr << __FUNCTION__
              << ": Error dynamic_cast from transaction "
                 "type to transaction, based on strRawFile: "
              << strFolder1name << Log::PathSeparator() << strFolder2name
              << Log::PathSeparator() << strFolder3name << Log::PathSeparator()
              << strFilename << "\n";
        delete pTransType;
        pTransType = nullptr;  // cleanup!
        return nullptr;
    }

    // BELOW THIS POINT, pBoxReceipt exists, and is an OTTransaction pointer,
    // and is loaded,
    // and basically is ready to be compared to theAbbrev, which is its
    // abbreviated version.
    // It MUST either be returned or deleted.

    bool bSuccess = theAbbrev.VerifyBoxReceipt(*pBoxReceipt);

    if (!bSuccess) {
        otErr << __FUNCTION__ << ": Failed verifying Box Receipt:\n"
              << strFolder1name << Log::PathSeparator() << strFolder2name
              << Log::PathSeparator() << strFolder3name << Log::PathSeparator()
              << strFilename << "\n";

        delete pBoxReceipt;
        pBoxReceipt = nullptr;
        return nullptr;
    } else
        otInfo << __FUNCTION__ << ": Successfully loaded Box Receipt in:\n"
               << strFolder1name << Log::PathSeparator() << strFolder2name
               << Log::PathSeparator() << strFolder3name << Log::PathSeparator()
               << strFilename << "\n";

    // Todo: security analysis. By this point we've verified the hash of the
    // transaction against the stored
    // hash inside the abbreviated version. (VerifyBoxReceipt) We've also
    // verified a few other values like transaction
    // number, and the "in ref to" display number. We're then assuming based on
    // those, that the adjustment and display
    // amount are correct. (The hash is actually a zero knowledge proof of this
    // already.) This is good for speedier
    // optimization but may be worth revisiting in case any security holes.
    // UPDATE: We'll save this for optimization needs in the future.
    //  pBoxReceipt->SetAbbrevAdjustment(       theAbbrev.GetAbbrevAdjustment()
    // );
    //  pBoxReceipt->SetAbbrevDisplayAmount(
    // theAbbrev.GetAbbrevDisplayAmount() );

    return pBoxReceipt;
}

bool SetupBoxReceiptFilename(
    std::int64_t lLedgerType,
    const String& strUserOrAcctID,
    const String& strNotaryID,
    const std::int64_t& lTransactionNum,
    const char* szCaller,
    String& strFolder1name,
    String& strFolder2name,
    String& strFolder3name,
    String& strFilename)
{
    OT_ASSERT(nullptr != szCaller);

    const char* pszFolder = nullptr;  // "nymbox" (or "inbox" or "outbox")
    switch (lLedgerType) {
        case 0:
            pszFolder = OTFolders::Nymbox().Get();
            break;
        case 1:
            pszFolder = OTFolders::Inbox().Get();
            break;
        case 2:
            pszFolder = OTFolders::Outbox().Get();
            break;
        //      case 3: (message ledger.)
        case 4:
            pszFolder = OTFolders::PaymentInbox().Get();
            break;
        case 5:
            pszFolder = OTFolders::RecordBox().Get();
            break;
        case 6:
            pszFolder = OTFolders::ExpiredBox().Get();
            break;
        default:
            otErr << "OTTransaction::" << __FUNCTION__ << " " << szCaller
                  << ": Error: unknown box type: " << lLedgerType
                  << ". (This should never happen.)\n";
            return false;
    }

    strFolder1name.Set(pszFolder);    // "nymbox" (or "inbox" or "outbox")
    strFolder2name.Set(strNotaryID);  // "NOTARY_ID"
    strFolder3name.Format("%s.r", strUserOrAcctID.Get());  // "NYM_ID.r"

    // "TRANSACTION_ID.rct"
    strFilename.Format("%" PRId64 ".rct", lTransactionNum);
    // todo hardcoding of file extension. Need to standardize extensions.

    // Finished product: "nymbox/NOTARY_ID/NYM_ID.r/TRANSACTION_ID.rct"

    return true;
}

bool SetupBoxReceiptFilename(
    std::int64_t lLedgerType,
    OTTransaction& theTransaction,
    const char* szCaller,
    String& strFolder1name,
    String& strFolder2name,
    String& strFolder3name,
    String& strFilename)
{
    String strUserOrAcctID;
    theTransaction.GetIdentifier(strUserOrAcctID);

    const String strNotaryID(theTransaction.GetRealNotaryID());

    return SetupBoxReceiptFilename(
        lLedgerType,
        strUserOrAcctID,
        strNotaryID,
        theTransaction.GetTransactionNum(),
        szCaller,
        strFolder1name,
        strFolder2name,
        strFolder3name,
        strFilename);
}

bool SetupBoxReceiptFilename(
    Ledger& theLedger,
    OTTransaction& theTransaction,
    const char* szCaller,
    String& strFolder1name,
    String& strFolder2name,
    String& strFolder3name,
    String& strFilename)
{
    std::int64_t lLedgerType = 0;

    switch (theLedger.GetType()) {
        case Ledger::nymbox:
            lLedgerType = 0;
            break;
        case Ledger::inbox:
            lLedgerType = 1;
            break;
        case Ledger::outbox:
            lLedgerType = 2;
            break;
        //        case OTLedger::message:         lLedgerType = 3;    break;
        case Ledger::paymentInbox:
            lLedgerType = 4;
            break;
        case Ledger::recordBox:
            lLedgerType = 5;
            break;
        case Ledger::expiredBox:
            lLedgerType = 6;
            break;
        default:
            otErr << "OTTransaction::" << __FUNCTION__ << " " << szCaller
                  << ": Error: unknown box type. "
                     "(This should never happen.)\n";
            return false;
    }

    return SetupBoxReceiptFilename(
        lLedgerType,
        theTransaction,
        szCaller,
        strFolder1name,
        strFolder2name,
        strFolder3name,
        strFilename);
}

}  // namespace opentxs
