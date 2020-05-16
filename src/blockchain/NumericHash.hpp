// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/blockchain/NumericHash.cpp"

#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <iosfwd>
#include <string>

#include "opentxs/blockchain/NumericHash.hpp"

namespace opentxs
{
class Factory;
}  // namespace opentxs

namespace mp = boost::multiprecision;

namespace opentxs::blockchain::implementation
{
class NumericHash : virtual public blockchain::NumericHash
{
public:
    using Type = mp::checked_cpp_int;

    auto operator==(const blockchain::NumericHash& rhs) const noexcept
        -> bool final;
    auto operator!=(const blockchain::NumericHash& rhs) const noexcept
        -> bool final;
    auto operator<(const blockchain::NumericHash& rhs) const noexcept
        -> bool final;
    auto operator<=(const blockchain::NumericHash& rhs) const noexcept
        -> bool final;

    auto asHex(const std::size_t minimumBytes) const noexcept
        -> std::string final;
    auto Decimal() const noexcept -> std::string final { return data_.str(); }

    ~NumericHash() final = default;

private:
    friend opentxs::Factory;

    Type data_;

    auto clone() const noexcept -> NumericHash* final
    {
        return new NumericHash(*this);
    }

    NumericHash(const Type& data) noexcept;
    NumericHash() noexcept;
    NumericHash(const NumericHash& rhs) noexcept;
    NumericHash(NumericHash&& rhs) = delete;
    auto operator=(const NumericHash& rhs) -> NumericHash& = delete;
    auto operator=(NumericHash&& rhs) -> NumericHash& = delete;
};
}  // namespace opentxs::blockchain::implementation
