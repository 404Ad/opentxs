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
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/socket/Dealer.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"

using namespace opentxs;

namespace zmq = ot::network::zeromq;

namespace
{
class Test_DealerSocket : public ::testing::Test
{
public:
    const zmq::Context& context_;

    Test_DealerSocket()
        : context_(Context().ZMQ())
    {
    }
};
}  // namespace

TEST_F(Test_DealerSocket, DealerSocket_Factory)
{
    auto dealerSocket = context_.DealerSocket(
        zmq::ListenCallback::Factory(),
        zmq::socket::Socket::Direction::Connect);

    ASSERT_NE(nullptr, &dealerSocket.get());
    ASSERT_EQ(SocketType::Dealer, dealerSocket->Type());
}

// TODO: Add tests for other public member functions: SetPublicKey,
// SetSocksProxy
