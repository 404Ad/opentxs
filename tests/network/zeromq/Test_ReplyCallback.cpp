// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/opentxs.hpp"

#include <gtest/gtest.h>

using namespace opentxs;

namespace
{
class Test_ReplyCallback : public ::testing::Test
{
public:
    const std::string testMessage_{"zeromq test message"};
};

}  // namespace

TEST(ReplyCallback, ReplyCallback_Factory)
{
    auto replyCallback = network::zeromq::ReplyCallback::Factory(
        [this](const network::zeromq::Message& input) -> OTZMQMessage {
            return network::zeromq::Message::ReplyFactory(input);
        });

    ASSERT_NE(nullptr, &replyCallback.get());
}

TEST_F(Test_ReplyCallback, ReplyCallback_Process)
{
    auto replyCallback = network::zeromq::ReplyCallback::Factory(
        [this](const network::zeromq::Message& input) -> OTZMQMessage {
            const std::string& inputString = *input.Body().begin();
            EXPECT_EQ(testMessage_, inputString);

            auto reply = network::zeromq::Message::ReplyFactory(input);
            reply->AddFrame(inputString);
            return reply;
        });

    ASSERT_NE(nullptr, &replyCallback.get());

    auto testMessage = network::zeromq::Message::Factory();
    testMessage->AddFrame(testMessage_);

    ASSERT_NE(nullptr, &testMessage.get());

    auto message = replyCallback->Process(testMessage);

    const std::string& messageString = *message->Body().begin();
    ASSERT_EQ(testMessage_, messageString);
}
