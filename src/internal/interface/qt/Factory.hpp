// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs
{
namespace api
{
namespace session
{
class Client;
}  // namespace session
}  // namespace api

namespace ui
{
class IdentityManagerQt;
}  // namespace ui
}  // namespace opentxs

namespace opentxs::factory
{
auto IdentityManagerQt(const api::session::Client& api) noexcept
    -> ui::IdentityManagerQt;
}  // namespace opentxs::factory
