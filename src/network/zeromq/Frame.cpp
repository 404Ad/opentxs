// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"              // IWYU pragma: associated
#include "1_Internal.hpp"            // IWYU pragma: associated
#include "network/zeromq/Frame.hpp"  // IWYU pragma: associated

#include <cstring>

#include "Factory.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Log.hpp"

template class opentxs::Pimpl<opentxs::network::zeromq::Frame>;

namespace opentxs
{
auto Factory::ZMQFrame() -> network::zeromq::Frame*
{
    using ReturnType = network::zeromq::implementation::Frame;

    return new ReturnType();
}

auto Factory::ZMQFrame(const void* data, const std::size_t size)
    -> network::zeromq::Frame*
{
    using ReturnType = network::zeromq::implementation::Frame;

    return new ReturnType(data, size);
}

auto Factory::ZMQFrame(const ProtobufType& data) -> network::zeromq::Frame*
{
    using ReturnType = network::zeromq::implementation::Frame;

    return new ReturnType(data);
}
}  // namespace opentxs

namespace opentxs::network::zeromq::implementation
{
Frame::Frame() noexcept
    : zeromq::Frame()
    , message_()
{
    const auto init = zmq_msg_init(&message_);

    OT_ASSERT(0 == init);
}

Frame::Frame(const std::size_t bytes) noexcept
    : Frame()
{
    const auto init = zmq_msg_init_size(&message_, bytes);

    OT_ASSERT(0 == init);
}

Frame::Frame(const ProtobufType& input) noexcept
    : Frame(input.ByteSize())
{
    input.SerializeToArray(zmq_msg_data(&message_), zmq_msg_size(&message_));
}

Frame::Frame(const void* data, const std::size_t bytes) noexcept
    : Frame(bytes)
{
    std::memcpy(zmq_msg_data(&message_), data, zmq_msg_size(&message_));
}

Frame::operator std::string() const noexcept { return std::string{Bytes()}; }

auto Frame::Bytes() const noexcept -> ReadView
{
    return ReadView{static_cast<const char*>(zmq_msg_data(&message_)),
                    zmq_msg_size(&message_)};
}

auto Frame::clone() const noexcept -> Frame*
{
    return new Frame(zmq_msg_data(&message_), zmq_msg_size(&message_));
}

Frame::~Frame() { zmq_msg_close(&message_); }
}  // namespace opentxs::network::zeromq::implementation
