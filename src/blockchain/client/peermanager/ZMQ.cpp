// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                   // IWYU pragma: associated
#include "1_Internal.hpp"                 // IWYU pragma: associated
#include "IncomingConnectionManager.hpp"  // IWYU pragma: associated

#include <iosfwd>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "internal/blockchain/Params.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "internal/blockchain/p2p/P2P.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/blockchain/p2p/Address.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/socket/Router.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"
#include "opentxs/util/WorkType.hpp"
#include "util/Work.hpp"

#define OT_METHOD                                                              \
    "opentxs::blockchain::client::implementation::"                            \
    "ZMQIncomingConnectionManager::"

namespace opentxs::blockchain::client::implementation
{
class ZMQIncomingConnectionManager
    : public PeerManager::IncomingConnectionManager
{
public:
    auto Disconnect(const int id) const noexcept -> void final
    {
        Lock lock(lock_);
        auto it = peers_.find(id);

        if (peers_.end() == it) { return; }

        const auto& [externalID, internalID, cached] = it->second;
        external_index_.erase(externalID);
        internal_index_.erase(internalID);
        peers_.erase(it);
    }
    auto Listen(const p2p::Address& address) const noexcept -> bool final
    {
        if (p2p::Network::zmq != address.Type()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid address").Flush();

            return false;
        }

        return external_->Start(
            address.Bytes()->str() + ':' + std::to_string(address.Port()));
    }

    ZMQIncomingConnectionManager(
        const api::Core& api,
        PeerManager::Peers& parent) noexcept
        : IncomingConnectionManager(parent)
        , api_(api)
        , lock_()
        , peers_()
        , external_index_()
        , internal_index_()
        , cb_ex_(zmq::ListenCallback::Factory(
              [=](auto& in) { this->external(in); }))
        , cb_int_(zmq::ListenCallback::Factory(
              [=](auto& in) { this->internal(in); }))
        , external_(api_.ZeroMQ().RouterSocket(
              cb_ex_,
              zmq::socket::Socket::Direction::Bind))
        , internal_(api_.ZeroMQ().RouterSocket(
              cb_int_,
              zmq::socket::Socket::Direction::Bind))
    {
    }

private:
    using ConnectionID = OTData;
    using CachedMessages = std::vector<OTZMQMessage>;
    using PeerData = std::tuple<ConnectionID, ConnectionID, CachedMessages>;
    using Peers = std::map<int, PeerData>;
    using PeerIndex = std::map<ConnectionID, int>;

    const api::Core& api_;
    mutable std::mutex lock_;
    mutable Peers peers_;
    mutable PeerIndex external_index_;
    mutable PeerIndex internal_index_;
    OTZMQListenCallback cb_ex_;
    OTZMQListenCallback cb_int_;
    OTZMQRouterSocket external_;
    OTZMQRouterSocket internal_;

    auto make_endpoint() const noexcept -> std::string
    {
        return std::string{"inproc://"} +
               api_.Crypto().Encode().Nonce(20)->Get();
    }

    auto external(zmq::Message& message) noexcept -> void
    {
        const auto& header = message.Header();

        if (0 == header.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid header").Flush();

            return;
        }

        const auto& idFrame = header.at(0);
        const auto id = api_.Factory().Data(idFrame);
        Lock lock(lock_);
        auto index = external_index_.find(id);

        if (external_index_.end() != index) {
            auto& [externalID, internalID, cached] = peers_.at(index->second);

            if (internalID->empty()) {
                cached.emplace_back(message);
            } else {
                forward_message(internalID, message);
            }
        } else {
            const auto inproc = make_endpoint();
            const auto port =
                params::Data::chains_.at(parent_.chain_).default_port_;
            const auto zmq = inproc + ':' + std::to_string(port);
            auto address = factory::BlockchainAddress(
                api_,
                p2p::Protocol::bitcoin,
                p2p::Network::zmq,
                api_.Factory().Data(inproc, StringStyle::Raw),
                port,
                parent_.chain_,
                {},
                {},
                true);

            if (false == internal_->Start(zmq)) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to listen to internal endpoint")
                    .Flush();

                return;
            }

            const auto peerID = parent_.ConstructPeer(std::move(address));

            if (-1 == peerID) {
                LogOutput(OT_METHOD)(__FUNCTION__)(
                    ": Failed to instantiate peer")
                    .Flush();

                return;
            }

            external_index_.try_emplace(id, peerID);
            peers_.try_emplace(
                peerID, id, api_.Factory().Data(), CachedMessages{message});
        }
    }
    auto forward_message(const Data& internalID, zmq::Message& message) noexcept
        -> void
    {
        using Task = client::internal::PeerManager::Task;

        auto forwarded = api_.ZeroMQ().Message(internalID);
        forwarded->StartBody();
        forwarded->AddFrame(Task::Body);

        for (const auto& frame : message.Body()) { forwarded->AddFrame(frame); }

        internal_->Send(forwarded);
    }
    auto internal(zmq::Message& message) noexcept -> void
    {
        using Task = client::internal::PeerManager::Task;

        const auto header = message.Header();

        OT_ASSERT(0 < header.size());

        auto incomingID = api_.Factory().Data(header.at(0));
        const auto body = message.Body();

        OT_ASSERT(0 < body.size());

        switch (body.at(0).as<OTZMQWorkType>()) {
            case OT_ZMQ_REGISTER_SIGNAL: {
                OT_ASSERT(1 < body.size());

                const auto peerID = body.at(1).as<int>();

                Lock lock(lock_);

                try {
                    auto& [externalID, internalID, cached] = peers_.at(peerID);
                    internalID = incomingID;
                    auto reply =
                        api_.ZeroMQ().TaggedReply(message, Task::Register);
                    internal_->Send(reply);

                    for (auto& message : cached) {
                        forward_message(internalID, message);
                    }

                    cached.clear();
                    internal_index_.try_emplace(std::move(incomingID), peerID);
                } catch (...) {

                    return;
                }
            } break;
            case OT_ZMQ_SEND_SIGNAL: {
                OT_ASSERT(1 < body.size());

                Lock lock(lock_);

                try {
                    auto& [externalID, internalID, cached] =
                        peers_.at(internal_index_.at(incomingID));

                    auto outgoing = api_.ZeroMQ().Message(externalID);
                    outgoing->StartBody();

                    for (auto i = std::size_t{1}; i < body.size(); ++i) {
                        outgoing->AddFrame(body.at(i));
                    }

                    external_->Send(outgoing);
                } catch (...) {

                    return;
                }
            } break;
            default: {
                OT_FAIL;
            }
        }
    }

    ZMQIncomingConnectionManager() = delete;
    ZMQIncomingConnectionManager(const ZMQIncomingConnectionManager&) = delete;
    ZMQIncomingConnectionManager(ZMQIncomingConnectionManager&&) = delete;
    auto operator=(const ZMQIncomingConnectionManager&)
        -> ZMQIncomingConnectionManager& = delete;
    auto operator=(ZMQIncomingConnectionManager &&)
        -> ZMQIncomingConnectionManager& = delete;
};

auto PeerManager::IncomingConnectionManager::ZMQ(
    const api::Core& api,
    PeerManager::Peers& parent) noexcept
    -> std::unique_ptr<IncomingConnectionManager>
{
    return std::make_unique<ZMQIncomingConnectionManager>(api, parent);
}
}  // namespace opentxs::blockchain::client::implementation
