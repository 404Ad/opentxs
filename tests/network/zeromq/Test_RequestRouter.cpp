// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/opentxs.hpp"

#include <gtest/gtest.h>

using namespace opentxs;

namespace
{

class Test_RequestRouter : public ::testing::Test
{
public:
    static OTZMQContext context_;

    const std::string testMessage_{"zeromq test message"};
    const std::string testMessage2_{"zeromq test message 2"};
    const std::string testMessage3_{"zeromq test message 3"};

    const std::string endpoint_{"inproc://opentxs/test/request_router_test"};

    std::atomic_int callbackFinishedCount_{0};

    int callbackCount_{0};

    void requestSocketThread(const std::string& msg);
    void requestSocketThreadMultipart();
};

OTZMQContext Test_RequestRouter::context_{network::zeromq::Context::Factory()};

void Test_RequestRouter::requestSocketThread(const std::string& msg)
{
    ASSERT_NE(nullptr, &Test_RequestRouter::context_.get());

    auto requestSocket =
        network::zeromq::RequestSocket::Factory(Test_RequestRouter::context_);

    ASSERT_NE(nullptr, &requestSocket.get());
    ASSERT_EQ(SocketType::Request, requestSocket->Type());

    requestSocket->SetTimeouts(
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(-1),
        std::chrono::milliseconds(30000));
    requestSocket->Start(endpoint_);

    auto [result, message] = requestSocket->SendRequest(msg);

    ASSERT_EQ(result, SendResult::VALID_REPLY);
    // RouterSocket removes the identity frame and RequestSocket removes the
    // delimiter.
    ASSERT_EQ(1, message->size());

    const std::string& messageString = *message->Body().begin();
    ASSERT_EQ(msg, messageString);
}

void Test_RequestRouter::requestSocketThreadMultipart()
{
    ASSERT_NE(nullptr, &Test_RequestRouter::context_.get());

    auto requestSocket =
        network::zeromq::RequestSocket::Factory(Test_RequestRouter::context_);

    ASSERT_NE(nullptr, &requestSocket.get());
    ASSERT_EQ(SocketType::Request, requestSocket->Type());

    requestSocket->SetTimeouts(
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(-1),
        std::chrono::milliseconds(30000));
    requestSocket->Start(endpoint_);

    auto multipartMessage = network::zeromq::Message::Factory(testMessage_);
    multipartMessage->AddFrame();
    multipartMessage->AddFrame(testMessage2_);
    multipartMessage->AddFrame(testMessage3_);

    auto [result, message] = requestSocket->SendRequest(multipartMessage);

    ASSERT_EQ(result, SendResult::VALID_REPLY);
    // RouterSocket removes the identity frame and RequestSocket removes the
    // delimiter.
    ASSERT_EQ(4, message->size());

    const std::string& messageHeader = *message->Header().begin();

    ASSERT_EQ(testMessage_, messageHeader);

    for (const std::string& frame : message->Body()) {
        bool match = frame == testMessage2_ || frame == testMessage3_;
        ASSERT_TRUE(match);
    }
}

}  // namespace

TEST_F(Test_RequestRouter, Request_Router)
{
    ASSERT_NE(nullptr, &Test_RequestRouter::context_.get());

    auto replyMessage = network::zeromq::Message::Factory();

    auto routerCallback = network::zeromq::ListenCallback::Factory(
        [this, &replyMessage](network::zeromq::Message& input) -> void {
            // RequestSocket prepends a delimiter and RouterSocket prepends an
            // identity frame.
            EXPECT_EQ(3, input.size());
            EXPECT_EQ(1, input.Header().size());
            EXPECT_EQ(1, input.Body().size());

            const std::string& inputString = *input.Body().begin();

            EXPECT_EQ(testMessage_, inputString);

            replyMessage = network::zeromq::Message::ReplyFactory(input);
            for (const std::string& frame : input.Body()) {
                replyMessage->AddFrame(frame);
            }

            ++callbackFinishedCount_;
        });

    ASSERT_NE(nullptr, &routerCallback.get());

    auto routerSocket = network::zeromq::RouterSocket::Factory(
        Test_RequestRouter::context_, false, routerCallback);

    ASSERT_NE(nullptr, &routerSocket.get());
    ASSERT_EQ(SocketType::Router, routerSocket->Type());

    routerSocket->SetTimeouts(
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(30000),
        std::chrono::milliseconds(-1));
    routerSocket->Start(endpoint_);

    // Send the request on a separate thread so this thread can continue and
    // wait for the ListenCallback to finish, then send the reply.
    std::thread requestSocketThread1(
        &Test_RequestRouter::requestSocketThread, this, testMessage_);

    auto end = std::time(nullptr) + 5;
    while (!callbackFinishedCount_ && std::time(nullptr) < end)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(1, callbackFinishedCount_);

    routerSocket->Send(replyMessage.get());

    requestSocketThread1.join();
}

TEST_F(Test_RequestRouter, Request_2_Router_1)
{
    callbackCount_ = 2;

    ASSERT_NE(nullptr, &Test_RequestRouter::context_.get());

    std::map<std::string, OTZMQMessage> replyMessages{
        std::pair<std::string, OTZMQMessage>(
            testMessage2_, network::zeromq::Message::Factory()),
        std::pair<std::string, OTZMQMessage>(
            testMessage3_, network::zeromq::Message::Factory())};

    auto routerCallback = network::zeromq::ListenCallback::Factory(
        [this, &replyMessages](network::zeromq::Message& input) -> void {
            // RequestSocket prepends a delimiter and RouterSocket prepends an
            // identity frame.
            EXPECT_EQ(3, input.size());
            EXPECT_EQ(1, input.Header().size());
            EXPECT_EQ(1, input.Body().size());

            const std::string& inputString = *input.Body().begin();
            bool match =
                inputString == testMessage2_ || inputString == testMessage3_;
            EXPECT_TRUE(match);

            auto& replyMessage = replyMessages.at(inputString);
            replyMessage = network::zeromq::Message::ReplyFactory(input);
            for (const std::string& frame : input.Body()) {
                replyMessage->AddFrame(frame);
            }

            ++callbackFinishedCount_;
        });

    ASSERT_NE(nullptr, &routerCallback.get());

    auto routerSocket = network::zeromq::RouterSocket::Factory(
        Test_RequestRouter::context_, false, routerCallback);

    ASSERT_NE(nullptr, &routerSocket.get());
    ASSERT_EQ(SocketType::Router, routerSocket->Type());

    routerSocket->SetTimeouts(
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(-1),
        std::chrono::milliseconds(30000));
    routerSocket->Start(endpoint_);

    std::thread requestSocketThread1(
        &Test_RequestRouter::requestSocketThread, this, testMessage2_);
    std::thread requestSocketThread2(
        &Test_RequestRouter::requestSocketThread, this, testMessage3_);

    auto& replyMessage1 = replyMessages.at(testMessage2_);
    auto& replyMessage2 = replyMessages.at(testMessage3_);

    auto end = std::time(nullptr) + 15;
    while (!callbackFinishedCount_ && std::time(nullptr) < end)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool message1Sent{false};
    if (0 != replyMessage1->size()) {
        routerSocket->Send(replyMessage1);
        message1Sent = true;
    } else {
        routerSocket->Send(replyMessage2);
    }

    end = std::time(nullptr) + 15;
    while (callbackFinishedCount_ < callbackCount_ && std::time(nullptr) < end)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (false == message1Sent) {
        routerSocket->Send(replyMessage1);
    } else {
        routerSocket->Send(replyMessage2);
    }

    ASSERT_EQ(callbackCount_, callbackFinishedCount_);

    requestSocketThread1.join();
    requestSocketThread2.join();
}

TEST_F(Test_RequestRouter, Request_Router_Multipart)
{
    ASSERT_NE(nullptr, &Test_RequestRouter::context_.get());

    auto replyMessage = network::zeromq::Message::Factory();

    auto routerCallback = network::zeromq::ListenCallback::Factory(
        [this, &replyMessage](network::zeromq::Message& input) -> void {
            // RequestSocket prepends a delimiter and RouterSocket prepends an
            // identity frame.
            EXPECT_EQ(6, input.size());
            // Identity frame.
            EXPECT_EQ(1, input.Header().size());
            // Original message: header, delimiter, two body parts.
            EXPECT_EQ(4, input.Body().size());

            for (const std::string& frame : input.Body()) {
                bool match = frame == testMessage_ || frame == testMessage2_ ||
                             frame == testMessage3_;
                EXPECT_TRUE(match || frame.size() == 0);
            }

            replyMessage = network::zeromq::Message::ReplyFactory(input);
            for (auto& frame : input.Body()) { replyMessage->AddFrame(frame); }
        });

    ASSERT_NE(nullptr, &routerCallback.get());

    auto routerSocket = network::zeromq::RouterSocket::Factory(
        Test_RequestRouter::context_, false, routerCallback);

    ASSERT_NE(nullptr, &routerSocket.get());
    ASSERT_EQ(SocketType::Router, routerSocket->Type());

    routerSocket->SetTimeouts(
        std::chrono::milliseconds(0),
        std::chrono::milliseconds(30000),
        std::chrono::milliseconds(-1));
    routerSocket->Start(endpoint_);

    // Send the request on a separate thread so this thread can continue and
    // wait for the ListenCallback to finish, then send the reply.
    std::thread requestSocketThread1(
        &Test_RequestRouter::requestSocketThreadMultipart, this);

    auto end = std::time(nullptr) + 15;
    while (0 == replyMessage->size() && std::time(nullptr) < end)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto sent = routerSocket->Send(replyMessage.get());

    ASSERT_TRUE(sent);

    requestSocketThread1.join();
}
