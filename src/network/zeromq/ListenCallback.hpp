// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/network/zeromq/ListenCallback.cpp"

#pragma once

#include "opentxs/network/zeromq/ListenCallback.hpp"

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Message;
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

namespace opentxs::network::zeromq::implementation
{
class ListenCallback final : virtual public zeromq::ListenCallback
{
public:
    void Process(zeromq::Message& message) const final;

    ~ListenCallback() final;

private:
    friend zeromq::ListenCallback;

    const zeromq::ListenCallback::ReceiveCallback callback_;

    ListenCallback* clone() const final;

    ListenCallback(zeromq::ListenCallback::ReceiveCallback callback);
    ListenCallback() = delete;
    ListenCallback(const ListenCallback&) = delete;
    ListenCallback(ListenCallback&&) = delete;
    ListenCallback& operator=(const ListenCallback&) = delete;
    ListenCallback& operator=(ListenCallback&&) = delete;
};
}  // namespace opentxs::network::zeromq::implementation
