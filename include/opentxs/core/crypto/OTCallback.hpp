// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CRYPTO_OTCALLBACK_HPP
#define OPENTXS_CORE_CRYPTO_OTCALLBACK_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#ifdef SWIG
// clang-format off
%feature("director") OTCallback;
// clang-format on
#endif  // SWIG

namespace opentxs
{
class Secret;
}  // namespace opentxs

namespace opentxs
{
class OTCallback
{
public:
    // Asks for password once. (For authentication when using nym.)
    OPENTXS_EXPORT virtual void runOne(const char* szDisplay, Secret& theOutput)
        const = 0;

    // Asks for password twice. (For confirmation when changing password or
    // creating nym.)
    OPENTXS_EXPORT virtual void runTwo(const char* szDisplay, Secret& theOutput)
        const = 0;

    OPENTXS_EXPORT OTCallback() = default;
    OPENTXS_EXPORT virtual ~OTCallback() = default;
};
}  // namespace opentxs
#endif
