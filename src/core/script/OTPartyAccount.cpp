// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/script/OTPartyAccount.hpp"

#include "opentxs/api/Native.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/script/OTAgent.hpp"
#include "opentxs/core/script/OTParty.hpp"
#include "opentxs/core/script/OTScript.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/util/Tag.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/OT.hpp"

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

// IDEA: Put a Nym in the Nyms folder for each entity. While it may
// not have a public key in the pubkey folder, or embedded within it,
// it can still have information about the entity or role related to it,
// which becomes accessible when that Nym is loaded based on the Entity ID.
// This also makes sure that Nyms and Entities don't ever share IDs, so the
// IDs become more and more interchangeable.

namespace opentxs
{
OTPartyAccount::OTPartyAccount(
    const api::Wallet& wallet,
    const std::string& dataFolder)
    : wallet_(wallet)
    , data_folder_{dataFolder}
    , m_pForParty(nullptr)
    , m_lClosingTransNo(0)
{
}

// For an account to be party to an agreement, there must be a closing
// transaction # provided, for the finalReceipt for that account.
OTPartyAccount::OTPartyAccount(
    const api::Wallet& wallet,
    const std::string& dataFolder,
    const std::string& str_account_name,
    const String& strAgentName,
    Account& theAccount,
    std::int64_t lClosingTransNo)
    : wallet_(wallet)
    , data_folder_{dataFolder}
    , m_pForParty(nullptr)
    // This gets set when this partyaccount is added to its party.
    , m_lClosingTransNo(lClosingTransNo)
    , m_strName(str_account_name.c_str())
    , m_strAcctID(theAccount.GetRealAccountID())
    , m_strInstrumentDefinitionID(theAccount.GetInstrumentDefinitionID())
    , m_strAgentName(strAgentName)
{
}

OTPartyAccount::OTPartyAccount(
    const api::Wallet& wallet,
    const std::string& dataFolder,
    const String& strName,
    const String& strAgentName,
    const String& strAcctID,
    const String& strInstrumentDefinitionID,
    std::int64_t lClosingTransNo)
    : wallet_(wallet)
    , data_folder_{dataFolder}
    , m_pForParty(nullptr)
    // This gets set when this partyaccount is added to its party.
    , m_lClosingTransNo(lClosingTransNo)
    , m_strName(strName)
    , m_strAcctID(strAcctID)
    , m_strInstrumentDefinitionID(strInstrumentDefinitionID)
    , m_strAgentName(strAgentName)
{
}

SharedAccount OTPartyAccount::get_account() const
{
    if (!m_strAcctID.Exists()) { return {}; }

    return wallet_.Account(data_folder_, Identifier::Factory(m_strAcctID));
}

// Every partyaccount has its own authorized agent's name.
// Use that name to look up the agent ON THE PARTY (I already
// have a pointer to my owner party.)
//
OTAgent* OTPartyAccount::GetAuthorizedAgent()
{
    OT_ASSERT(nullptr != m_pForParty);

    if (!m_strAgentName.Exists()) {
        otErr << "OTPartyAccount::" << __FUNCTION__
              << ": Error: Authorized agent "
                 "name (for this account) is blank!\n";
        return nullptr;
    }

    const std::string str_agent_name = m_strAgentName.Get();

    OTAgent* pAgent = m_pForParty->GetAgent(str_agent_name);

    return pAgent;
}

// This happens when the partyaccount is added to the party.
//
void OTPartyAccount::SetParty(OTParty& theOwnerParty)
{
    m_pForParty = &theOwnerParty;
}

bool OTPartyAccount::IsAccountByID(const Identifier& theAcctID) const
{
    if (!m_strAcctID.Exists()) { return false; }

    if (!m_strInstrumentDefinitionID.Exists()) { return false; }

    const auto theMemberAcctID = Identifier::Factory(m_strAcctID);
    if (!(theAcctID == theMemberAcctID)) {
        String strRHS(theAcctID);
        otLog4 << "OTPartyAccount::" << __FUNCTION__
               << ": Account IDs don't match: " << m_strAcctID << " / "
               << strRHS << " \n";
        // I set output to 4 because it's normal to call IsAccountByID() even
        // when they don't match.
        return false;
    }

    // They  match!

    return true;
}

bool OTPartyAccount::IsAccount(const Account& theAccount)
{
    if (!m_strAcctID.Exists()) {
        otErr << "OTPartyAccount::" << __FUNCTION__
              << ": Error: Empty m_strAcctID.\n";
        return false;
    }

    bool bCheckAssetId = true;
    if (!m_strInstrumentDefinitionID.Exists()) {
        otErr << "OTPartyAccount::" << __FUNCTION__
              << ": FYI, Asset ID is blank in this smart contract, for this "
                 "account.\n";
        bCheckAssetId = false;
    }

    const auto theAcctID = Identifier::Factory(m_strAcctID);
    if (!(theAccount.GetRealAccountID() == theAcctID)) {
        String strRHS(theAccount.GetRealAccountID());
        otLog4 << "OTPartyAccount::" << __FUNCTION__
               << ": Account IDs don't match: " << m_strAcctID << " / "
               << strRHS
               << " \n";  // I set output to 4 because it's normal to call
                          // IsAccount() even when they don't match.
        return false;
    }

    if (bCheckAssetId) {
        const auto theInstrumentDefinitionID =
            Identifier::Factory(m_strInstrumentDefinitionID);
        if (!(theAccount.GetInstrumentDefinitionID() ==
              theInstrumentDefinitionID)) {
            String strRHS(theAccount.GetInstrumentDefinitionID());
            otOut << "OTPartyAccount::" << __FUNCTION__
                  << ": Instrument Definition IDs don't "
                     "match ( "
                  << m_strInstrumentDefinitionID << " / " << strRHS
                  << " ) for Acct ID: " << m_strAcctID << " \n";
            return false;
        }
    }

    return true;
}

// I have a ptr to my owner (party), as well as to the actual account.
// I will ask him to verify whether he actually owns it.
bool OTPartyAccount::VerifyOwnership() const
{
    if (nullptr == m_pForParty) {
        otErr << "OTPartyAccount::" << __FUNCTION__
              << ": Error: nullptr pointer to "
                 "owner party. \n";
        return false;
    }

    auto account = get_account();

    if (false == bool(account)) {
        otErr << "OTPartyAccount::" << __FUNCTION__
              << ": Error: nullptr pointer to "
                 "account. (This function expects account to already be "
                 "loaded.) \n";
        return false;
    }  // todo maybe turn the above into OT_ASSERT()s.

    if (!m_pForParty->VerifyOwnershipOfAccount(account.get())) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Party %s doesn't verify as "
                 "the ACTUAL owner of account: "
              << m_strName << " \n";
        return false;
    }

    return true;
}

// I can get a ptr to my agent, and I have one to the actual account.
// I will ask him to verify whether he actually has agency over it.
bool OTPartyAccount::VerifyAgency()
{
    auto account = get_account();

    if (false == bool(account)) {
        otErr << "OTPartyAccount::" << __FUNCTION__
              << ": Error: nullptr pointer to "
                 "account. (This function expects account to already be "
                 "loaded.) \n";
        return false;
    }  // todo maybe turn the above into OT_ASSERT()s.

    OTAgent* pAgent = GetAuthorizedAgent();

    if (nullptr == pAgent) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Unable to find authorized agent (" << GetAgentName()
              << ") for this account: " << GetName() << " \n";
        return false;
    }

    if (!pAgent->VerifyAgencyOfAccount(account.get())) {
        otOut << "OTPartyAccount::" << __FUNCTION__ << ": Agent "
              << GetAgentName()
              << " doesn't verify as ACTUALLY having rights over account "
              << GetName() << " with ID: " << GetAcctID() << " \n";
        return false;
    }

    return true;
}

bool OTPartyAccount::DropFinalReceiptToInbox(
    const String& strNotaryID,
    OTSmartContract& theSmartContract,
    const std::int64_t& lNewTransactionNumber,
    const String& strOrigCronItem,
    String* pstrNote,
    String* pstrAttachment)
{
    const char* szFunc = "OTPartyAccount::DropFinalReceiptToInbox";

    if (nullptr == m_pForParty) {
        otErr << szFunc << ": nullptr m_pForParty.\n";
        return false;
    } else if (!m_strAcctID.Exists()) {
        otErr << szFunc << ": Empty Acct ID.\n";
        return false;
    } else if (!m_strAgentName.Exists()) {
        otErr << szFunc << ": No agent named for this account.\n";
        return false;
    }

    // TODO: When entites and roles are added, this function may change a bit to
    // accommodate them.

    const std::string str_agent_name(m_strAgentName.Get());

    OTAgent* pAgent = m_pForParty->GetAgent(str_agent_name);

    if (nullptr == pAgent)
        otErr << szFunc << ": named agent wasn't found on party.\n";
    else {
        const auto theAccountID = Identifier::Factory(m_strAcctID);

        return pAgent->DropFinalReceiptToInbox(
            strNotaryID,
            theSmartContract,
            theAccountID,  // acct ID from this.
            lNewTransactionNumber,
            m_lClosingTransNo,  // closing_no from this.
            strOrigCronItem,
            pstrNote,
            pstrAttachment);
    }

    return false;
}

// CALLER IS RESPONSIBLE TO DELETE.
// This is very low-level. (It's better to use OTPartyAccount through it's
// interface, than to just load up its account directly.) But this is here
// because it is appropriate in certain cases.
SharedAccount OTPartyAccount::LoadAccount()
{
    if (!m_strAcctID.Exists()) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Bad: Acct ID is blank for "
                 "account: "
              << m_strName << " \n";

        return {};
    }

    auto account =
        wallet_.Account(data_folder_, Identifier::Factory(m_strAcctID));

    if (false == bool(account)) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Failed trying to load account: " << m_strName
              << ", with AcctID: " << m_strAcctID << " \n";

        return {};
    }

    // This compares instrument definition ID, AND account ID on the actual
    // loaded account, to what is expected.
    else if (!IsAccount(account.get())) {

        return {};
    }

    return account;
}

void OTPartyAccount::Serialize(
    Tag& parent,
    bool bCalculatingID,
    bool bSpecifyInstrumentDefinitionID) const
{
    TagPtr pTag(new Tag("assetAccount"));

    pTag->add_attribute("name", m_strName.Get());
    pTag->add_attribute("acctID", bCalculatingID ? "" : m_strAcctID.Get());
    pTag->add_attribute(
        "instrumentDefinitionID",
        (bCalculatingID && !bSpecifyInstrumentDefinitionID)
            ? ""
            : m_strInstrumentDefinitionID.Get());
    pTag->add_attribute(
        "agentName", bCalculatingID ? "" : m_strAgentName.Get());
    pTag->add_attribute(
        "closingTransNo", formatLong(bCalculatingID ? 0 : m_lClosingTransNo));

    parent.add_tag(pTag);
}

void OTPartyAccount::RegisterForExecution(OTScript& theScript)
{
    const std::string str_acct_name = m_strName.Get();
    theScript.AddAccount(str_acct_name, *this);
}

// Done
bool OTPartyAccount::Compare(const OTPartyAccount& rhs) const
{
    if (!(GetName().Compare(rhs.GetName()))) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Names don't match: " << GetName() << " / " << rhs.GetName()
              << " \n";
        return false;
    }

    if ((GetClosingTransNo() > 0) && (rhs.GetClosingTransNo() > 0) &&
        (GetClosingTransNo() != rhs.GetClosingTransNo())) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Closing transaction numbers don't "
                 "match: "
              << GetName() << " \n";
        return false;
    }

    if ((GetAcctID().Exists()) && (rhs.GetAcctID().Exists()) &&
        (!GetAcctID().Compare(rhs.GetAcctID()))) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Asset account numbers don't match "
                 "for party account "
              << GetName() << ".\n( " << GetAcctID() << "  /  "
              << rhs.GetAcctID() << " ) \n";
        return false;
    }

    if ((GetAgentName().Exists()) && (rhs.GetAgentName().Exists()) &&
        (!GetAgentName().Compare(rhs.GetAgentName()))) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Agent names don't match for party "
                 "account "
              << GetName() << ".\n( " << GetAgentName() << "  /  "
              << rhs.GetAgentName() << " ) \n";
        return false;
    }

    if ((GetInstrumentDefinitionID().Exists() &&
         rhs.GetInstrumentDefinitionID().Exists()) &&
        !GetInstrumentDefinitionID().Compare(rhs.GetInstrumentDefinitionID())) {
        otOut << "OTPartyAccount::" << __FUNCTION__
              << ": Instrument Definition IDs don't "
                 "exist, or don't match ( "
              << GetInstrumentDefinitionID() << " / "
              << rhs.GetInstrumentDefinitionID()
              << " ) for party's account: " << GetName() << " \n";
        return false;
    }

    return true;
}

OTPartyAccount::~OTPartyAccount()
{
    // m_pForParty NOT cleaned up here. pointer is only for convenience.
    m_pForParty = nullptr;
}
}  // namespace opentxs
