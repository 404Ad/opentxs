// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                       // IWYU pragma: associated
#include "1_Internal.hpp"                     // IWYU pragma: associated
#include "blockchain/client/PeerManager.hpp"  // IWYU pragma: associated

#include <map>
#include <memory>
#include <utility>

#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Publish.hpp"
#include "opentxs/network/zeromq/socket/Push.hpp"
#include "opentxs/network/zeromq/socket/Sender.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"

// #define OT_METHOD
// "opentxs::blockchain::client::implementation::PeerManager::Jobs::"

namespace opentxs::blockchain::client::implementation
{
PeerManager::Jobs::Jobs(const api::Core& api) noexcept
    : zmq_(api.ZeroMQ())
    , getheaders_(api.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Bind))
    , getcfheaders_(
          api.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Bind))
    , getcfilters_(
          api.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Bind))
    , heartbeat_(api.ZeroMQ().PublishSocket())
    , getblock_(api.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Bind))
    , broadcast_transaction_(
          api.ZeroMQ().PushSocket(zmq::socket::Socket::Direction::Bind))
    , endpoint_map_()
    , socket_map_({
          {Task::Getheaders, &getheaders_.get()},
          {Task::Getcfheaders, &getcfheaders_.get()},
          {Task::Getcfilters, &getcfilters_.get()},
          {Task::Heartbeat, &heartbeat_.get()},
          {Task::Getblock, &getblock_.get()},
          {Task::BroadcastTransaction, &broadcast_transaction_.get()},
      })
{
    // NOTE endpoint_map_ should never be modified after construction
    listen(Task::Getheaders, getheaders_);
    listen(Task::Getcfheaders, getcfheaders_);
    listen(Task::Getcfilters, getcfilters_);
    listen(Task::Heartbeat, heartbeat_);
    listen(Task::Getblock, getblock_);
    listen(Task::BroadcastTransaction, broadcast_transaction_);
}

auto PeerManager::Jobs::Dispatch(const Task type) noexcept -> void
{
    Dispatch(Work(type));
}

auto PeerManager::Jobs::Dispatch(zmq::Message& work) noexcept -> void
{
    const auto body = work.Body();

    OT_ASSERT(0 < body.size());

    socket_map_.at(body.at(0).as<Task>())->Send(work);
}

auto PeerManager::Jobs::Endpoint(const Task type) const noexcept -> std::string
{
    try {

        return endpoint_map_.at(type);
    } catch (...) {

        return {};
    }
}

auto PeerManager::Jobs::listen(
    const Task type,
    const zmq::socket::Sender& socket) noexcept -> void
{
    auto& map = const_cast<EndpointMap&>(endpoint_map_);
    auto [it, added] = map.emplace(
        type,
        std::string{"inproc://opentxs//blockchain/peer_tasks/"} +
            Identifier::Random()->str());

    OT_ASSERT(added);

    const auto listen = socket.Start(it->second);

    OT_ASSERT(listen);
}

auto PeerManager::Jobs::Shutdown() noexcept -> void
{
    for (auto [type, socket] : socket_map_) { socket->Close(); }
}

auto PeerManager::Jobs::Work(const Task task, std::promise<void>* promise)
    const noexcept -> OTZMQMessage
{
    if (nullptr != promise) {
        return zmq_.TaggedReply(
            ReadView{reinterpret_cast<char*>(promise), sizeof(promise)}, task);
    } else {
        return zmq_.TaggedMessage(task);
    }
}
}  // namespace opentxs::blockchain::client::implementation
