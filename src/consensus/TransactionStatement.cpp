// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/consensus/TransactionStatement.hpp"

#include "opentxs/core/crypto/OTASCIIArmor.hpp"
#include "opentxs/core/util/Tag.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/NumList.hpp"
#include "opentxs/core/OTStringXML.hpp"

#include <irrxml/irrXML.hpp>

namespace opentxs
{
TransactionStatement::TransactionStatement(
    const std::string& notary,
    const std::set<TransactionNumber>& issued,
    const std::set<TransactionNumber>& available)
    : version_("1.0")
    , nym_id_("")
    , notary_(notary)
    , available_(available)
    , issued_(issued)
{
}

TransactionStatement::TransactionStatement(const String& serialized)
{
    auto raw = irr::io::createIrrXMLReader(OTStringXML(serialized));
    std::unique_ptr<irr::io::IrrXMLReader> xml(raw);

    if (!xml) { return; }

    while (xml && xml->read()) {
        const String nodeName = xml->getNodeName();
        switch (xml->getNodeType()) {
            case irr::io::EXN_NONE:
            case irr::io::EXN_TEXT:
            case irr::io::EXN_COMMENT:
            case irr::io::EXN_ELEMENT_END:
            case irr::io::EXN_CDATA: {
            } break;
            case irr::io::EXN_ELEMENT: {
                if (nodeName.Compare("nymData")) {
                    version_ = xml->getAttributeValue("version");
                    nym_id_ = xml->getAttributeValue("nymID");
                } else if (nodeName.Compare("transactionNums")) {
                    notary_ = xml->getAttributeValue("notaryID");
                    String list;
                    const bool loaded =
                        Contract::LoadEncodedTextField(raw, list);

                    if (notary_.empty() || !loaded) {
                        otErr << __FUNCTION__
                              << ": Error: transactionNums field without value."
                              << std::endl;
                        break;
                    }

                    NumList numlist;

                    if (!list.empty()) { numlist.Add(list); }

                    TransactionNumber number = 0;

                    while (numlist.Peek(number)) {
                        numlist.Pop();

                        otLog3 << "Transaction Number " << number
                               << " ready-to-use for NotaryID: " << notary_
                               << std::endl;
                        available_.insert(number);
                    }
                } else if (nodeName.Compare("issuedNums")) {
                    notary_ = xml->getAttributeValue("notaryID");
                    String list;
                    const bool loaded =
                        Contract::LoadEncodedTextField(raw, list);

                    if (notary_.empty() || !loaded) {
                        otErr << __FUNCTION__
                              << ": Error: issuedNums field without value."
                              << std::endl;
                        break;
                    }

                    NumList numlist;

                    if (!list.empty()) { numlist.Add(list); }

                    TransactionNumber number = 0;

                    while (numlist.Peek(number)) {
                        numlist.Pop();

                        otLog3 << "Currently liable for issued trans# "
                               << number << " at NotaryID: " << notary_
                               << std::endl;
                        issued_.insert(number);
                    }
                } else {
                    otErr << "Unknown element type in " << __FUNCTION__ << ": "
                          << nodeName << std::endl;
                }
            } break;
            default: {
                otLog5 << "Unknown XML type in " << __FUNCTION__ << ": "
                       << nodeName << std::endl;
                break;
            }
        }
    }
}

TransactionStatement::operator String() const
{
    String output;

    Tag serialized("nymData");

    serialized.add_attribute("version", version_);
    serialized.add_attribute("nymID", nym_id_);

    if (0 < issued_.size()) {
        NumList issuedList(issued_);
        String issued;
        issuedList.Output(issued);
        TagPtr issuedTag(new Tag("issuedNums", OTASCIIArmor(issued).Get()));
        issuedTag->add_attribute("notaryID", notary_);
        serialized.add_tag(issuedTag);
    }

    if (0 < available_.size()) {
        NumList availableList(available_);
        String available;
        availableList.Output(available);
        TagPtr availableTag(
            new Tag("transactionNums", OTASCIIArmor(available).Get()));
        availableTag->add_attribute("notaryID", notary_);
        serialized.add_tag(availableTag);
    }

    std::string result;
    serialized.output(result);

    return result.c_str();
}

const std::set<TransactionNumber>& TransactionStatement::Issued() const
{
    return issued_;
}

const std::string& TransactionStatement::Notary() const { return notary_; }

void TransactionStatement::Remove(const TransactionNumber& number)
{
    available_.erase(number);
    issued_.erase(number);
}
}  // namespace opentxs
