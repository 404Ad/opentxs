// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_SERVER_CONFIGLOADER_HPP
#define OPENTXS_SERVER_CONFIGLOADER_HPP

#include "Internal.hpp"

namespace opentxs
{
class String;

namespace api
{
class Crypto;
class Settings;
}  // namespace api

namespace server
{

struct ConfigLoader {
    static bool load(
        const api::Crypto& crypto,
        const api::Settings& config,
        String& walletFilename);
};
}  // namespace server
}  // namespace opentxs

#endif  // OPENTXS_SERVER_CONFIGLOADER_HPP
