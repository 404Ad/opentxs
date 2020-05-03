// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                     // IWYU pragma: associated
#include "1_Internal.hpp"                   // IWYU pragma: associated
#include "network/zeromq/curve/Server.hpp"  // IWYU pragma: associated

#include <array>
#include <cstdint>
#include <utility>
#include <zmq.h>

#include "network/zeromq/socket/Socket.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/crypto/OTPassword.hpp"

#define OT_METHOD "opentxs::network::zeromq::curve::implementation::Server::"

namespace opentxs::network::zeromq::curve::implementation
{
Server::Server(zeromq::socket::implementation::Socket& socket) noexcept
    : parent_(socket)
{
}

bool Server::SetDomain(const std::string& domain) const noexcept
{
    auto set =
        zmq_setsockopt(parent_, ZMQ_ZAP_DOMAIN, domain.data(), domain.size());

    if (0 != set) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to set domain.").Flush();

        return false;
    }

    return true;
}

bool Server::SetPrivateKey(const OTPassword& key) const noexcept
{
    if (CURVE_KEY_BYTES != key.getMemorySize()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key.").Flush();

        return false;
    }

    return set_private_key(key.getMemory(), key.getMemorySize());
}

bool Server::SetPrivateKey(const std::string& z85) const noexcept
{
    if (CURVE_KEY_Z85_BYTES > z85.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid private key size (")(
            z85.size())(").")
            .Flush();

        return false;
    }

    std::array<std::uint8_t, CURVE_KEY_BYTES> key;
    ::zmq_z85_decode(key.data(), z85.data());

    return set_private_key(key.data(), key.size());
}

bool Server::set_private_key(const void* key, const std::size_t keySize) const
    noexcept
{
    OT_ASSERT(nullptr != parent_);

    socket::implementation::Socket::SocketCallback cb{[&](const Lock&) -> bool {
        const int server{1};
        auto set =
            zmq_setsockopt(parent_, ZMQ_CURVE_SERVER, &server, sizeof(server));

        if (0 != set) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Failed to set ZMQ_CURVE_SERVER")
                .Flush();

            return false;
        }

        set = zmq_setsockopt(parent_, ZMQ_CURVE_SECRETKEY, key, keySize);

        if (0 != set) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to set private key.")
                .Flush();

            return false;
        }

        return true;
    }};

    return parent_.apply_socket(std::move(cb));
}
}  // namespace opentxs::network::zeromq::curve::implementation
