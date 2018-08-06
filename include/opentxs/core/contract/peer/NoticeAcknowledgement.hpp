// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_NOTICEACKNOWLEDGEMENT_HPP
#define OPENTXS_CORE_CONTRACT_PEER_NOTICEACKNOWLEDGEMENT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/contract/peer/PeerReply.hpp"

#include <string>

namespace opentxs
{

class NoticeAcknowledgement : public PeerReply
{
private:
    typedef PeerReply ot_super;
    friend class PeerReply;

    bool ack_{false};

    proto::PeerReply IDVersion(const Lock& lock) const override;

    NoticeAcknowledgement(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const proto::PeerReply& serialized);
    NoticeAcknowledgement(
        const api::Wallet& wallet,
        const ConstNym& nym,
        const Identifier& initiator,
        const Identifier& request,
        const Identifier& server,
        const proto::PeerRequestType type,
        const bool& ack);
    NoticeAcknowledgement() = delete;

public:
    ~NoticeAcknowledgement() = default;
};
}  // namespace opentxs

#endif  // OPENTXS_CORE_CONTRACT_PEER_NOTICEACKNOWLEDGEMENT_HPP
