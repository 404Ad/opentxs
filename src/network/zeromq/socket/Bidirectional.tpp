// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "network/zeromq/socket/Bidirectional.hpp"  // IWYU pragma: associated

#include <zmq.h>
#include <memory>
#include <mutex>
#include <thread>

#include "network/zeromq/socket/Receiver.tpp"
#include "network/zeromq/socket/Sender.tpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameIterator.hpp"
#include "opentxs/network/zeromq/Message.hpp"

#define CALLBACK_WAIT_MILLISECONDS 50
#define POLL_MILLISECONDS 100

#define OT_METHOD_BIDIRECTIONAL                                                \
    "opentxs::network::zeromq::socket::implementation::Bidirectional::"

namespace opentxs::network::zeromq::socket::implementation
{
template <typename InterfaceType, typename MessageType>
Bidirectional<InterfaceType, MessageType>::Bidirectional(
    const zeromq::Context& context,
    const bool startThread) noexcept
    : bidirectional_start_thread_(startThread)
    , endpoint_(Socket::random_inproc_endpoint())
    , push_socket_(zmq_socket(context, ZMQ_PUSH))
    , pull_socket_(zmq_socket(context, ZMQ_PULL))
    , linger_(0)
    , send_timeout_(-1)
    , receive_timeout_(-1)
    , send_lock_()
{
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::apply_timeouts(
    void* socket,
    std::mutex& socket_mutex) const noexcept -> bool
{
    OT_ASSERT(nullptr != socket);
    Lock lock(socket_mutex);

    auto set = zmq_setsockopt(socket, ZMQ_LINGER, &linger_, sizeof(linger_));

    if (0 != set) {
        LogOutput(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(
            ": Failed to set ZMQ_LINGER.")
            .Flush();

        return false;
    }

    set = zmq_setsockopt(
        socket, ZMQ_SNDTIMEO, &send_timeout_, sizeof(send_timeout_));

    if (0 != set) {
        LogOutput(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(
            ": Failed to set ZMQ_SNDTIMEO.")
            .Flush();

        return false;
    }

    set = zmq_setsockopt(
        socket, ZMQ_RCVTIMEO, &receive_timeout_, sizeof(receive_timeout_));

    if (0 != set) {
        LogOutput(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(
            ": Failed to set ZMQ_RCVTIMEO.")
            .Flush();

        return false;
    }

    return true;
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::bind(
    void* socket,
    std::mutex& socket_mutex,
    const std::string& endpoint) const noexcept -> bool
{
    apply_timeouts(socket, socket_mutex);

    return (0 == zmq_bind(socket, endpoint.c_str()));
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::connect(
    void* socket,
    std::mutex& socket_mutex,
    const std::string& endpoint) const noexcept -> bool
{
    apply_timeouts(socket, socket_mutex);

    return (0 == zmq_connect(socket, endpoint.c_str()));
}

template <typename InterfaceType, typename MessageType>
void Bidirectional<InterfaceType, MessageType>::init() noexcept
{
    OT_ASSERT(nullptr != pull_socket_);
    OT_ASSERT(nullptr != push_socket_);

    auto bound = bind(pull_socket_, this->lock_, endpoint_);

    if (false == bound) {
        pull_socket_ = nullptr;
        push_socket_ = nullptr;
        std::cerr << OT_METHOD_BIDIRECTIONAL << __FUNCTION__ << ": "
                  << zmq_strerror(zmq_errno()) << std::endl;
        return;
    }

    auto connected = connect(push_socket_, this->lock_, endpoint_);

    if (false == connected) {
        pull_socket_ = nullptr;
        push_socket_ = nullptr;
        std::cerr << OT_METHOD_BIDIRECTIONAL << __FUNCTION__ << ": "
                  << zmq_strerror(zmq_errno()) << std::endl;
        return;
    }

    Socket::init();

    if (bidirectional_start_thread_) {
        this->receiver_thread_ = std::thread(
            &Bidirectional<InterfaceType, MessageType>::thread, this);
    }
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::process_pull_socket(
    const Lock& lock) noexcept -> bool
{
    auto msg = Message::Factory();
    const auto received = Socket::receive_message(lock, pull_socket_, msg);

    if (false == received) { return false; }

    const auto sent = send(lock, msg);

    return sent;
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::process_receiver_socket(
    const Lock& lock) noexcept -> bool
{
    auto reply = Message::Factory();
    const auto received = Socket::receive_message(lock, this->socket_, reply);

    if (false == received) { return false; }

    this->process_incoming(lock, reply);

    return true;
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::send(
    zeromq::Message& message) const noexcept -> bool
{
    Lock lock(send_lock_);

    if (false == this->running_.get()) { return false; }

    OT_ASSERT(nullptr != push_socket_);

    return Socket::send_message(lock, push_socket_, message);
}

template <typename InterfaceType, typename MessageType>
auto Bidirectional<InterfaceType, MessageType>::send(
    const Lock& lock,
    zeromq::Message& message) noexcept -> bool
{
    return Socket::send_message(lock, this->socket_, message);
}

template <typename InterfaceType, typename MessageType>
void Bidirectional<InterfaceType, MessageType>::shutdown(
    const Lock& lock) noexcept
{
    Lock send(send_lock_);

    if (this->running_.get()) {
        zmq_disconnect(push_socket_, endpoint_.c_str());
        zmq_unbind(pull_socket_, endpoint_.c_str());
    }

    this->running_->Off();
    send.unlock();
    Receiver<InterfaceType, MessageType>::shutdown(lock);
}

template <typename InterfaceType, typename MessageType>
void Bidirectional<InterfaceType, MessageType>::thread() noexcept
{
    LogVerbose(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(": Starting listener")
        .Flush();

    while (this->running_.get()) {
        if (this->have_callback()) { break; }

        Sleep(std::chrono::milliseconds(CALLBACK_WAIT_MILLISECONDS));
    }

    LogVerbose(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(": Callback ready")
        .Flush();

    while (this->running_.get()) {
        std::this_thread::yield();
        auto newEndpoints = this->endpoint_queue_.pop();
        Lock lock(this->lock_, std::try_to_lock);
        zmq_pollitem_t poll[2]{};
        poll[0].socket = this->socket_;
        poll[0].events = ZMQ_POLLIN;
        poll[1].socket = pull_socket_;
        poll[1].events = ZMQ_POLLIN;

        if (false == lock.owns_lock()) { continue; }

        for (const auto& endpoint : newEndpoints) {
            this->start(lock, endpoint);
        }

        this->run_tasks(lock);
        const auto events = zmq_poll(poll, 2, POLL_MILLISECONDS);

        if (0 == events) {
            LogVerbose(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(": No messages.")
                .Flush();

            continue;
        }

        if (-1 == events) {
            const auto error = zmq_errno();
            LogOutput(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(": Poll error: ")(
                zmq_strerror(error))(".")
                .Flush();

            continue;
        }

        bool processed = true;

        if (ZMQ_POLLIN == poll[0].revents) {
            processed = process_receiver_socket(lock);
        }

        if (processed && ZMQ_POLLIN == poll[1].revents) {
            processed = process_pull_socket(lock);
        }
    }

    LogVerbose(OT_METHOD_BIDIRECTIONAL)(__FUNCTION__)(": Shutting down")
        .Flush();
}
}  // namespace opentxs::network::zeromq::socket::implementation
