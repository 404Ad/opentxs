// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "OTTestEnvironment.hpp"

#include <zmq.h>

using namespace opentxs;

TEST(Frame, Factory1)
{
    OTZMQFrame message{Factory::ZMQFrame()};

    ASSERT_NE(nullptr, &message.get());
}

TEST(Frame, Factory2)
{
    auto data = Data::Factory("0", 1);

    OTZMQFrame message{Factory::ZMQFrame(data->data(), data->size())};

    ASSERT_NE(nullptr, &message.get());
    ASSERT_EQ(message->size(), data->size());
}

TEST(Frame, operator_string)
{
    const auto test = Data::Factory("testString", 10);

    OTZMQFrame message{Factory::ZMQFrame(test->data(), test->size())};

    ASSERT_NE(nullptr, &message.get());
    std::string messageString = message.get();
    ASSERT_STREQ("testString", messageString.c_str());
}

TEST(Frame, data)
{
    OTZMQFrame message{Factory::ZMQFrame()};

    const void* data = message->data();
    ASSERT_NE(nullptr, data);

    zmq_msg_t* zmq_msg = message.get();
    ASSERT_EQ(data, zmq_msg_data(zmq_msg));
}

TEST(Frame, size)
{
    OTZMQFrame message{Factory::ZMQFrame()};

    std::size_t size = message->size();
    ASSERT_EQ(size, 0);

    zmq_msg_t* zmq_msg = message.get();
    ASSERT_EQ(zmq_msg_size(zmq_msg), 0);

    const auto test = Data::Factory("testString", 10);
    message = OTZMQFrame{Factory::ZMQFrame(test->data(), test->size())};
    size = message->size();
    ASSERT_EQ(10, size);

    zmq_msg = message.get();
    ASSERT_EQ(10, zmq_msg_size(zmq_msg));
}

TEST(Frame, zmq_msg_t)
{
    OTZMQFrame message{Factory::ZMQFrame()};

    zmq_msg_t* zmq_msg = message.get();
    ASSERT_NE(nullptr, zmq_msg);
}
