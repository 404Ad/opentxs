// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "opentxs/core/contract/peer/BailmentReply.hpp"

#include "opentxs/core/Identifier.hpp"

#define CURRENT_VERSION 4

namespace opentxs
{
BailmentReply::BailmentReply(
    const ConstNym& nym,
    const proto::PeerReply& serialized)
    : ot_super(nym, serialized)
{
    conditions_ = serialized.bailment().instructions();
}

BailmentReply::BailmentReply(
    const ConstNym& nym,
    const Identifier& initiator,
    const Identifier& request,
    const Identifier& server,
    const std::string& terms)
    : ot_super(
          nym,
          CURRENT_VERSION,
          initiator,
          server,
          proto::PEERREQUEST_BAILMENT,
          request)
{
    conditions_ = terms;
}

proto::PeerReply BailmentReply::IDVersion(const Lock& lock) const
{
    auto contract = ot_super::IDVersion(lock);
    auto& bailment = *contract.mutable_bailment();
    bailment.set_version(version_);
    bailment.set_instructions(conditions_);

    return contract;
}
}  // namespace opentxs
