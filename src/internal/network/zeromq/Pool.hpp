// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Context;
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

namespace opentxs::network::zeromq::internal
{
class Pool
{
public:
    virtual auto Parent() const noexcept -> const zeromq::Context& = 0;

    virtual auto DoModify(SocketID id, ModifyCallback& cb) noexcept -> bool = 0;
    virtual auto UpdateIndex(BatchID id, StartArgs&& sockets) noexcept
        -> void = 0;
    virtual auto UpdateIndex(BatchID id) noexcept -> void = 0;

    virtual ~Pool() = default;
};
}  // namespace opentxs::network::zeromq::internal