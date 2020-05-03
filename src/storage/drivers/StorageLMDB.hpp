// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/storage/drivers/StorageLMDB.cpp"

#pragma once

#include <future>
#include <string>

#include "opentxs/Bytes.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/api/storage/Driver.hpp"
#include "storage/Plugin.hpp"
#include "util/LMDB.hpp"

namespace opentxs
{
namespace api
{
namespace storage
{
class Storage;
}  // namespace storage
}  // namespace api

class Factory;
class Flag;
class StorageConfig;
}  // namespace opentxs

namespace opentxs::storage::implementation
{
// LMDB implementation of opentxs::storage
class StorageLMDB final : public virtual Plugin,
                          public virtual opentxs::api::storage::Driver
{
public:
    bool EmptyBucket(const bool bucket) const final;
    bool LoadFromBucket(
        const std::string& key,
        std::string& value,
        const bool bucket) const final;
    std::string LoadRoot() const final;
    bool StoreRoot(const bool commit, const std::string& hash) const final;

    void Cleanup() final;
    void Cleanup_StorageLMDB();

    ~StorageLMDB() final;

private:
    using ot_super = Plugin;

    friend Factory;

    enum Table {
        Control = 0,
        A = 1,
        B = 2,
    };

    const lmdb::TableNames table_names_;
    lmdb::LMDB lmdb_;

    Table get_table(const bool bucket) const;
    void store(
        const bool isTransaction,
        const std::string& key,
        const std::string& value,
        const bool bucket,
        std::promise<bool>* promise) const final;

    void Init_StorageLMDB();

    StorageLMDB(
        const api::storage::Storage& storage,
        const StorageConfig& config,
        const Digest& hash,
        const Random& random,
        const Flag& bucket);
    StorageLMDB() = delete;
    StorageLMDB(const StorageLMDB&) = delete;
    StorageLMDB(StorageLMDB&&) = delete;
    StorageLMDB& operator=(const StorageLMDB&) = delete;
    StorageLMDB& operator=(StorageLMDB&&) = delete;
};
}  // namespace opentxs::storage::implementation
