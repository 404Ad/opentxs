// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_SCRIPT_OTSTASHITEM_HPP
#define OPENTXS_CORE_SCRIPT_OTSTASHITEM_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/String.hpp"

#include <cstdint>

namespace opentxs
{

class Identifier;

class OTStashItem
{
    String m_strInstrumentDefinitionID;
    std::int64_t m_lAmount;

public:
    std::int64_t GetAmount() const { return m_lAmount; }
    void SetAmount(std::int64_t lAmount) { m_lAmount = lAmount; }
    bool CreditStash(const std::int64_t& lAmount);
    bool DebitStash(const std::int64_t& lAmount);
    const String& GetInstrumentDefinitionID()
    {
        return m_strInstrumentDefinitionID;
    }
    OTStashItem();
    OTStashItem(
        const String& strInstrumentDefinitionID,
        std::int64_t lAmount = 0);
    OTStashItem(
        const Identifier& theInstrumentDefinitionID,
        std::int64_t lAmount = 0);
    virtual ~OTStashItem();
};

}  // namespace opentxs

#endif
