// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_ZEROMQ_SOCKET_REQUEST_TPP
#define OPENTXS_NETWORK_ZEROMQ_SOCKET_REQUEST_TPP

#include "opentxs/Forward.hpp"

#include "opentxs/network/zeromq/socket/Request.hpp"
#include "opentxs/network/zeromq/Context.hpp"

namespace opentxs
{
namespace network
{
namespace zeromq
{
namespace socket
{
template <typename Input>
std::
    pair<opentxs::SendResult, opentxs::Pimpl<opentxs::network::zeromq::Message>>
    Request::Send(const Input& data) const noexcept
{
    return send_request(Context().Message(data));
}
}  // namespace socket
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif
