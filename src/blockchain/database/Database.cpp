// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                      // IWYU pragma: associated
#include "1_Internal.hpp"                    // IWYU pragma: associated
#include "blockchain/database/Database.hpp"  // IWYU pragma: associated

#include <memory>
#include <string>

#include "internal/api/Api.hpp"
#include "internal/blockchain/Blockchain.hpp"
#include "opentxs/core/Log.hpp"
#include "util/LMDB.hpp"

// #define OT_METHOD "opentxs::blockchain::implementation::Database::"

namespace opentxs::factory
{
auto BlockchainDatabase(
    const api::internal::Core& api,
    const api::client::internal::Blockchain& blockchain,
    const blockchain::client::internal::Network& network,
    const api::client::blockchain::database::implementation::Database& common,
    const blockchain::Type type) noexcept
    -> std::unique_ptr<blockchain::internal::Database>
{
    using ReturnType = blockchain::implementation::Database;

    return std::make_unique<ReturnType>(api, blockchain, network, common, type);
}
}  // namespace opentxs::factory

namespace opentxs::blockchain::client::internal
{
const VersionNumber WalletDatabase::DefaultIndexVersion{1};
}  // namespace opentxs::blockchain::client::internal

namespace opentxs::blockchain::implementation
{
template <typename Input>
auto tsv(const Input& in) noexcept -> ReadView
{
    return {reinterpret_cast<const char*>(&in), sizeof(in)};
}

const std::size_t Database::db_version_{1};
const opentxs::storage::lmdb::TableNames Database::table_names_{
    {database::Config, "config"},
    {database::BlockHeaderMetadata, "block_header_metadata"},
    {database::BlockHeaderBest, "best_header_chain"},
    {database::ChainData, "block_header_data"},
    {database::BlockHeaderSiblings, "block_siblings"},
    {database::BlockHeaderDisconnected, "disconnected_block_headers"},
    {database::BlockFilterBest, "filter_tips"},
    {database::BlockFilterHeaderBest, "filter_header_tips"},
};

Database::Database(
    const api::internal::Core& api,
    const api::client::internal::Blockchain& blockchain,
    const client::internal::Network& network,
    const database::Common& common,
    const blockchain::Type type) noexcept
    : chain_(type)
    , common_(common)
    , lmdb_(
          table_names_,
          common.AllocateStorageFolder(
              std::to_string(static_cast<std::uint32_t>(type))),
          {{database::Config, MDB_INTEGERKEY},
           {database::BlockHeaderMetadata, 0},
           {database::BlockHeaderBest, MDB_INTEGERKEY},
           {database::ChainData, MDB_INTEGERKEY},
           {database::BlockHeaderSiblings, 0},
           {database::BlockHeaderDisconnected, MDB_DUPSORT},
           {database::BlockFilterBest, MDB_INTEGERKEY},
           {database::BlockFilterHeaderBest, MDB_INTEGERKEY}},
          0)
    , blocks_(api, common_, type)
    , filters_(api, common_, lmdb_, type)
    , headers_(api, network, common_, lmdb_, type)
    , wallet_(api, blockchain, chain_)
{
    init_db();
}

auto Database::init_db() noexcept -> void
{
    if (false == lmdb_.Exists(database::Config, tsv(database::Key::Version))) {
        const auto stored = lmdb_.Store(
            database::Config, tsv(database::Key::Version), tsv(db_version_));

        OT_ASSERT(stored.first);
    }
}
}  // namespace opentxs::blockchain::implementation
