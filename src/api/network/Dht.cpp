// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"         // IWYU pragma: associated
#include "1_Internal.hpp"       // IWYU pragma: associated
#include "api/network/Dht.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "network/DhtConfig.hpp"
#if OT_DHT
#include "network/OpenDHT.hpp"
#endif  // OT_DHT
#include "opentxs/Forward.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Proto.tpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/api/network/Dht.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/ReplyCallback.hpp"
#include "opentxs/network/zeromq/socket/Reply.hpp"
#include "opentxs/network/zeromq/socket/Socket.hpp"

#if OT_DHT
#define OT_METHOD "opentxs::Dht"
#endif

namespace zmq = opentxs::network::zeromq;

namespace opentxs
{
auto Factory::Dht(
    const bool defaultEnable,
    const api::internal::Core& api,
    std::int64_t& nymPublishInterval,
    std::int64_t& nymRefreshInterval,
    std::int64_t& serverPublishInterval,
    std::int64_t& serverRefreshInterval,
    std::int64_t& unitPublishInterval,
    std::int64_t& unitRefreshInterval) -> api::network::Dht*
{
    DhtConfig config;
    bool notUsed;
    api.Config().CheckSet_bool(
        String::Factory("OpenDHT"),
        String::Factory("enable_dht"),
        defaultEnable,
        config.enable_dht_,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("nym_publish_interval"),
        config.nym_publish_interval_,
        nymPublishInterval,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("nym_refresh_interval"),
        config.nym_refresh_interval_,
        nymRefreshInterval,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("server_publish_interval"),
        config.server_publish_interval_,
        serverPublishInterval,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("server_refresh_interval"),
        config.server_refresh_interval_,
        serverRefreshInterval,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("unit_publish_interval"),
        config.unit_publish_interval_,
        unitPublishInterval,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("unit_refresh_interval"),
        config.unit_refresh_interval_,
        unitRefreshInterval,
        notUsed);
    api.Config().CheckSet_long(
        String::Factory("OpenDHT"),
        String::Factory("listen_port"),
        config.default_port_,
        config.listen_port_,
        notUsed);
    api.Config().CheckSet_str(
        String::Factory("OpenDHT"),
        String::Factory("bootstrap_url"),
        String::Factory(config.bootstrap_url_),
        config.bootstrap_url_,
        notUsed);
    api.Config().CheckSet_str(
        String::Factory("OpenDHT"),
        String::Factory("bootstrap_port"),
        String::Factory(config.bootstrap_port_),
        config.bootstrap_port_,
        notUsed);

    return new api::network::implementation::Dht(config, api);
}
}  // namespace opentxs

namespace opentxs::api::network::implementation
{
Dht::Dht(DhtConfig& config, const api::internal::Core& api)
    : api_(api)
    , config_(new DhtConfig(config))
#if OT_DHT
    , node_(new opentxs::network::implementation::OpenDHT(*config_))
#endif
    , request_nym_callback_{zmq::ReplyCallback::Factory(
          [=](const zmq::Message& incoming) -> OTZMQMessage {
              return this->process_request(incoming, &Dht::GetPublicNym);
          })}
    , request_nym_socket_{api_.ZeroMQ().ReplySocket(
          request_nym_callback_,
          zmq::socket::Socket::Direction::Bind)}
    , request_server_callback_{zmq::ReplyCallback::Factory(
          [=](const zmq::Message& incoming) -> OTZMQMessage {
              return this->process_request(incoming, &Dht::GetServerContract);
          })}
    , request_server_socket_{api_.ZeroMQ().ReplySocket(
          request_server_callback_,
          zmq::socket::Socket::Direction::Bind)}
    , request_unit_callback_{zmq::ReplyCallback::Factory(
          [=](const zmq::Message& incoming) -> OTZMQMessage {
              return this->process_request(incoming, &Dht::GetUnitDefinition);
          })}
    , request_unit_socket_{api_.ZeroMQ().ReplySocket(
          request_unit_callback_,
          zmq::socket::Socket::Direction::Bind)}
{
    request_nym_socket_->Start(api_.Endpoints().DhtRequestNym());
    request_server_socket_->Start(api_.Endpoints().DhtRequestServer());
    request_unit_socket_->Start(api_.Endpoints().DhtRequestUnit());
}

void Dht::Insert(
    [[maybe_unused]] const std::string& key,
    [[maybe_unused]] const std::string& value) const
{
#if OT_DHT
    node_->Insert(key, value);
#endif
}

void Dht::Insert([[maybe_unused]] const identity::Nym::Serialized& nym) const
{
#if OT_DHT
    node_->Insert(nym.nymid(), proto::ToString(nym));
#endif
}

void Dht::Insert([[maybe_unused]] const proto::ServerContract& contract) const
{
#if OT_DHT
    node_->Insert(contract.id(), proto::ToString(contract));
#endif
}

void Dht::Insert([[maybe_unused]] const proto::UnitDefinition& contract) const
{
#if OT_DHT
    node_->Insert(contract.id(), proto::ToString(contract));
#endif
}

void Dht::GetPublicNym([[maybe_unused]] const std::string& key) const
{
#if OT_DHT
    auto it = callback_map_.find(Dht::Callback::PUBLIC_NYM);
    bool haveCB = (it != callback_map_.end());
    NotifyCB notifyCB;

    if (haveCB) { notifyCB = it->second; }

    DhtResultsCallback gcb(
        [this, notifyCB, key](const DhtResults& values) -> bool {
            return ProcessPublicNym(api_, key, values, notifyCB);
        });

    node_->Retrieve(key, gcb);
#endif
}

void Dht::GetServerContract([[maybe_unused]] const std::string& key) const
{
#if OT_DHT
    auto it = callback_map_.find(Dht::Callback::SERVER_CONTRACT);
    bool haveCB = (it != callback_map_.end());
    NotifyCB notifyCB;

    if (haveCB) { notifyCB = it->second; }

    DhtResultsCallback gcb(
        [this, notifyCB, key](const DhtResults& values) -> bool {
            return ProcessServerContract(api_, key, values, notifyCB);
        });

    node_->Retrieve(key, gcb);
#endif
}

void Dht::GetUnitDefinition([[maybe_unused]] const std::string& key) const
{
#if OT_DHT
    auto it = callback_map_.find(Dht::Callback::ASSET_CONTRACT);
    bool haveCB = (it != callback_map_.end());
    NotifyCB notifyCB;

    if (haveCB) { notifyCB = it->second; }

    DhtResultsCallback gcb(
        [this, notifyCB, key](const DhtResults& values) -> bool {
            return ProcessUnitDefinition(api_, key, values, notifyCB);
        });

    node_->Retrieve(key, gcb);
#endif
}

#if OT_DHT
auto Dht::OpenDHT() const -> const opentxs::network::OpenDHT& { return *node_; }
#endif

auto Dht::process_request(
    const zmq::Message& incoming,
    void (Dht::*get)(const std::string&) const) const -> OTZMQMessage
{
    OT_ASSERT(nullptr != get)

    bool output{false};

    if (1 == incoming.size()) {
        const std::string id{incoming.at(0)};
        const auto nymID = identifier::Nym::Factory(id);

        if (false == nymID->empty()) {
            output = true;
            (this->*get)(id);
        }
    }

    return api_.ZeroMQ().Message(output);
}

#if OT_DHT
auto Dht::ProcessPublicNym(
    const api::internal::Core& api,
    const std::string key,
    const DhtResults& values,
    NotifyCB notifyCB) -> bool
{
    std::string theresult;
    bool foundData = false;
    bool foundValid = false;

    if (key.empty()) { return false; }

    for (const auto& it : values) {
        if (nullptr == it) { continue; }

        auto& data = *it;
        foundData = data.size() > 0;

        if (0 == data.size()) { continue; }

        auto publicNym = proto::Factory<proto::Nym>(data);

        if (key != publicNym.nymid()) { continue; }

        auto existing = api.Wallet().Nym(api.Factory().NymID(key));

        if (existing) {
            if (existing->Revision() >= publicNym.revision()) { continue; }
        }

        auto saved = api.Wallet().Nym(publicNym);

        if (!saved) { continue; }

        foundValid = true;

        LogDebug(OT_METHOD)(__FUNCTION__)(": Saved nym: ")(key).Flush();

        if (notifyCB) { notifyCB(key); }
    }

    if (!foundValid) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Found results, but none are valid.")
            .Flush();
    }

    if (!foundData) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": All results are empty.").Flush();
    }

    return foundData;
}

auto Dht::ProcessServerContract(
    const api::internal::Core& api,
    const std::string key,
    const DhtResults& values,
    NotifyCB notifyCB) -> bool
{
    std::string theresult;
    bool foundData = false;
    bool foundValid = false;

    if (key.empty()) { return false; }

    for (const auto& it : values) {
        if (nullptr == it) { continue; }

        auto& data = *it;
        foundData = data.size() > 0;

        if (0 == data.size()) { continue; }

        auto contract = proto::Factory<proto::ServerContract>(data);

        if (key != contract.id()) { continue; }

        try {
            auto saved = api.Wallet().Server(contract);
        } catch (...) {
            continue;
        }

        LogDebug(OT_METHOD)(__FUNCTION__)(": Saved contract: ")(key).Flush();
        foundValid = true;

        if (notifyCB) { notifyCB(key); }

        break;  // We only need the first valid result
    }

    if (!foundValid) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Found results, but none are valid.")
            .Flush();
    }

    if (!foundData) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": All results are empty.").Flush();
    }

    return foundData;
}

auto Dht::ProcessUnitDefinition(
    const api::internal::Core& api,
    const std::string key,
    const DhtResults& values,
    NotifyCB notifyCB) -> bool
{
    std::string theresult;
    bool foundData = false;
    bool foundValid = false;

    if (key.empty()) { return false; }

    for (const auto& it : values) {
        if (nullptr == it) { continue; }

        auto& data = *it;
        foundData = data.size() > 0;

        if (0 == data.size()) { continue; }

        auto contract = proto::Factory<proto::UnitDefinition>(data);

        if (key != contract.id()) { continue; }

        try {
            api.Wallet().UnitDefinition(contract);
        } catch (...) {

            continue;
        }

        LogDebug(OT_METHOD)(__FUNCTION__)(": Saved unit definition: ")(key)
            .Flush();
        foundValid = true;

        if (notifyCB) { notifyCB(key); }

        break;  // We only need the first valid result
    }

    if (!foundValid) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Found results, but none are valid.")
            .Flush();
    }

    if (!foundData) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": All results are empty.").Flush();
    }

    return foundData;
}
#endif

void Dht::RegisterCallbacks(const CallbackMap& callbacks) const
{
    callback_map_ = callbacks;
}
}  // namespace opentxs::api::network::implementation
