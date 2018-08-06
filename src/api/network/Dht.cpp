// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Internal.hpp"

#include "opentxs/api/network/Dht.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/ReplyCallback.hpp"
#include "opentxs/network/zeromq/ReplySocket.hpp"
#include "opentxs/network/zeromq/Socket.hpp"
#include "opentxs/Types.hpp"

#include "network/DhtConfig.hpp"
#if OT_DHT
#include "network/OpenDHT.hpp"
#endif

#include <cstdint>
#include <string>

#include "Dht.hpp"

namespace opentxs
{
api::network::Dht* Factory::Dht(
    const int instance,
    const bool defaultEnable,
    const api::Settings& settings,
    const api::Wallet& wallet,
    const network::zeromq::Context& zmq,
    std::int64_t& nymPublishInterval,
    std::int64_t& nymRefreshInterval,
    std::int64_t& serverPublishInterval,
    std::int64_t& serverRefreshInterval,
    std::int64_t& unitPublishInterval,
    std::int64_t& unitRefreshInterval)
{
    DhtConfig config;
    bool notUsed;
    settings.CheckSet_bool(
        "OpenDHT", "enable_dht", defaultEnable, config.enable_dht_, notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "nym_publish_interval",
        config.nym_publish_interval_,
        nymPublishInterval,
        notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "nym_refresh_interval",
        config.nym_refresh_interval_,
        nymRefreshInterval,
        notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "server_publish_interval",
        config.server_publish_interval_,
        serverPublishInterval,
        notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "server_refresh_interval",
        config.server_refresh_interval_,
        serverRefreshInterval,
        notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "unit_publish_interval",
        config.unit_publish_interval_,
        unitPublishInterval,
        notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "unit_refresh_interval",
        config.unit_refresh_interval_,
        unitRefreshInterval,
        notUsed);
    settings.CheckSet_long(
        "OpenDHT",
        "listen_port",
        config.default_port_,
        config.listen_port_,
        notUsed);
    settings.CheckSet_str(
        "OpenDHT",
        "bootstrap_url",
        String(config.bootstrap_url_),
        config.bootstrap_url_,
        notUsed);
    settings.CheckSet_str(
        "OpenDHT",
        "bootstrap_port",
        String(config.bootstrap_port_),
        config.bootstrap_port_,
        notUsed);

    return new api::network::implementation::Dht(instance, config, wallet, zmq);
}
}  // namespace opentxs

namespace opentxs::api::network::implementation
{
Dht::Dht(
    const int instance,
    DhtConfig& config,
    const api::Wallet& wallet,
    const opentxs::network::zeromq::Context& zmq)
    : instance_{instance}
    , wallet_(wallet)
    , config_(new DhtConfig(config))
#if OT_DHT
    , node_(new opentxs::network::implementation::OpenDHT(*config_))
#endif
    , request_nym_callback_{opentxs::network::zeromq::ReplyCallback::Factory(
          [=](const opentxs::network::zeromq::Message& incoming)
              -> OTZMQMessage {
              return this->process_request(incoming, &Dht::GetPublicNym);
          })}
    , request_nym_socket_{zmq.ReplySocket(request_nym_callback_, false)}
    , request_server_callback_{opentxs::network::zeromq::ReplyCallback::Factory(
          [=](const opentxs::network::zeromq::Message& incoming)
              -> OTZMQMessage {
              return this->process_request(incoming, &Dht::GetServerContract);
          })}
    , request_server_socket_{zmq.ReplySocket(request_server_callback_, false)}
    , request_unit_callback_{opentxs::network::zeromq::ReplyCallback::Factory(
          [=](const opentxs::network::zeromq::Message& incoming)
              -> OTZMQMessage {
              return this->process_request(incoming, &Dht::GetUnitDefinition);
          })}
    , request_unit_socket_{zmq.ReplySocket(request_unit_callback_, false)}
{
    request_nym_socket_->Start(
        opentxs::network::zeromq::Socket::GetDhtRequestNymEndpoint(instance_));
    request_server_socket_->Start(
        opentxs::network::zeromq::Socket::GetDhtRequestServerEndpoint(
            instance_));
    request_unit_socket_->Start(
        opentxs::network::zeromq::Socket::GetDhtRequestUnitEndpoint(instance_));
}

void Dht::Insert(
    __attribute__((unused)) const std::string& key,
    __attribute__((unused)) const std::string& value) const
{
#if OT_DHT
    node_->Insert(key, value);
#endif
}

void Dht::Insert(__attribute__((unused))
                 const serializedCredentialIndex& nym) const
{
#if OT_DHT
    node_->Insert(nym.nymid(), proto::ProtoAsString(nym));
#endif
}

void Dht::Insert(__attribute__((unused))
                 const proto::ServerContract& contract) const
{
#if OT_DHT
    node_->Insert(contract.id(), proto::ProtoAsString(contract));
#endif
}

void Dht::Insert(__attribute__((unused))
                 const proto::UnitDefinition& contract) const
{
#if OT_DHT
    node_->Insert(contract.id(), proto::ProtoAsString(contract));
#endif
}

void Dht::GetPublicNym(__attribute__((unused)) const std::string& key) const
{
#if OT_DHT
    auto it = callback_map_.find(Dht::Callback::PUBLIC_NYM);
    bool haveCB = (it != callback_map_.end());
    NotifyCB notifyCB;

    if (haveCB) { notifyCB = it->second; }

    DhtResultsCallback gcb(
        [this, notifyCB, key](const DhtResults& values) -> bool {
            return ProcessPublicNym(wallet_, key, values, notifyCB);
        });

    node_->Retrieve(key, gcb);
#endif
}

void Dht::GetServerContract(__attribute__((unused))
                            const std::string& key) const
{
#if OT_DHT
    auto it = callback_map_.find(Dht::Callback::SERVER_CONTRACT);
    bool haveCB = (it != callback_map_.end());
    NotifyCB notifyCB;

    if (haveCB) { notifyCB = it->second; }

    DhtResultsCallback gcb(
        [this, notifyCB, key](const DhtResults& values) -> bool {
            return ProcessServerContract(wallet_, key, values, notifyCB);
        });

    node_->Retrieve(key, gcb);
#endif
}

void Dht::GetUnitDefinition(__attribute__((unused))
                            const std::string& key) const
{
#if OT_DHT
    auto it = callback_map_.find(Dht::Callback::ASSET_CONTRACT);
    bool haveCB = (it != callback_map_.end());
    NotifyCB notifyCB;

    if (haveCB) { notifyCB = it->second; }

    DhtResultsCallback gcb(
        [this, notifyCB, key](const DhtResults& values) -> bool {
            return ProcessUnitDefinition(wallet_, key, values, notifyCB);
        });

    node_->Retrieve(key, gcb);
#endif
}

#if OT_DHT
const opentxs::network::OpenDHT& Dht::OpenDHT() const { return *node_; }

OTZMQMessage Dht::process_request(
    const opentxs::network::zeromq::Message& incoming,
    void (Dht::*get)(const std::string&) const) const
{
    OT_ASSERT(nullptr != get)

    bool output{false};

    if (1 == incoming.size()) {
        const std::string id{incoming.at(0)};
        const auto nymID = Identifier::Factory(id);

        if (false == nymID->empty()) {
            output = true;
            (this->*get)(id);
        }
    }

    return opentxs::network::zeromq::Message::Factory(
        Data::Factory(&output, sizeof(output)));
}

bool Dht::ProcessPublicNym(
    const api::Wallet& wallet,
    const std::string key,
    const DhtResults& values,
    NotifyCB notifyCB)
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

        auto publicNym = proto::DataToProto<proto::CredentialIndex>(
            Data::Factory(data.c_str(), data.size()));

        if (key != publicNym.nymid()) { continue; }

        auto existing = wallet.Nym(Identifier::Factory(key));

        if (existing) {
            if (existing->Revision() >= publicNym.revision()) { continue; }
        }

        auto saved = wallet.Nym(publicNym);

        if (!saved) { continue; }

        foundValid = true;
        otLog3 << "Saved nym: " << key << std::endl;

        if (notifyCB) { notifyCB(key); }
    }

    if (!foundValid) {
        otErr << __FUNCTION__ << "Found results, but none are valid."
              << std::endl;
    }

    if (!foundData) {
        otErr << __FUNCTION__ << "All results are empty" << std::endl;
    }

    return foundData;
}

bool Dht::ProcessServerContract(
    const api::Wallet& wallet,
    const std::string key,
    const DhtResults& values,
    NotifyCB notifyCB)
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

        auto contract = proto::DataToProto<proto::ServerContract>(
            Data::Factory(data.c_str(), data.size()));

        if (key != contract.id()) { continue; }

        auto saved = wallet.Server(contract);

        if (!saved) { continue; }

        otLog3 << "Saved contract: " << key << std::endl;
        foundValid = true;

        if (notifyCB) { notifyCB(key); }

        break;  // We only need the first valid result
    }

    if (!foundValid) {
        otErr << __FUNCTION__ << "Found results, but none are valid."
              << std::endl;
    }

    if (!foundData) {
        otErr << __FUNCTION__ << "All results are empty" << std::endl;
    }

    return foundData;
}

bool Dht::ProcessUnitDefinition(
    const api::Wallet& wallet,
    const std::string key,
    const DhtResults& values,
    NotifyCB notifyCB)
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

        auto contract = proto::DataToProto<proto::UnitDefinition>(
            Data::Factory(data.c_str(), data.size()));

        if (key != contract.id()) { continue; }

        auto saved = wallet.UnitDefinition(contract);

        if (!saved) { continue; }

        otLog3 << "Saved unit definition: " << key << std::endl;
        foundValid = true;

        if (notifyCB) { notifyCB(key); }

        break;  // We only need the first valid result
    }

    if (!foundValid) {
        otErr << __FUNCTION__ << "Found results, but none are valid."
              << std::endl;
    }

    if (!foundData) {
        otErr << __FUNCTION__ << "All results are empty" << std::endl;
    }

    return foundData;
}
#endif

void Dht::RegisterCallbacks(const CallbackMap& callbacks) const
{
    callback_map_ = callbacks;
}

Dht::~Dht() {}
}  // namespace opentxs::api::network::implementation
