# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(message)

target_sources(
  opentxs-common
  PRIVATE
    "${opentxs_SOURCE_DIR}/src/internal/blockchain/p2p/bitcoin/Bitcoin.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/blockchain/p2p/bitcoin/Factory.hpp"
    "Bitcoin.cpp"
    "Header.cpp"
    "Header.hpp"
    "Message.cpp"
    "Message.hpp"
    "Peer.cpp"
    "Peer.hpp"
)
target_include_directories(
  opentxs-common SYSTEM
  PRIVATE "${opentxs_SOURCE_DIR}/deps/robin-hood/src/include"
)
