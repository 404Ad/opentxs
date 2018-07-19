// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP
#define OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/Proto.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <tuple>

namespace opentxs
{
class ServerContract : public Signable
{
public:
    /** A server listen address */
    typedef std::tuple<
        proto::AddressType,
        proto::ProtocolVersion,
        std::string,    // hostname / address
        std::uint32_t,  // port
        std::uint32_t>  // version
        Endpoint;

    static ServerContract* Create(
        const ConstNym& nym,
        const std::list<Endpoint>& endpoints,
        const std::string& terms,
        const std::string& name);
    static ServerContract* Factory(
        const ConstNym& nym,
        const proto::ServerContract& serialized);

    bool ConnectInfo(
        std::string& strHostname,
        std::uint32_t& nPort,
        const proto::AddressType& preferred = proto::ADDRESSTYPE_IPV4) const;
    proto::ServerContract Contract() const;
    std::string EffectiveName() const;
    std::string Name() const override { return name_; }
    proto::ServerContract PublicContract() const;
    bool Statistics(String& strContents) const;
    const unsigned char* PublicTransportKey() const;
    OTData Serialize() const override;
    const Data& TransportKey() const;
    std::unique_ptr<OTPassword> TransportKey(Data& pubkey) const;

    void SetAlias(const std::string& alias) override;

    ~ServerContract() = default;

private:
    typedef Signable ot_super;

    std::list<Endpoint> listen_params_{};
    std::string name_{""};
    OTData transport_key_;

    proto::ServerContract contract(const Lock& lock) const;
    OTIdentifier GetID(const Lock& lock) const override;
    proto::ServerContract IDVersion(const Lock& lock) const;
    proto::ServerContract SigVersion(const Lock& lock) const;
    bool validate(const Lock& lock) const override;
    bool verify_signature(const Lock& lock, const proto::Signature& signature)
        const override;

    bool update_signature(const Lock& lock) override;

    ServerContract() = delete;
    explicit ServerContract(const ConstNym& nym);
    ServerContract(
        const ConstNym& nym,
        const proto::ServerContract& serialized);
};
}  // namespace opentxs
#endif  // OPENTXS_CORE_CONTRACT_SERVERCONTRACT_HPP
