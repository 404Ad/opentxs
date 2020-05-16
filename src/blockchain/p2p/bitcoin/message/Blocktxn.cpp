// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "blockchain/p2p/bitcoin/message/Blocktxn.hpp"  // IWYU pragma: associated

#include <iosfwd>
#include <utility>

#include "Factory.hpp"
#include "blockchain/p2p/bitcoin/Header.hpp"
#include "blockchain/p2p/bitcoin/Message.hpp"
#include "internal/blockchain/p2p/bitcoin/Bitcoin.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"

//#define OT_METHOD "opentxs::blockchain::p2p::bitcoin::message::Blocktxn::"

namespace opentxs
{
auto Factory::BitcoinP2PBlocktxn(
    const api::internal::Core& api,
    std::unique_ptr<blockchain::p2p::bitcoin::Header> pHeader,
    const blockchain::p2p::bitcoin::ProtocolVersion version,
    const void* payload,
    const std::size_t size)
    -> blockchain::p2p::bitcoin::message::internal::Blocktxn*
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::implementation::Blocktxn;

    if (false == bool(pHeader)) {
        LogOutput("opentxs::Factory::")(__FUNCTION__)(": Invalid header")
            .Flush();

        return nullptr;
    }

    const auto raw_Blocktxn(Data::Factory(payload, size));

    return new ReturnType(api, std::move(pHeader), raw_Blocktxn);
}

auto Factory::BitcoinP2PBlocktxn(
    const api::internal::Core& api,
    const blockchain::Type network,
    const Data& payload)
    -> blockchain::p2p::bitcoin::message::internal::Blocktxn*
{
    namespace bitcoin = blockchain::p2p::bitcoin;
    using ReturnType = bitcoin::message::implementation::Blocktxn;

    return new ReturnType(api, network, payload);
}
}  // namespace opentxs

namespace opentxs::blockchain::p2p::bitcoin::message::implementation
{
Blocktxn::Blocktxn(
    const api::internal::Core& api,
    const blockchain::Type network,
    const Data& payload) noexcept
    : Message(api, network, bitcoin::Command::blocktxn)
    , payload_(Data::Factory(payload))
{
    init_hash();
}

Blocktxn::Blocktxn(
    const api::internal::Core& api,
    std::unique_ptr<Header> header,
    const Data& payload) noexcept
    : Message(api, std::move(header))
    , payload_(payload)
{
}
}  // namespace opentxs::blockchain::p2p::bitcoin::message::implementation
