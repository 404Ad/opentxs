// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                         // IWYU pragma: associated
#include "1_Internal.hpp"                       // IWYU pragma: associated
#include "network/zeromq/socket/Subscribe.hpp"  // IWYU pragma: associated

#include <zmq.h>

#include "Factory.hpp"
#include "network/zeromq/socket/Receiver.tpp"
#include "network/zeromq/socket/Socket.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"

namespace opentxs
{
namespace network
{
namespace zeromq
{
namespace socket
{
class Subscribe;
}  // namespace socket

class Context;
class Message;
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

template class opentxs::Pimpl<opentxs::network::zeromq::socket::Subscribe>;

#define OT_METHOD                                                              \
    "opentxs::network::zeromq::socket::implementation::Subscribe::"

namespace opentxs
{
auto Factory::SubscribeSocket(
    const network::zeromq::Context& context,
    const network::zeromq::ListenCallback& callback)
    -> network::zeromq::socket::Subscribe*
{
    using ReturnType = network::zeromq::socket::implementation::Subscribe;

    return new ReturnType(context, callback);
}
}  // namespace opentxs

namespace opentxs::network::zeromq::socket::implementation
{
Subscribe::Subscribe(
    const zeromq::Context& context,
    const zeromq::ListenCallback& callback) noexcept
    : Receiver(context, SocketType::Subscribe, Socket::Direction::Connect, true)
    , Client(this->get())
    , callback_(callback)
{
    init();
}

auto Subscribe::clone() const noexcept -> Subscribe*
{
    return new Subscribe(context_, callback_);
}

auto Subscribe::have_callback() const noexcept -> bool { return true; }

void Subscribe::init() noexcept
{
    // subscribe to all messages until filtering is implemented
    const auto set = zmq_setsockopt(socket_, ZMQ_SUBSCRIBE, "", 0);
    Receiver::init();

    OT_ASSERT(0 == set);
}

void Subscribe::process_incoming(const Lock& lock, Message& message) noexcept
{
    OT_ASSERT(verify_lock(lock))

    LogDetail(OT_METHOD)(__FUNCTION__)(
        ": Incoming messaged received. Triggering callback.")
        .Flush();
    try {
        callback_.Process(message);
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Callback failed").Flush();
    }

    LogDetail(OT_METHOD)(__FUNCTION__)(" Done.").Flush();
}

auto Subscribe::SetSocksProxy(const std::string& proxy) const noexcept -> bool
{
    return set_socks_proxy(proxy);
}

Subscribe::~Subscribe() SHUTDOWN
}  // namespace opentxs::network::zeromq::socket::implementation
