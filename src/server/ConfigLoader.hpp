// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs
{
namespace api
{
class Settings;

namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

class String;
}  // namespace opentxs

namespace opentxs::server
{
struct ConfigLoader {
    static auto load(
        const api::internal::Core& api,
        const api::Settings& config,
        String& walletFilename) -> bool;
};
}  // namespace opentxs::server
