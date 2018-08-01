// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/client/OTMessageOutbuffer.hpp"

#include "opentxs/api/Legacy.hpp"
#include "opentxs/consensus/ServerContext.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/util/OTFolders.hpp"
#include "opentxs/core/util/OTPaths.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/Message.hpp"
#include "opentxs/core/NumList.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/core/OTStorage.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/String.hpp"

#include <cinttypes>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

namespace opentxs
{
OTMessageOutbuffer::OTMessageOutbuffer(const api::Legacy& legacy)
    : legacy_(legacy)
    , messagesMap_()
    , dataFolder_(legacy_.ClientDataFolder())
{
    OT_ASSERT(dataFolder_.Exists());
}

void OTMessageOutbuffer::AddSentMessage(Message& theMessage)  // must be heap
                                                              // allocated.
{
    Lock lock(lock_);
    std::int64_t lRequestNum = 0;

    if (theMessage.m_strRequestNum.Exists())
        lRequestNum = theMessage.m_strRequestNum.ToLong();  // The map index
                                                            // is the request
                                                            // number on the
                                                            // message itself.

    // It's technically possible to have TWO messages (from two different
    // servers) that happen to have the same request number. So we verify
    // that here, before removing any old ones with the same number and IDs.
    //
    auto it = messagesMap_.begin();

    for (; it != messagesMap_.end(); ++it) {

        const std::int64_t& lTempReqNum = it->first;

        if (lTempReqNum != lRequestNum) { continue; }

        Message* pMsg = it->second;
        OT_ASSERT(nullptr != pMsg);

        //
        // If a server ID was passed in, but doesn't match the server ID on this
        // message,
        // Then skip this one. (Same with the NymID.)
        //
        if (!theMessage.m_strNotaryID.Compare(pMsg->m_strNotaryID) ||
            !theMessage.m_strNymID.Compare(pMsg->m_strNymID)) {
            continue;
        } else {
            delete pMsg;
            pMsg = nullptr;
            messagesMap_.erase(it);
            break;
        }
    }
    // Whatever it was, it's gone now!

    // Now that we KNOW there's nothing already there with that request number
    // (for that
    // server ID and Nym ID), we go ahead and add the new message to the map.
    // (And take ownership.)
    //
    messagesMap_.insert(
        std::pair<std::int64_t, Message*>(lRequestNum, &theMessage));

    //
    // Save it to local storage, in case we don't see the reply until the next
    // run.
    //
    bool bAlreadyExists = false, bIsNewFolder = false;
    String strFolder, strFolder1, strFolder2;
    strFolder1.Format(
        "%s%s%s",
        OTFolders::Nym().Get(),
        Log::PathSeparator(),
        theMessage.m_strNotaryID.Get());
    strFolder2.Format(
        "%s%s%s",
        strFolder1.Get(),
        Log::PathSeparator(),
        "sent" /*todo hardcoding*/);

    strFolder.Format(
        "%s%s%s",
        strFolder2.Get(),
        Log::PathSeparator(),
        theMessage.m_strNymID.Get());

    String strFolderPath = "", strFolder1Path = "", strFolder2Path = "";

    OTPaths::AppendFolder(strFolderPath, dataFolder_, strFolder);
    OTPaths::AppendFolder(strFolder1Path, dataFolder_, strFolder1);
    OTPaths::AppendFolder(strFolder2Path, dataFolder_, strFolder2);

    OTPaths::ConfirmCreateFolder(strFolderPath, bAlreadyExists, bIsNewFolder);
    OTPaths::ConfirmCreateFolder(strFolder1Path, bAlreadyExists, bIsNewFolder);
    OTPaths::ConfirmCreateFolder(strFolder2Path, bAlreadyExists, bIsNewFolder);

    String strFile;
    strFile.Format("%s.msg", theMessage.m_strRequestNum.Get());

    theMessage.SaveContract(strFolder.Get(), strFile.Get());

    // We also keep a list of the request numbers, so let's load it up, add the
    // number
    // to that list, and then save it again.
    //
    NumList theNumList;
    std::string str_data_filename("sent.dat");  // todo hardcoding.
    if (OTDB::Exists(strFolder.Get(), str_data_filename)) {
        String strNumList(
            OTDB::QueryPlainString(strFolder.Get(), str_data_filename));
        if (strNumList.Exists()) theNumList.Add(strNumList);
        theNumList.Add(lRequestNum);  // Add the new request number to it.
    } else  // it doesn't exist on disk, so let's just create it from the list
            // we
            // have in RAM so we can store it to disk.
    {
        it = messagesMap_.begin();
        while (it != messagesMap_.end()) {

            const std::int64_t& lTempReqNum = it->first;

            Message* pMsg = it->second;
            OT_ASSERT(nullptr != pMsg);

            //
            // If a server ID was passed in, but doesn't match the server ID on
            // this message,
            // Then skip this one. (Same with the NymID.)
            //
            if (!theMessage.m_strNotaryID.Compare(pMsg->m_strNotaryID) ||
                !theMessage.m_strNymID.Compare(pMsg->m_strNymID)) {
                ++it;
                continue;
            } else {
                theNumList.Add(lTempReqNum);
            }
            ++it;
        }
    }  // else

    // By this point, theNumList has either been loaded from local storage and
    // had the new number added,
    // or it wasn't in local storage and thus we created it and added all the
    // numnbers to it (including new one.)
    // Therefore nothing left to do here, but save it back again!
    //
    String strOutput;
    theNumList.Output(strOutput);

    if (!OTDB::StorePlainString(
            strOutput.Get(),
            strFolder.Get(),
            str_data_filename))  // todo hardcoding.
    {
        otErr << "OTMessageOutbuffer::AddSentMessage: Error: failed writing "
                 "list of request numbers to storage.\n";
    }
}

// You are NOT responsible to delete the OTMessage object
// that comes back from this function. The buffer maintains
// ownership until you call RemoveSentMessage().

Message* OTMessageOutbuffer::GetSentMessage(
    const std::int64_t& lRequestNum,
    const String& strNotaryID,
    const String& strNymID)
{
    Lock lock(lock_);
    auto it = messagesMap_.begin();

    for (; it != messagesMap_.end(); ++it) {

        const std::int64_t& lTempReqNum = it->first;

        if (lTempReqNum != lRequestNum) { continue; }

        Message* pMsg = it->second;
        OT_ASSERT(nullptr != pMsg);

        //
        // If a server ID was passed in, but doesn't match the server ID on this
        // message,
        // Then skip this one. (Same with the NymID.)
        if (!strNotaryID.Compare(pMsg->m_strNotaryID) ||
            !strNymID.Compare(pMsg->m_strNymID)) {
            continue;
        } else {
            return pMsg;
        }
    }

    // Didn't find it? Okay let's load it from local storage, if it's there...
    //
    String strFolder, strFile;
    strFolder.Format(
        "%s%s%s%s%s%s%s",
        OTFolders::Nym().Get(),
        Log::PathSeparator(),
        strNotaryID.Get(),
        Log::PathSeparator(),
        "sent",
        /*todo hardcoding*/ Log::PathSeparator(),
        strNymID.Get());
    strFile.Format("%" PRId64 ".msg", lRequestNum);

    // Check the existing list, if it exists.
    //
    NumList theNumList;
    std::string str_data_filename("sent.dat");
    if (OTDB::Exists(strFolder.Get(), str_data_filename))  // todo hardcoding.
    {
        String strNumList(
            OTDB::QueryPlainString(strFolder.Get(), str_data_filename));

        if (strNumList.Exists()) theNumList.Add(strNumList);

        if (theNumList.Verify(lRequestNum)) {
            // Even if the outgoing message was stored, we still act like it
            // "doesn't exist" if it doesn't appear on the official list.
            // The list is what matters -- the message is just the contents
            // referencedby that list.
            Message* pMsg = new Message{legacy_.ClientDataFolder()};

            OT_ASSERT(nullptr != pMsg);

            std::unique_ptr<Message> theMsgAngel(pMsg);

            if (OTDB::Exists(strFolder.Get(), strFile.Get()) &&
                pMsg->LoadContract(strFolder.Get(), strFile.Get())) {
                // Since we had to load it from local storage, let's add it to
                // the list in RAM.
                //
                messagesMap_.insert(std::pair<std::int64_t, Message*>(
                    lRequestNum, theMsgAngel.release()));
                return pMsg;
            }
        }
    }

    // STILL didn't find it? (Failure.)
    //
    return nullptr;
}

// WARNING: ONLY call this (with arguments) directly after a successful
// getNymboxResponse has been received!
// See comments below for more details.
//
void OTMessageOutbuffer::Clear(
    const String& pstrNotaryID,
    const String& pstrNymID,
    const bool pbHarvestingForRetry,
    ServerContext& context,
    const Identifier& nymID)
{
    OT_ASSERT(pstrNymID.Exists());
    OT_ASSERT(pstrNotaryID.Exists());
    OT_ASSERT(nymID == Identifier::Factory(pstrNymID));

    Lock lock(lock_);
    auto it = messagesMap_.begin();

    while (it != messagesMap_.end()) {
        const std::int64_t& lRequestNum = it->first;
        Message* pThisMsg = it->second;

        OT_ASSERT(nullptr != pThisMsg);

        // If a server ID was passed in, but doesn't match the server ID on this
        // message, Then skip this one. (Same with the NymID.)
        const bool notaryMatch = pstrNotaryID.Compare(pThisMsg->m_strNotaryID);
        const bool nymMatch = pstrNymID.Compare(pThisMsg->m_strNymID);
        const bool match = notaryMatch && nymMatch;

        if (!match) {
            ++it;
            continue;
        }

        /*
        Sent messages are cached because some of them are so important, that the
        server drops a reply notice into the Nymbox to make sure they were
        received. This way, when we download the Nymbox we can SEE which
        messages were ACTUALLY replied to, and at that time, we removed those
        messages already from *this "sent buffer." After that loop was done, we
        called CLEAR (this function) and cleared ALL the sent messages from the
        buffer (for the appropriate server and nym IDs...clear without those IDs
        is only for the destructor.)

        This Clear, where we are now, HARVESTS the transaction numbers back from
        any messages left in the sent buffer. We are able to do this with
        confidence because we know that this function is only called in
        getNymboxResponse on client side, and only after the ones with actual
        replies (as evidenced by the Nymbox) have already been removed from
          *this "sent buffer."

        Why were they removed in advance? Because clearly: if the server HAS
        replied to them already, then there's no need to harvest anything: just
        let it process as normal, whether the transaction inside is a success or
        fail. (We KNOW the message didn't fail because otherwise there wouldn't
        even be a notice in the Nymbox. So this is about the transaction
        inside.)

        So we remove the ones that we DEFINITELY know the server HAS replied to.

        And the ones remaining? We know for those, the server definitely has NOT
        replied to them (the message must have been dropped by the network or
        something.) How do we know this? Because there would be a notice in the
        Nymbox! So at the moment of successful getNymboxResponse, we are able to
        loop through those receipts and know FOR SURE, WHICH ones definitely
        have a reply, and which ones definitely DO NOT.

        The ones where we definitely do NOT have a reply--that is, the ones that
        are in the "sent messages" buffer, but are not in the Nymbox with the
        same request number--we harvest those numbers, since the server clearly
        never saw them, or rejected the message before the transaction itself
        even had a chance to run.
        */

        /*
        getNymbox            -- client is NOT sending hash, server is NOT
                                rejecting bad hashes, server IS SENDING HASH in
                                the getNymboxResponse reply
        getRequestNumber     -- client is NOT sending hash, server is NOT
                                rejecting bad hashes, server IS SENDING HASH in
                                the getRequestNumberResponse reply
        processNymbox        -- client is SENDING HASH, server is REJECTING BAD
                                HASHES, server is SENDING HASH in the
                                processNymboxResponse reply
        notarizeTransaction  -- client is SENDING HASH, server is REJECTING BAD
                                HASHES, server is SENDING HASH in the
                                notarizeTransactionResponse reply
        processInbox         -- client is SENDING HASH, server is REJECTING BAD
                                HASHES, server is SENDING HASH in the
                                processInboxResponse reply
        triggerClause        -- client is SENDING HASH, server is REJECTING BAD
                                HASHES, server is SENDING HASH in the
                                triggerClauseResponse reply

        getTransactionNumber -- client is SENDING HASH, server is REJECTING BAD
                                HASHES, server is SENDING HASH in the
                                getTransactionNumResponse reply

        Already covered in NotarizeTransaction:
            transfer,
            withdrawal,
            deposit,
            marketOffer,
            paymentPlan,
            smartContract,
            cancelCronItem,
            exchangeBasket
        */

        if (pThisMsg->m_ascPayload.Exists() &&
            (pThisMsg->m_strCommand.Compare("processNymbox") ||
             pThisMsg->m_strCommand.Compare("processInbox") ||
             pThisMsg->m_strCommand.Compare("notarizeTransaction") ||
             pThisMsg->m_strCommand.Compare("triggerClause"))) {

            // If we are here in the first place (i.e. after getNymboxResponse
            // just removed all the messages in this sent buffer that already
            // had a reply sitting in the nymbox) therefore we KNOW any messages
            // in here never got a reply from the server

            // If the msg had been a success, the reply (whether transaction
            // within succeeded or failed) would have been dropped into my
            // Nymbox, and thus removed from this "sent buffer" in
            // getNymboxResponse.
            const bool bReplyWasSuccess = false;

            // If the msg had been an explicit failure, the reply (without the
            // transaction inside of it even having a chance to succeed or fail)
            // would definitely NOT have been dropped into my Nymbox, and thus
            // removed from this "sent buffer" in getNymboxResponse. However, IN
            // THIS ONE CASE, since we DID just download the Nymbox and verify
            // there ARE NO REPLIES for this request number (before calling this
            // function), and since a dropped message is basically identical to
            // a rejected message, since in either case, the transaction itself
            // never even had a chance to run, we are able to now harvest the
            // message AS IF the server HAD explicitly rejected the message.
            // This is why I pass true here, where anywhere else in the code I
            // would always pass false unless I had explicitly received a
            // failure from the server. This place in the code, where we are
            // now, is the failsafe endpoint for missed/dropped messages! IF
            // they STILL haven't been found by this point, they are cleaned up
            // as if the message was explicitly rejected by the server before
            // the transaction even had a chance to run.
            const bool bReplyWasFailure = true;

            // Per above, since "the transaction never had a chance to run" then
            // it could NOT have been an explicit success.
            const bool bTransactionWasSuccess = false;

            // Per above, since "the transaction never had a chance to run" then
            // it could NOT have been an explicit failure.
            const bool bTransactionWasFailure = false;

            // Actually it's pNym who is "harvesting" the numbers in this call.
            pThisMsg->HarvestTransactionNumbers(
                context,
                pbHarvestingForRetry,
                bReplyWasSuccess,
                bReplyWasFailure,
                bTransactionWasSuccess,
                bTransactionWasFailure);
        }  // if there's a transaction to be harvested inside this message.

        String strFolder, strFile;
        strFolder.Format(
            "%s%s%s%s%s%s%s",
            OTFolders::Nym().Get(),
            Log::PathSeparator(),
            pstrNotaryID.Get(),
            Log::PathSeparator(),
            "sent",
            Log::PathSeparator(),
            pstrNymID.Get());
        strFile.Format("%" PRId64 ".msg", lRequestNum);
        NumList theNumList;
        std::string str_data_filename("sent.dat");

        if (OTDB::Exists(strFolder.Get(), str_data_filename)) {
            String strNumList(
                OTDB::QueryPlainString(strFolder.Get(), str_data_filename));

            if (strNumList.Exists()) { theNumList.Add(strNumList); }

            theNumList.Remove(lRequestNum);
        }

        String strOutput;
        theNumList.Output(strOutput);
        const bool saved = OTDB::StorePlainString(
            strOutput.Get(), strFolder.Get(), str_data_filename);

        if (!saved) {
            otErr << "OTMessageOutbuffer::Clear: Error: failed writing list of "
                  << "request numbers to storage." << std::endl;
        }

        // Make sure any messages being erased here, are also erased from local
        // storage.
        std::unique_ptr<Message> storedMessage(
            new Message{legacy_.ClientDataFolder()});

        OT_ASSERT(storedMessage);

        if (OTDB::Exists(strFolder.Get(), strFile.Get()) &&
            storedMessage->LoadContract(strFolder.Get(), strFile.Get())) {
            OTDB::EraseValueByKey(strFolder.Get(), strFile.Get());
        }

        delete pThisMsg;
        pThisMsg = nullptr;

        it = messagesMap_.erase(it);
    }
}

// OTMessageOutbuffer deletes the OTMessage when you call this.
//
bool OTMessageOutbuffer::RemoveSentMessage(
    const std::int64_t& lRequestNum,
    const String& strNotaryID,
    const String& strNymID)
{
    Lock lock(lock_);
    String strFolder, strFile;
    strFolder.Format(
        "%s%s%s%s%s%s%s",
        OTFolders::Nym().Get(),
        Log::PathSeparator(),
        strNotaryID.Get(),
        Log::PathSeparator(),
        "sent",
        /*todo hardcoding*/ Log::PathSeparator(),
        strNymID.Get());
    strFile.Format("%" PRId64 ".msg", lRequestNum);

    auto it = messagesMap_.begin();

    bool bReturnValue = false;

    while (it != messagesMap_.end()) {

        const std::int64_t& lTempReqNum = it->first;

        if (lTempReqNum != lRequestNum) {
            ++it;
            continue;
        }

        Message* pMsg = it->second;
        OT_ASSERT(nullptr != pMsg);

        //
        // If a server ID was passed in, but doesn't match the server ID on this
        // message,
        // Then skip this one. (Same with the NymID.)
        if (!strNotaryID.Compare(pMsg->m_strNotaryID) ||
            !strNymID.Compare(pMsg->m_strNymID)) {
            ++it;
            continue;
        } else {
            delete pMsg;
            pMsg = nullptr;

            auto temp_it = it;
            ++temp_it;
            messagesMap_.erase(it);
            it = temp_it;  // here's where it gets incremented. (During the
                           // erase, basically.)

            bReturnValue = true;
            break;
        }
    }

    // Whether we found it in RAM or not, let's make sure to delete it from
    // local storage, if it's there... (Since there's a list there we have to
    // update,
    // anyway.)
    // We keep a list of the request numbers, so let's load it up, remove the
    // number
    // from that list, and then save it again.

    NumList theNumList;
    std::string str_data_filename("sent.dat");  // todo hardcoding.
    if (OTDB::Exists(strFolder.Get(), str_data_filename)) {
        String strNumList(
            OTDB::QueryPlainString(strFolder.Get(), str_data_filename));
        if (strNumList.Exists()) theNumList.Add(strNumList);
        theNumList.Remove(lRequestNum);
    } else  // it doesn't exist on disk, so let's just create it from the list
            // we
            // have in RAM so we can store it to disk.
    {
        it = messagesMap_.begin();
        while (it != messagesMap_.end()) {

            const std::int64_t& lTempReqNum = it->first;

            Message* pMsg = it->second;
            OT_ASSERT(nullptr != pMsg);

            //
            // If a server ID was passed in, but doesn't match the server ID on
            // this message,
            // Then skip this one. (Same with the NymID.)
            //
            if (!strNotaryID.Compare(pMsg->m_strNotaryID) ||
                !strNymID.Compare(pMsg->m_strNymID)) {
                ++it;
                continue;
            } else {
                theNumList.Add(lTempReqNum);
            }
            ++it;
        }
    }  // else

    // By this point, theNumList has either been loaded from local storage and
    // had the number removed,
    // or it wasn't in local storage and thus we created it and added all the
    // numbers to it from RAM (not
    // including the one being erased, since it was already removed from the RAM
    // list, above.) So either
    // way, the number being removed is now ABSENT from theNumList.
    //
    // Therefore nothing left to do here, but save it back again!
    //
    String strOutput;
    theNumList.Output(strOutput);
    if (!OTDB::StorePlainString(
            strOutput.Get(), strFolder.Get(), str_data_filename)) {
        otErr << "OTMessageOutbuffer::RemoveSentMessage: Error: failed writing "
                 "list of request numbers to storage.\n";
    }

    // Now that we've updated the numlist in local storage, let's
    // erase the sent message itself...
    //
    Message* pMsg = new Message{legacy_.ClientDataFolder()};
    OT_ASSERT(nullptr != pMsg);
    std::unique_ptr<Message> theMsgAngel(pMsg);

    if (OTDB::Exists(strFolder.Get(), strFile.Get()) &&
        pMsg->LoadContract(strFolder.Get(), strFile.Get())) {
        OTDB::EraseValueByKey(strFolder.Get(), strFile.Get());
        return true;
    }

    return bReturnValue;
}

Message* OTMessageOutbuffer::GetSentMessage(const OTTransaction& theTransaction)
{
    const std::int64_t& lRequestNum = theTransaction.GetRequestNum();
    const String strNotaryID(theTransaction.GetPurportedNotaryID());
    const String strNymID(theTransaction.GetNymID());

    return GetSentMessage(lRequestNum, strNotaryID, strNymID);
}

// OTMessageOutbuffer deletes the OTMessage when you call this.
//
bool OTMessageOutbuffer::RemoveSentMessage(const OTTransaction& theTransaction)
{
    const std::int64_t& lRequestNum = theTransaction.GetRequestNum();
    const String strNotaryID(theTransaction.GetPurportedNotaryID());
    const String strNymID(theTransaction.GetNymID());

    return RemoveSentMessage(lRequestNum, strNotaryID, strNymID);
}

OTMessageOutbuffer::~OTMessageOutbuffer() { messagesMap_.clear(); }

}  // namespace opentxs
