// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_BAILMENTNOTICE_HPP
#define OPENTXS_CORE_CONTRACT_PEER_BAILMENTNOTICE_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/contract/peer/PeerRequest.hpp"

namespace opentxs
{
using OTBailmentNotice = SharedPimpl<contract::peer::request::BailmentNotice>;

namespace contract
{
namespace peer
{
namespace request
{
class BailmentNotice : virtual public peer::Request
{
public:
    OPENTXS_EXPORT ~BailmentNotice() override = default;

protected:
    BailmentNotice() noexcept = default;

private:
    friend OTBailmentNotice;

#ifndef _WIN32
    OPENTXS_EXPORT BailmentNotice* clone() const noexcept override = 0;
#endif

    BailmentNotice(const BailmentNotice&) = delete;
    BailmentNotice(BailmentNotice&&) = delete;
    BailmentNotice& operator=(const BailmentNotice&) = delete;
    BailmentNotice& operator=(BailmentNotice&&) = delete;
};
}  // namespace request
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif
