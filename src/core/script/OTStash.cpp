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

#include "opentxs/core/script/OTStash.hpp"

#include "opentxs/core/script/OTStashItem.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/util/Tag.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"

#include <irrxml/irrXML.hpp>
#include <sys/types.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

namespace opentxs
{

void OTStash::Serialize(Tag& parent) const
{
    std::uint32_t sizeMapStashItems = m_mapStashItems.size();

    TagPtr pTag(new Tag("stash"));

    pTag->add_attribute("name", m_str_stash_name);
    pTag->add_attribute("count", formatUint(sizeMapStashItems));

    for (auto& it : m_mapStashItems) {
        const std::string str_instrument_definition_id = it.first;
        OTStashItem* pStashItem = it.second;
        OT_ASSERT(
            (str_instrument_definition_id.size() > 0) &&
            (nullptr != pStashItem));

        TagPtr pTagItem(new Tag("stashItem"));

        pTagItem->add_attribute(
            "instrumentDefinitionID",
            pStashItem->GetInstrumentDefinitionID().Get());
        pTagItem->add_attribute("balance", formatLong(pStashItem->GetAmount()));

        pTag->add_tag(pTagItem);
    }

    parent.add_tag(pTag);
}

std::int32_t OTStash::ReadFromXMLNode(
    irr::io::IrrXMLReader*& xml,
    const String& strStashName,
    const String& strItemCount)
{
    if (!strStashName.Exists()) {
        otErr << "OTStash::ReadFromXMLNode: Failed: Empty stash 'name' "
                 "attribute.\n";
        return (-1);
    }

    m_str_stash_name = strStashName.Get();

    //
    // Load up the stash items.
    //
    std::int32_t nCount = strItemCount.Exists() ? atoi(strItemCount.Get()) : 0;
    if (nCount > 0) {
        while (nCount-- > 0) {
            //            xml->read();
            if (!Contract::SkipToElement(xml)) {
                otOut << "OTStash::ReadFromXMLNode: Failure: Unable to find "
                         "expected element.\n";
                return (-1);
            }

            if ((xml->getNodeType() == irr::io::EXN_ELEMENT) &&
                (!strcmp("stashItem", xml->getNodeName()))) {
                String strInstrumentDefinitionID = xml->getAttributeValue(
                    "instrumentDefinitionID");  // Instrument Definition ID of
                                                // this account.
                String strAmount = xml->getAttributeValue(
                    "balance");  // Account ID for this account.

                if (!strInstrumentDefinitionID.Exists() ||
                    !strAmount.Exists()) {
                    otErr << "OTStash::ReadFromXMLNode: Error loading "
                             "stashItem: Either the instrumentDefinitionID ("
                          << strInstrumentDefinitionID << "), or the balance ("
                          << strAmount << ") was EMPTY.\n";
                    return (-1);
                }

                if (!CreditStash(
                        strInstrumentDefinitionID.Get(),
                        strAmount.ToLong()))  // <===============
                {
                    otErr << "OTStash::ReadFromXMLNode: Failed crediting "
                             "stashItem for stash "
                          << strStashName << ". instrumentDefinitionID ("
                          << strInstrumentDefinitionID << "), balance ("
                          << strAmount << ").\n";
                    return (-1);
                }

                // (Success)
            } else {
                otErr << "OTStash::ReadFromXMLNode: Expected stashItem "
                         "element.\n";
                return (-1);  // error condition
            }
        }  // while
    }

    if (!Contract::SkipAfterLoadingField(xml))  // </stash>
    {
        otOut << "*** OTStash::ReadFromXMLNode: Bad data? Expected "
                 "EXN_ELEMENT_END here, but "
                 "didn't get it. Returning -1.\n";
        return (-1);
    }

    return 1;
}

OTStash::OTStash()
{
    // m_mapStashItems
}

OTStash::OTStash(const String& strInstrumentDefinitionID, std::int64_t lAmount)
{
    OTStashItem* pItem = new OTStashItem(strInstrumentDefinitionID, lAmount);
    OT_ASSERT(nullptr != pItem);

    m_mapStashItems.insert(std::pair<std::string, OTStashItem*>(
        strInstrumentDefinitionID.Get(), pItem));
}

OTStash::OTStash(
    const Identifier& theInstrumentDefinitionID,
    std::int64_t lAmount)
{
    OTStashItem* pItem = new OTStashItem(theInstrumentDefinitionID, lAmount);
    OT_ASSERT(nullptr != pItem);

    String strInstrumentDefinitionID(theInstrumentDefinitionID);

    m_mapStashItems.insert(std::pair<std::string, OTStashItem*>(
        strInstrumentDefinitionID.Get(), pItem));
}

OTStash::~OTStash()
{
    while (!m_mapStashItems.empty()) {
        OTStashItem* pTemp = m_mapStashItems.begin()->second;
        OT_ASSERT(nullptr != pTemp);
        delete pTemp;
        pTemp = nullptr;
        m_mapStashItems.erase(m_mapStashItems.begin());
    }
}

// Creates it if it's not already there.
// (*this owns it and will clean it up when destroyed.)
//
OTStashItem* OTStash::GetStash(const std::string& str_instrument_definition_id)
{
    auto it = m_mapStashItems.find(str_instrument_definition_id);

    if (m_mapStashItems.end() == it)  // It's not already there for this
                                      // instrument definition.
    {
        const String strInstrumentDefinitionID(
            str_instrument_definition_id.c_str());
        OTStashItem* pStashItem = new OTStashItem(strInstrumentDefinitionID);
        OT_ASSERT(nullptr != pStashItem);

        m_mapStashItems.insert(std::pair<std::string, OTStashItem*>(
            strInstrumentDefinitionID.Get(), pStashItem));
        return pStashItem;
    }

    OTStashItem* pStashItem = it->second;
    OT_ASSERT(nullptr != pStashItem);

    return pStashItem;
}

std::int64_t OTStash::GetAmount(const std::string& str_instrument_definition_id)
{
    OTStashItem* pStashItem =
        GetStash(str_instrument_definition_id);  // (Always succeeds, and will
                                                 // OT_ASSERT()
                                                 // if failure.)

    return pStashItem->GetAmount();
}

bool OTStash::CreditStash(
    const std::string& str_instrument_definition_id,
    const std::int64_t& lAmount)
{
    OTStashItem* pStashItem =
        GetStash(str_instrument_definition_id);  // (Always succeeds, and will
                                                 // OT_ASSERT()
                                                 // if failure.)

    return pStashItem->CreditStash(lAmount);
}

bool OTStash::DebitStash(
    const std::string& str_instrument_definition_id,
    const std::int64_t& lAmount)
{
    OTStashItem* pStashItem =
        GetStash(str_instrument_definition_id);  // (Always succeeds, and will
                                                 // OT_ASSERT()
                                                 // if failure.)

    return pStashItem->DebitStash(lAmount);
}

}  // namespace opentxs
