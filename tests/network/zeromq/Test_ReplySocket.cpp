// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <memory>

#include "OTTestEnvironment.hpp"  // IWYU pragma: keep
#include "opentxs/Forward.hpp"
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/ReplyCallback.hpp"
#include "opentxs/network/zeromq/socket/Reply.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"

using namespace opentxs;

namespace zmq = ot::network::zeromq;

namespace
{
class Test_ReplySocket : public ::testing::Test
{
public:
    const zmq::Context& context_;

    Test_ReplySocket()
        : context_(Context().ZMQ())
    {
    }
};
}  // namespace

TEST_F(Test_ReplySocket, ReplySocket_Factory)
{
    auto replyCallback = zmq::ReplyCallback::Factory(
        [](const zmq::Message& input) -> OTZMQMessage {
            return zmq::Message::Factory();
        });

    ASSERT_NE(nullptr, &replyCallback.get());

    auto replySocket = context_.ReplySocket(
        replyCallback, zmq::socket::Socket::Direction::Bind);

    ASSERT_NE(nullptr, &replySocket.get());
    ASSERT_EQ(SocketType::Reply, replySocket->Type());
}

// TODO: Add tests for other public member functions: SetPrivateKey
