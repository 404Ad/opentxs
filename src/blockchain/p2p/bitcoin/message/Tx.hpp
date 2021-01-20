// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/blockchain/p2p/bitcoin/message/Tx.cpp"

#pragma once

#include <memory>
#include <set>

#include "blockchain/p2p/bitcoin/Message.hpp"
#include "blockchain/p2p/bitcoin/Message.hpp"
#include "internal/blockchain/p2p/bitcoin/Bitcoin.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/BlockchainType.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/core/Data.hpp"

namespace opentxs
{
namespace api
{
class Core;
}  // namespace api

namespace blockchain
{
namespace p2p
{
namespace bitcoin
{
class Header;
}  // namespace bitcoin
}  // namespace p2p
}  // namespace blockchain
}  // namespace opentxs

namespace opentxs::blockchain::p2p::bitcoin::message
{
class Tx final : virtual public implementation::Message
{
public:
    auto getRawTx() const noexcept -> OTData { return Data::Factory(raw_tx_); }

    Tx(const api::Core& api,
       const blockchain::Type network,
       const ReadView transaction) noexcept;
    Tx(const api::Core& api,
       std::unique_ptr<Header> header,
       const ReadView transaction) noexcept(false);

    ~Tx() final = default;

private:
    const OTData raw_tx_;

    auto payload() const noexcept -> OTData final;

    Tx(const Tx&) = delete;
    Tx(Tx&&) = delete;
    auto operator=(const Tx&) -> Tx& = delete;
    auto operator=(Tx&&) -> Tx& = delete;
};
}  // namespace opentxs::blockchain::p2p::bitcoin::message
