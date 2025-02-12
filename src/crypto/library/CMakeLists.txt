# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(openssl)

if(PACKETCRYPT_EXPORT)
  add_subdirectory(packetcrypt)
endif()

add_subdirectory(secp256k1)
add_subdirectory(sodium)

target_sources(
  opentxs-common
  PRIVATE
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Factory.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Null.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Pbkdf2.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Ripemd160.hpp"
    "${opentxs_SOURCE_DIR}/src/internal/crypto/library/Scrypt.hpp"
    "AsymmetricProvider.cpp"
    "AsymmetricProvider.hpp"
    "EcdsaProvider.cpp"
    "EcdsaProvider.hpp"
    "HashingProvider.cpp"
)
set(cxx-install-headers
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/AsymmetricProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/EcdsaProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/EncodingProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/HashingProvider.hpp"
    "${opentxs_SOURCE_DIR}/include/opentxs/crypto/library/SymmetricProvider.hpp"
)
target_include_directories(
  opentxs-common SYSTEM
  PRIVATE "${opentxs_SOURCE_DIR}/deps/robin-hood/src/include"
)
target_link_libraries(
  opentxs-common
  PRIVATE
    Boost::headers
    opentxs::messages
    unofficial-sodium::sodium
)
target_link_libraries(opentxs PUBLIC unofficial-sodium::sodium)
target_sources(opentxs-common PRIVATE ${cxx-install-headers})

if(OPENTXS_STANDALONE)
  install(
    FILES ${cxx-install-headers}
    DESTINATION include/opentxs/crypto/library
    COMPONENT dev
  )
endif()
