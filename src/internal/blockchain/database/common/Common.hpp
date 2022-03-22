// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "internal/blockchain/p2p/P2P.hpp"
// IWYU pragma: no_include "opentxs/blockchain/BlockchainType.hpp"
// IWYU pragma: no_include "opentxs/blockchain/cfilter::Type.hpp"

#pragma once

#include <memory>
#include <tuple>
#include <utility>

#include "internal/blockchain/node/Node.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/Types.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"
#include "opentxs/util/Container.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs  // NOLINT
{
// inline namespace v1
// {
namespace blockchain
{
namespace p2p
{
namespace internal
{
struct Address;
}  // namespace internal
}  // namespace p2p
}  // namespace blockchain
// }  // namespace v1
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::blockchain::database::common
{
using Chain = Type;
using Address = p2p::internal::Address;
using Address_p = std::unique_ptr<Address>;
using FilterData = node::internal::FilterDatabase::Filter;
using FilterHash = node::internal::FilterDatabase::Hash;
using CFHeaderParams = node::internal::FilterDatabase::CFHeaderParams;
using Position = block::Position;
using Protocol = p2p::Protocol;
using Service = p2p::Service;
using Type = p2p::Network;
using UpdatedHeader = node::UpdatedHeader;
using SyncTableData = std::pair<int, UnallocatedCString>;

enum Table {
    BlockHeadersDeleted = 0,
    PeerDetails = 1,
    PeerChainIndex = 2,
    PeerProtocolIndex = 3,
    PeerServiceIndex = 4,
    PeerNetworkIndex = 5,
    PeerConnectedIndex = 6,
    FiltersBasicDeleted = 7,
    FiltersBCHDeleted = 8,
    FiltersOpentxsDeleted = 9,
    FilterHeadersBasic = 10,
    FilterHeadersBCH = 11,
    FilterHeadersOpentxs = 12,
    Config = 13,
    BlockIndex = 14,
    Enabled = 15,
    SyncTips = 16,
    ConfigMulti = 17,
    HeaderIndex = 18,
    FilterIndexBasic = 19,
    FilterIndexBCH = 20,
    FilterIndexES = 21,
    TransactionIndex = 22,
};

auto ChainToSyncTable(const opentxs::blockchain::Type chain) noexcept(false)
    -> int;
auto SyncTables() noexcept -> const UnallocatedVector<SyncTableData>&;
}  // namespace opentxs::blockchain::database::common
