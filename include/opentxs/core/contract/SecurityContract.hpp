// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_SECURITYCONTRACT_HPP
#define OPENTXS_CORE_SECURITYCONTRACT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/contract/UnitDefinition.hpp"

namespace opentxs
{

class Nym;

class SecurityContract : public UnitDefinition
{
private:
    typedef UnitDefinition ot_super;
    friend ot_super;

    SecurityContract(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const proto::UnitDefinition serialized);
    SecurityContract(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const std::string& shortname,
        const std::string& name,
        const std::string& symbol,
        const std::string& terms);

    proto::UnitDefinition IDVersion(const Lock& lock) const override;

public:
    EXPORT proto::UnitType Type() const override
    {
        return proto::UNITTYPE_SECURITY;
    }

    EXPORT std::string TLA() const override { return primary_unit_symbol_; }

    virtual ~SecurityContract() = default;
};
}  // namespace opentxs
#endif  // OPENTXS_CORE_SECURITYCONTRACT_HPP
