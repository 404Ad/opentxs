// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <future>
#include <string>

#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/storage/Driver.hpp"
#include "opentxs/util/Bytes.hpp"
#include "storage/Plugin.hpp"
#include "util/LMDB.hpp"

namespace opentxs
{
namespace api
{
namespace network
{
class Asio;
}  // namespace network

namespace session
{
class Storage;
}  // namespace session

class Crypto;
}  // namespace api

namespace storage
{
class Config;
class Plugin;
}  // namespace storage

class Flag;
}  // namespace opentxs

namespace opentxs::storage::driver
{
// LMDB implementation of opentxs::storage
class LMDB final : public virtual implementation::Plugin,
                   public virtual storage::Driver
{
public:
    auto EmptyBucket(const bool bucket) const -> bool final;
    auto LoadFromBucket(
        const std::string& key,
        std::string& value,
        const bool bucket) const -> bool final;
    auto LoadRoot() const -> std::string final;
    auto StoreRoot(const bool commit, const std::string& hash) const
        -> bool final;

    void Cleanup() final;
    void Cleanup_LMDB();

    LMDB(
        const api::Crypto& crypto,
        const api::network::Asio& asio,
        const api::session::Storage& storage,
        const storage::Config& config,
        const Flag& bucket);

    ~LMDB() final;

private:
    using ot_super = Plugin;

    enum Table {
        Control = 0,
        A = 1,
        B = 2,
    };

    const lmdb::TableNames table_names_;
    lmdb::LMDB lmdb_;

    auto get_table(const bool bucket) const -> Table;
    void store(
        const bool isTransaction,
        const std::string& key,
        const std::string& value,
        const bool bucket,
        std::promise<bool>* promise) const final;

    void Init_LMDB();

    LMDB() = delete;
    LMDB(const LMDB&) = delete;
    LMDB(LMDB&&) = delete;
    auto operator=(const LMDB&) -> LMDB& = delete;
    auto operator=(LMDB&&) -> LMDB& = delete;
};
}  // namespace opentxs::storage::driver