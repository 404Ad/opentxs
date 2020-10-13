// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"            // IWYU pragma: associated
#include "1_Internal.hpp"          // IWYU pragma: associated
#include "display/Definition.hpp"  // IWYU pragma: associated

#include <iosfwd>
#include <stdexcept>

#include "display/Definition_imp.hpp"
#include "opentxs/core/Log.hpp"

namespace opentxs::display
{
Definition::Definition(Scales&& scales) noexcept
    : imp_(std::make_unique<Imp>(std::move(scales)))
{
    OT_ASSERT(imp_);
}

auto Definition::Format(
    const Amount amount,
    const Index index,
    const OptionalInt minDecimals,
    const OptionalInt maxDecimals) const noexcept(false) -> std::string
{
    try {
        const auto& scale = imp_->scales_.at(static_cast<std::size_t>(index));

        return scale.second.Format(amount, minDecimals, maxDecimals);
    } catch (...) {
        throw std::out_of_range("Invalid scale index");
    }
}

auto Definition::GetScales() const noexcept -> std::map<Index, Name>
{
    auto output = std::map<Index, Name>{};
    auto index = Index{0};

    for (const auto& [name, scale] : imp_->scales_) {
        output.emplace(index++, name);
    }

    return output;
}

auto Definition::Import(const std::string& formatted, const Index scale) const
    noexcept(false) -> Amount
{
    return imp_->Import(formatted, scale);
}

Definition::~Definition() = default;
}  // namespace opentxs::display
