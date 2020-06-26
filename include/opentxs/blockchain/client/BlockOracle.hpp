// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_BLOCKCHAIN_CLIENT_BLOCKORACLE_HPP
#define OPENTXS_BLOCKCHAIN_CLIENT_BLOCKORACLE_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <future>
#include <memory>

#include "opentxs/blockchain/Blockchain.hpp"

namespace opentxs
{
namespace blockchain
{
namespace client
{
class BlockOracle
{
public:
    using BitcoinBlock = block::bitcoin::Block;
    using BitcoinBlock_p = std::shared_ptr<const BitcoinBlock>;
    using BitcoinBlockFuture = std::shared_future<BitcoinBlock_p>;

    OPENTXS_EXPORT virtual auto LoadBitcoin(
        const block::Hash& block) const noexcept -> BitcoinBlockFuture = 0;

    virtual ~BlockOracle() = default;

protected:
    BlockOracle() noexcept = default;

private:
    BlockOracle(const BlockOracle&) = delete;
    BlockOracle(BlockOracle&&) = delete;
    BlockOracle& operator=(const BlockOracle&) = delete;
    BlockOracle& operator=(BlockOracle&&) = delete;
};
}  // namespace client
}  // namespace blockchain
}  // namespace opentxs
#endif
