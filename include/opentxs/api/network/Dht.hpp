// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_NETWORK_DHT_HPP
#define OPENTXS_API_NETWORK_DHT_HPP

// IWYU pragma: no_include "opentxs/Proto.hpp"

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <functional>
#include <map>
#include <string>

#include "opentxs/Proto.hpp"
#include "opentxs/identity/Nym.hpp"

namespace opentxs
{
namespace api
{
namespace network
{
class Dht
{
public:
    enum class Callback : std::uint8_t {
        SERVER_CONTRACT = 0,
        ASSET_CONTRACT = 1,
        PUBLIC_NYM = 2
    };

    using NotifyCB = std::function<void(const std::string)>;
    using CallbackMap = std::map<Callback, NotifyCB>;

    OPENTXS_EXPORT virtual void GetPublicNym(const std::string& key) const = 0;
    OPENTXS_EXPORT virtual void GetServerContract(
        const std::string& key) const = 0;
    OPENTXS_EXPORT virtual void GetUnitDefinition(
        const std::string& key) const = 0;
    OPENTXS_EXPORT virtual void Insert(
        const std::string& key,
        const std::string& value) const = 0;
    OPENTXS_EXPORT virtual void Insert(
        const identity::Nym::Serialized& nym) const = 0;
    OPENTXS_EXPORT virtual void Insert(
        const proto::ServerContract& contract) const = 0;
    OPENTXS_EXPORT virtual void Insert(
        const proto::UnitDefinition& contract) const = 0;
#if OT_DHT
    OPENTXS_EXPORT virtual const opentxs::network::OpenDHT& OpenDHT() const = 0;
#endif
    OPENTXS_EXPORT virtual void RegisterCallbacks(
        const CallbackMap& callbacks) const = 0;

    OPENTXS_EXPORT virtual ~Dht() = default;

protected:
    Dht() = default;

private:
    Dht(const Dht&) = delete;
    Dht(Dht&&) = delete;
    Dht& operator=(const Dht&) = delete;
    Dht& operator=(Dht&&) = delete;
};
}  // namespace network
}  // namespace api
}  // namespace opentxs

#endif
