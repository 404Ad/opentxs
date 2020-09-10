// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/container/flat_map.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#if OT_BLOCKCHAIN
#include "internal/blockchain/p2p/bitcoin/Bitcoin.hpp"
#endif  // OT_BLOCKCHAIN
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/blockchain/Blockchain.hpp"
#include "opentxs/blockchain/p2p/Types.hpp"
#include "opentxs/protobuf/ContactEnums.pb.h"

namespace opentxs::blockchain::params
{
struct Data {
    using ChainData = boost::container::flat_map<blockchain::Type, Data>;
#if OT_BLOCKCHAIN
    using FilterData = boost::container::flat_map<
        blockchain::Type,
        boost::container::
            flat_map<filter::Type, std::pair<std::string, std::string>>>;
    using FilterTypes = std::map<Type, std::map<filter::Type, std::uint8_t>>;
    using ServiceBits = std::
        map<blockchain::Type, std::map<p2p::bitcoin::Service, p2p::Service>>;
#endif  // OT_BLOCKCHAIN

    struct Checkpoint {
        block::Height height_{};
        std::string block_hash_{};
        std::string previous_block_hash_{};
        std::string filter_header_{};
    };

    static const ChainData chains_;
#if OT_BLOCKCHAIN
    static const FilterData genesis_filters_;
    static const FilterTypes bip158_types_;
    static const ServiceBits service_bits_;
#endif  // OT_BLOCKCHAIN

    bool supported_{};
    bool testnet_{};
    proto::ContactItemType proto_{};
    std::string display_string_{};
    std::string display_ticker_{};
    unsigned int display_precision_{};
    std::int32_t nBits_{};
    std::string genesis_header_hex_{};
    std::string genesis_hash_hex_{};
    Checkpoint checkpoint_{};
    filter::Type default_filter_type_{};
    p2p::Protocol p2p_protocol_{};
    std::uint32_t p2p_magic_bits_{};
    std::uint16_t default_port_{};
    std::vector<std::string> dns_seeds_{};
    Amount default_fee_rate_{};
};
}  // namespace opentxs::blockchain::params
