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

#ifndef OPENTXS_CONTACT_CONTACT_DATA_HPP
#define OPENTXS_CONTACT_CONTACT_DATA_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>

namespace opentxs
{

class ContactGroup;
class ContactItem;
class ContactSection;
class Identifier;

class ContactData
{
public:
    typedef std::map<proto::ContactSectionName, std::shared_ptr<ContactSection>>
        SectionMap;

    static std::string PrintContactData(const proto::ContactData& data);

    ContactData(
        const std::string& nym,
        const std::uint32_t version,
        const std::uint32_t targetVersion,
        const SectionMap& sections);
    ContactData(
        const std::string& nym,
        const std::uint32_t targetVersion,
        const proto::ContactData& serialized);
    ContactData(const ContactData&) = default;
    ContactData(ContactData&&) = default;

    ContactData operator+(const ContactData& rhs) const;

    operator std::string() const;

    ContactData AddContract(
        const std::string& instrumentDefinitionID,
        const proto::ContactItemType currency,
        const bool primary,
        const bool active) const;
    ContactData AddEmail(
        const std::string& value,
        const bool primary,
        const bool active) const;
    ContactData AddItem(const Claim& claim) const;
    ContactData AddItem(const std::shared_ptr<ContactItem>& item) const;
    ContactData AddPaymentCode(
        const std::string& code,
        const proto::ContactItemType currency,
        const bool primary,
        const bool active) const;
    ContactData AddPhoneNumber(
        const std::string& value,
        const bool primary,
        const bool active) const;
    ContactData AddPreferredOTServer(const Identifier& id, const bool primary)
        const;
    ContactData AddSocialMediaProfile(
        const std::string& value,
        const proto::ContactItemType type,
        const bool primary,
        const bool active) const;
    SectionMap::const_iterator begin() const { return sections_.begin(); }
    std::string BestEmail() const;
    std::string BestPhoneNumber() const;
    std::string BestSocialMediaProfile(const proto::ContactItemType type) const;
    std::shared_ptr<ContactItem> Claim(const Identifier& item) const;
    std::set<OTIdentifier> Contracts(
        const proto::ContactItemType currency,
        const bool onlyActive) const;
    ContactData Delete(const Identifier& id) const;
    std::string EmailAddresses(bool active = true) const;
    SectionMap::const_iterator end() const { return sections_.end(); }
    std::shared_ptr<ContactGroup> Group(
        const proto::ContactSectionName& section,
        const proto::ContactItemType& type) const;
    bool HaveClaim(const Identifier& item) const;
    bool HaveClaim(
        const proto::ContactSectionName& section,
        const proto::ContactItemType& type,
        const std::string& value) const;
    std::string Name() const;
    std::string PhoneNumbers(bool active = true) const;
    OTIdentifier PreferredOTServer() const;
    std::shared_ptr<ContactSection> Section(
        const proto::ContactSectionName& section) const;
    proto::ContactData Serialize(const bool withID = false) const;
    ContactData SetCommonName(const std::string& name) const;
    ContactData SetName(const std::string& name, const bool primary = true)
        const;
    ContactData SetScope(
        const proto::ContactItemType type,
        const std::string& name) const;
    std::string SocialMediaProfiles(
        const proto::ContactItemType type,
        bool active = true) const;
    const std::set<proto::ContactItemType> SocialMediaProfileTypes() const;
    proto::ContactItemType Type() const;
    std::uint32_t Version() const;

    ~ContactData() = default;

private:
    typedef std::
        pair<proto::ContactItemType, std::shared_ptr<const ContactGroup>>
            Scope;

    const std::uint32_t version_{0};
    const std::string nym_{};
    const SectionMap sections_{};

    static std::uint32_t check_version(
        const std::uint32_t in,
        const std::uint32_t targetVersion);
    static SectionMap extract_sections(
        const std::string& nym,
        const std::uint32_t targetVersion,
        const proto::ContactData& serialized);

    Scope scope() const;

    ContactData() = delete;
    ContactData& operator=(const ContactData&) = delete;
    ContactData& operator=(ContactData&&) = delete;
};
}  // namespace opentxs

#endif  // OPENTXS_CONTACT_CONTACT_DATA_HPP
