// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

namespace opentxs
{
namespace network
{
class DhtConfig;
class OpenDHT;
}  // namespace network
}  // namespace opentxs

namespace opentxs::factory
{
auto OpenDHT(const network::DhtConfig& config) noexcept
    -> std::unique_ptr<network::OpenDHT>;
}  // namespace opentxs::factory
