// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                       // IWYU pragma: associated
#include "1_Internal.hpp"                     // IWYU pragma: associated
#include "blockchain/client/PeerManager.hpp"  // IWYU pragma: associated

#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include "internal/blockchain/Params.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "internal/blockchain/p2p/P2P.hpp"
#include "internal/blockchain/p2p/bitcoin/Factory.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/api/Core.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/blockchain/p2p/Address.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"

#define OT_METHOD                                                              \
    "opentxs::blockchain::client::implementation::PeerManager::Peers::"

namespace opentxs::blockchain::client::implementation
{
PeerManager::Peers::Peers(
    const api::Core& api,
    const internal::Network& network,
    const internal::PeerDatabase& database,
    const internal::PeerManager& parent,
    const Flag& running,
    const std::string& shutdown,
    const Type chain,
    const std::string& seednode,
    const blockchain::client::internal::IO& context,
    const std::size_t peerTarget) noexcept
    : api_(api)
    , network_(network)
    , database_(database)
    , parent_(parent)
    , context_(context)
    , running_(running)
    , shutdown_endpoint_(shutdown)
    , chain_(chain)
    , invalid_peer_(false)
    , localhost_peer_(api.Factory().Data("0x7f000001", StringStyle::Hex))
    , default_peer_(set_default_peer(
          seednode,
          localhost_peer_,
          const_cast<bool&>(invalid_peer_)))
    , preferred_services_(get_preferred_services(database_))
    , resolver_(context_.operator boost::asio::io_context&())
    , next_id_(0)
    , minimum_peers_(peerTarget)
    , peers_()
    , active_()
    , count_()
    , connected_()
{
    const auto& data = params::Data::chains_.at(chain_);
    database_.AddOrUpdate(Endpoint{factory::BlockchainAddress(
        api_,
        data.p2p_protocol_,
        p2p::Network::ipv4,
        default_peer_,
        data.default_port_,
        chain_,
        Time{},
        {})});
}

auto PeerManager::Peers::add_peer(Endpoint endpoint) noexcept -> void
{
    OT_ASSERT(endpoint);

    const auto address = OTIdentifier{endpoint->ID()};
    auto& count = active_[address];

    if (0 == count) {
        auto addressID = OTIdentifier{endpoint->ID()};
        const auto id = ++next_id_;
        const auto [it, added] =
            peers_.emplace(id, peer_factory(std::move(endpoint), id));

        if (added) {
            ++count;
            ++count_;
            connected_.emplace(std::move(addressID));
        }
    }
}

auto PeerManager::Peers::AddPeer(
    const p2p::Address& address,
    std::promise<bool>& promise) noexcept -> void
{
    if (false == running_) {
        promise.set_value(false);

        return;
    }

    if (address.Chain() != chain_) {
        promise.set_value(false);

        return;
    }

    auto endpoint = Endpoint{factory::BlockchainAddress(
        api_,
        address.Style(),
        address.Type(),
        address.Bytes(),
        address.Port(),
        address.Chain(),
        address.LastConnected(),
        address.Services())};

    OT_ASSERT(endpoint);

    add_peer(std::move(endpoint));
    promise.set_value(true);
}

auto PeerManager::Peers::Disconnect(const int id) noexcept -> void
{
    auto it = peers_.find(id);

    if (peers_.end() == it) { return; }

    auto& peer = *it->second;
    const auto address = peer.AddressID();
    --active_.at(address);
    peer.Shutdown().get();
    it->second.reset();
    peers_.erase(it);
    --count_;
    connected_.erase(address);
}

auto PeerManager::Peers::get_default_peer() const noexcept -> Endpoint
{
    if (localhost_peer_.get() == default_peer_) { return {}; }

    const auto& data = params::Data::chains_.at(chain_);

    return Endpoint{factory::BlockchainAddress(
        api_,
        data.p2p_protocol_,
        p2p::Network::ipv4,
        default_peer_,
        data.default_port_,
        chain_,
        Time{},
        {})};
}

auto PeerManager::Peers::get_dns_peer() const noexcept -> Endpoint
{
    try {
        const auto& data = params::Data::chains_.at(chain_);
        const auto& dns = data.dns_seeds_;
        auto seeds = std::vector<std::string>{};
        const auto count = std::size_t{1};
        std::sample(
            std::begin(dns),
            std::end(dns),
            std::back_inserter(seeds),
            count,
            std::mt19937{std::random_device{}()});

        if (0 == seeds.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": No dns seeds available")
                .Flush();

            return {};
        }

        const auto& seed = *seeds.cbegin();
        const auto port = data.default_port_;
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Using DNS seed: ")(seed).Flush();
        const auto results = resolver_.resolve(
            seed, std::to_string(port), Resolver::query::numeric_service);

        for (const auto& result : results) {
            const auto address = result.endpoint().address();
            LogVerbose(OT_METHOD)(__FUNCTION__)(": Found address: ")(
                address.to_string())
                .Flush();
            auto output = Endpoint{};

            if (address.is_v4()) {
                const auto bytes = address.to_v4().to_bytes();
                output = factory::BlockchainAddress(
                    api_,
                    data.p2p_protocol_,
                    p2p::Network::ipv4,
                    api_.Factory().Data(ReadView{
                        reinterpret_cast<const char*>(bytes.data()),
                        bytes.size()}),
                    port,
                    chain_,
                    Time{},
                    {});
            } else if (address.is_v6()) {
                const auto bytes = address.to_v6().to_bytes();
                output = factory::BlockchainAddress(
                    api_,
                    data.p2p_protocol_,
                    p2p::Network::ipv6,
                    api_.Factory().Data(ReadView{
                        reinterpret_cast<const char*>(bytes.data()),
                        bytes.size()}),
                    port,
                    chain_,
                    Time{},
                    {});
            }

            if (output) {
                database_.AddOrUpdate(output->clone_internal());

                return output;
            }
        }

        LogVerbose(OT_METHOD)(__FUNCTION__)(": No addresses found").Flush();

        return {};
    } catch (const boost::system::system_error& e) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": ")(e.what()).Flush();

        return {};
    } catch (...) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": No dns seeds defined").Flush();

        return {};
    }
}

auto PeerManager::Peers::get_fallback_peer(
    const p2p::Protocol protocol) const noexcept -> Endpoint
{
    return database_.Get(
        protocol, {p2p::Network::ipv4, p2p::Network::ipv6}, {});
}

auto PeerManager::Peers::get_peer() const noexcept -> Endpoint
{
    const auto protocol = params::Data::chains_.at(chain_).p2p_protocol_;
    auto pAddress = get_default_peer();

    if (pAddress && is_not_connected(*pAddress)) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": Attempting to connect to peer: ")(pAddress->Display())
            .Flush();

        return pAddress;
    }

    pAddress = get_preferred_peer(protocol);

    if (pAddress && is_not_connected(*pAddress)) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": Attempting to connect to peer: ")(pAddress->Display())
            .Flush();

        return pAddress;
    }

    pAddress = get_dns_peer();

    if (pAddress && is_not_connected(*pAddress)) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": Attempting to connect to peer: ")(pAddress->Display())
            .Flush();

        return pAddress;
    }

    pAddress = get_fallback_peer(protocol);

    OT_ASSERT(pAddress);

    LogVerbose(OT_METHOD)(__FUNCTION__)(": Attempting to connect to peer: ")(
        pAddress->Display())
        .Flush();

    return pAddress;
}

auto PeerManager::Peers::get_preferred_peer(
    const p2p::Protocol protocol) const noexcept -> Endpoint
{
    return database_.Get(
        protocol,
        {p2p::Network::ipv4, p2p::Network::ipv6},
        preferred_services_);
}

auto PeerManager::Peers::get_preferred_services(
    const internal::PeerDatabase& db) noexcept -> std::set<p2p::Service>
{
    if (api::client::blockchain::BlockStorage::All == db.BlockPolicy()) {

        return {};
    } else {

        return {p2p::Service::CompactFilters};
    }
}

auto PeerManager::Peers::is_not_connected(
    const p2p::Address& endpoint) const noexcept -> bool
{
    return 0 == connected_.count(endpoint.ID());
}

auto PeerManager::Peers::peer_factory(Endpoint endpoint, const int id) noexcept
    -> p2p::internal::Peer*
{
    switch (chain_) {
        case Type::Bitcoin:
        case Type::Bitcoin_testnet3:
        case Type::BitcoinCash:
        case Type::BitcoinCash_testnet3:
        case Type::Litecoin:
        case Type::Litecoin_testnet4: {
            return factory::BitcoinP2PPeerLegacy(
                api_,
                network_,
                parent_,
                context_,
                id,
                std::move(endpoint),
                shutdown_endpoint_);
        }
        case Type::Unknown:
        case Type::Ethereum_frontier:
        case Type::Ethereum_ropsten:
        case Type::UnitTest:
        default: {
            OT_FAIL;
        }
    }
}

auto PeerManager::Peers::set_default_peer(
    const std::string node,
    const Data& localhost,
    bool& invalidPeer) noexcept -> OTData
{
    if (false == node.empty()) {
        try {
            const auto bytes = ip::make_address_v4(node).to_bytes();

            return Data::Factory(bytes.data(), bytes.size());
        } catch (...) {
            invalidPeer = true;
        }
    }

    return localhost;
}

auto PeerManager::Peers::Run() noexcept -> bool
{
    if ((false == running_) || invalid_peer_) { return false; }

    const auto target = minimum_peers_.load();

    if (target > peers_.size()) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(": Fewer peers (")(peers_.size())(
            ") than desired (")(target)(")")
            .Flush();
        add_peer(get_peer());
    }

    return target > peers_.size();
}

auto PeerManager::Peers::Shutdown() noexcept -> void
{
    OT_ASSERT(false == running_);

    for (auto& [id, peer] : peers_) {
        peer->Shutdown().wait_for(std::chrono::seconds(1));
        peer.reset();
    }

    peers_.clear();
    count_.store(0);
    active_.clear();
}
}  // namespace opentxs::blockchain::client::implementation
