// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_PEERREPLY_HPP
#define OPENTXS_CORE_CONTRACT_PEER_PEERREPLY_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/contract/Signable.hpp"
#include "opentxs/Proto.hpp"

#include <string>

namespace opentxs
{
class PeerReply : public Signable
{
private:
    typedef Signable ot_super;

    OTIdentifier initiator_;
    OTIdentifier recipient_;
    OTIdentifier server_;
    OTIdentifier cookie_;
    proto::PeerRequestType type_{proto::PEERREQUEST_ERROR};

    static OTIdentifier GetID(const proto::PeerReply& contract);
    static bool FinalizeContract(PeerReply& contract);
    static std::unique_ptr<PeerReply> Finish(
        std::unique_ptr<PeerReply>& contract);
    static std::shared_ptr<proto::PeerRequest> LoadRequest(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const Identifier& requestID);

    proto::PeerReply contract(const Lock& lock) const;
    OTIdentifier GetID(const Lock& lock) const override;
    proto::PeerReply SigVersion(const Lock& lock) const;

    bool update_signature(const Lock& lock) override;

    PeerReply() = delete;

protected:
    const api::Wallet& wallet_;

    virtual proto::PeerReply IDVersion(const Lock& lock) const;
    bool validate(const Lock& lock) const override;
    bool verify_signature(const Lock& lock, const proto::Signature& signature)
        const override;

    PeerReply(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const proto::PeerReply& serialized);
    PeerReply(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const std::uint32_t version,
        const Identifier& initiator,
        const Identifier& server,
        const proto::PeerRequestType& type,
        const Identifier& request);

public:
    static std::unique_ptr<PeerReply> Create(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const proto::PeerRequestType& type,
        const Identifier& request,
        const Identifier& server,
        const std::string& terms);
    static std::unique_ptr<PeerReply> Create(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const Identifier& request,
        const Identifier& server,
        const bool& ack);
    static std::unique_ptr<PeerReply> Create(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const Identifier& request,
        const Identifier& server,
        const bool& ack,
        const std::string& url,
        const std::string& login,
        const std::string& password,
        const std::string& key);
    static std::unique_ptr<PeerReply> Factory(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const proto::PeerReply& serialized);

    std::string Alias() const override { return Name(); }
    proto::PeerReply Contract() const;
    std::string Name() const override;
    OTData Serialize() const override;
    const proto::PeerRequestType& Type() const { return type_; }
    void SetAlias(const std::string&) override {}

    ~PeerReply() = default;
};
}  // namespace opentxs

#endif  // OPENTXS_CORE_CONTRACT_PEER_PEERREPLY_HPP
