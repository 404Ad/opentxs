// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CRYPTO_UTIL_HPP
#define OPENTXS_API_CRYPTO_UTIL_HPP

#include "opentxs/Forward.hpp"

#include <cstdint>

namespace opentxs
{
namespace api
{
namespace crypto
{

class Util
{
public:
    virtual bool RandomizeMemory(void* destination, const std::size_t size)
        const = 0;

    virtual ~Util() = default;

protected:
    Util() = default;

private:
    Util(const Util&) = delete;
    Util(Util&&) = delete;
    Util& operator=(const Util&) = delete;
    Util& operator=(Util&&) = delete;
};
}  // namespace crypto
}  // namespace api
}  // namespace opentxs
#endif  // OPENTXS_API_CRYPTO_UTIL_HPP
