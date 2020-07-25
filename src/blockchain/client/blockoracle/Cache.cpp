// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                       // IWYU pragma: associated
#include "1_Internal.hpp"                     // IWYU pragma: associated
#include "blockchain/client/BlockOracle.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#include "internal/blockchain/block/bitcoin/Bitcoin.hpp"
#include "internal/blockchain/client/Client.hpp"
#include "opentxs/Bytes.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/blockchain/block/bitcoin/Block.hpp"
#include "opentxs/blockchain/client/BlockOracle.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/network/zeromq/Frame.hpp"

#define OT_METHOD                                                              \
    "opentxs::blockchain::client::implementation::BlockOracle::Cache::"

namespace opentxs::blockchain::client::implementation
{
const std::size_t BlockOracle::Cache::cache_limit_{16};
const std::chrono::seconds BlockOracle::Cache::download_timeout_{60};

BlockOracle::Cache::Cache(
    const api::Core& api,
    const api::client::Blockchain& blockchain,
    const internal::Network& network,
    const internal::BlockDatabase& db,
    const blockchain::Type chain) noexcept
    : api_(api)
    , blockchain_(blockchain)
    , network_(network)
    , db_(db)
    , chain_(chain)
    , lock_()
    , pending_()
    , completed_()
    , running_(true)
{
}

auto BlockOracle::Cache::download(const block::Hash& block) const noexcept
    -> bool
{
    return network_.RequestBlock(block);
}

auto BlockOracle::Cache::ReceiveBlock(const zmq::Frame& in) const noexcept
    -> void
{
    auto pBlock = factory::BitcoinBlock(api_, blockchain_, chain_, in.Bytes());

    if (false == bool(pBlock)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid block").Flush();

        return;
    }

    Lock lock{lock_};
    auto& block = *pBlock;

    if (api::client::blockchain::BlockStorage::None != db_.BlockPolicy()) {
        db_.BlockStore(block);
    }

    const auto& id = block.ID();
    auto pending = pending_.find(id);

    if (pending_.end() == pending) {
        LogVerbose(OT_METHOD)(__FUNCTION__)(
            ": Received block not in request list")
            .Flush();

        return;
    }

    auto& [time, promise, future, queued] = pending->second;
    promise.set_value(std::move(pBlock));
    completed_.emplace_back(id, std::move(future));
    pending_.erase(pending);
    LogVerbose(OT_METHOD)(__FUNCTION__)(": Cached block ")(id.asHex()).Flush();
}

auto BlockOracle::Cache::Request(const block::Hash& block) const noexcept
    -> BitcoinBlockFuture
{
    const auto output = Request(BlockHashes{block});

    OT_ASSERT(1 == output.size());

    return output.at(0);
}

auto BlockOracle::Cache::Request(const BlockHashes& hashes) const noexcept
    -> BitcoinBlockFutures
{
    auto output = BitcoinBlockFutures{};
    output.reserve(hashes.size());
    auto download = std::map<block::pHash, BitcoinBlockFutures::iterator>{};
    Lock lock{lock_};

    if (false == running_) {
        for (auto i = std::size_t{0}; i < hashes.size(); ++i) {
            auto promise = Promise{};
            promise.set_value(nullptr);
            output.emplace_back(promise.get_future());
        }

        return output;
    }

    for (const auto& block : hashes) {
        for (const auto& [hash, future] : completed_) {
            if (block == hash) {
                output.emplace_back(future);

                continue;
            }
        }

        {
            auto it = pending_.find(block);

            if (pending_.end() != it) {
                const auto& [time, promise, future, queued] = it->second;
                output.emplace_back(future);

                continue;
            }
        }

        if (auto pBlock = db_.BlockLoadBitcoin(block); bool(pBlock)) {
            auto promise = Promise{};
            promise.set_value(std::move(pBlock));
            output.emplace_back(
                completed_.emplace_back(block, promise.get_future()).second);

            continue;
        }

        output.emplace_back();
        auto it = output.begin();
        std::advance(it, output.size() - 1);
        download.emplace(block, it);
    }

    if (0 < download.size()) {
        auto blockList = std::vector<ReadView>{};
        std::transform(
            std::begin(download),
            std::end(download),
            std::back_inserter(blockList),
            [](const auto& in) -> auto {
                const auto& [key, value] = in;

                return key->Bytes();
            });
        const auto messageSent = network_.RequestBlocks(blockList);

        for (auto& [hash, futureOut] : download) {
            auto& [time, promise, future, queued] = pending_[hash];
            time = Clock::now();
            future = promise.get_future();
            *futureOut = future;
            queued = messageSent;
        }
    }

    return output;
}

auto BlockOracle::Cache::Shutdown() noexcept -> void
{
    Lock lock{lock_};

    if (running_) {
        running_ = false;
        completed_.clear();

        for (auto& [hash, item] : pending_) {
            auto& [time, promise, future, queued] = item;
            promise.set_value(nullptr);
        }

        pending_.clear();
    }
}

auto BlockOracle::Cache::StateMachine() const noexcept -> bool
{
    Lock lock{lock_};

    if (false == running_) { return false; }

    while (completed_.size() > cache_limit_) { completed_.pop_front(); }

    for (auto& [hash, item] : pending_) {
        auto& [time, promise, future, queued] = item;
        const auto now = Clock::now();
        const auto timeout = download_timeout_ <= (now - time);

        if (timeout || (false == queued)) {
            queued = download(hash);
            time = now;
        }
    }

    return 0 < pending_.size();
}
}  // namespace opentxs::blockchain::client::implementation
