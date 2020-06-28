// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CORE_CONTRACT_PEER_CONNECTIONREQUEST_HPP
#define OPENTXS_CORE_CONTRACT_PEER_CONNECTIONREQUEST_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include "opentxs/SharedPimpl.hpp"
#include "opentxs/core/contract/peer/PeerRequest.hpp"

namespace opentxs
{
namespace contract
{
namespace peer
{
namespace request
{
class Connection;
}  // namespace request
}  // namespace peer
}  // namespace contract

using OTConnectionRequest = SharedPimpl<contract::peer::request::Connection>;
}  // namespace opentxs

namespace opentxs
{
namespace contract
{
namespace peer
{
namespace request
{
class Connection : virtual public peer::Request
{
public:
    OPENTXS_EXPORT ~Connection() override = default;

protected:
    Connection() noexcept = default;

private:
    friend OTConnectionRequest;

#ifndef _WIN32
    OPENTXS_EXPORT Connection* clone() const noexcept override = 0;
#endif

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;
};
}  // namespace request
}  // namespace peer
}  // namespace contract
}  // namespace opentxs
#endif
