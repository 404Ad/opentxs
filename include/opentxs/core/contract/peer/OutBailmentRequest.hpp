// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_OUTBAILMENTREQUEST_HPP
#define OPENTXS_CORE_CONTRACT_PEER_OUTBAILMENTREQUEST_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/contract/peer/PeerRequest.hpp"

namespace opentxs
{
namespace contract
{
namespace peer
{
namespace request
{
class Outbailment : virtual public peer::Request
{
public:
    OPENTXS_EXPORT ~Outbailment() override = default;

protected:
    Outbailment() noexcept = default;

private:
    friend OTOutbailmentRequest;

#ifndef _WIN32
    OPENTXS_EXPORT Outbailment* clone() const noexcept override = 0;
#endif

    Outbailment(const Outbailment&) = delete;
    Outbailment(Outbailment&&) = delete;
    Outbailment& operator=(const Outbailment&) = delete;
    Outbailment& operator=(Outbailment&&) = delete;
};
}  // namespace request
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif
