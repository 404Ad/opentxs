// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"         // IWYU pragma: associated
#include "1_Internal.hpp"       // IWYU pragma: associated
#include "api/crypto/Hash.hpp"  // IWYU pragma: associated

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include "Factory.hpp"
#include "internal/crypto/library/Pbkdf2.hpp"
#include "internal/crypto/library/Ripemd160.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/api/crypto/Hash.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/library/HashingProvider.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/protobuf/Enums.pb.h"
#include "smhasher/src/MurmurHash3.h"

#define OT_METHOD "opentxs::api::crypto::implementation::Hash::"

namespace opentxs
{
using ReturnType = api::crypto::implementation::Hash;

auto Factory::Hash(
    const api::crypto::Encode& encode,
    const crypto::HashingProvider& ssl,
    const crypto::HashingProvider& sodium,
    const crypto::Pbkdf2& pbkdf2,
    const crypto::Ripemd160& ripe) noexcept
    -> std::unique_ptr<api::crypto::Hash>
{
    return std::make_unique<ReturnType>(encode, ssl, sodium, pbkdf2, ripe);
}
}  // namespace opentxs

namespace opentxs::api::crypto::implementation
{
using Provider = opentxs::crypto::HashingProvider;

Hash::Hash(
    const api::crypto::Encode& encode,
    const Provider& ssl,
    const Provider& sodium,
    const opentxs::crypto::Pbkdf2& pbkdf2,
    const opentxs::crypto::Ripemd160& ripe) noexcept
    : encode_(encode)
    , ssl_(ssl)
    , sodium_(sodium)
    , pbkdf2_(pbkdf2)
    , ripe_(ripe)
{
}

auto Hash::allocate(
    const proto::HashType type,
    const AllocateOutput destination) noexcept -> WritableView
{
    if (false == bool(destination)) { return {}; }

    return destination(Provider::HashSize(type));
}

auto Hash::bitcoin_hash_160(
    const void* input,
    const std::size_t size,
    void* output) const noexcept -> bool
{
    auto temp = space(Provider::HashSize(proto::HASHTYPE_SHA256));

    if (false == digest(proto::HASHTYPE_SHA256, input, size, temp.data())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate intermediate hash.")
            .Flush();

        return false;
    }

    return digest(proto::HASHTYPE_RIPEMD160, temp.data(), temp.size(), output);
}

auto Hash::Digest(
    const proto::HashType type,
    const ReadView data,
    const AllocateOutput destination) const noexcept -> bool
{
    auto view = allocate(type, destination);

    if (false == view.valid()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to allocate output space.")
            .Flush();

        return false;
    }

    return digest(type, data.data(), data.size(), view);
}

auto Hash::Digest(
    const proto::HashType type,
    const opentxs::network::zeromq::Frame& data,
    const AllocateOutput destination) const noexcept -> bool
{
    auto view = allocate(type, destination);

    if (false == view.valid()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to allocate output space.")
            .Flush();

        return false;
    }

    return digest(type, data.data(), data.size(), view);
}

auto Hash::Digest(
    const std::uint32_t hash,
    const ReadView data,
    const AllocateOutput destination) const noexcept -> bool
{
    if (false == bool(destination)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid output allocator")
            .Flush();

        return false;
    }

    const auto type = static_cast<proto::HashType>(hash);
    auto temp =
        Data::Factory();  // FIXME IdentifierEncode should accept ReadView
    auto view = allocate(type, temp->WriteInto());

    if (false == view.valid()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to allocate temp space")
            .Flush();

        return false;
    }

    if (digest(type, data.data(), data.size(), view)) {
        const auto encoded = encode_.IdentifierEncode(temp);
        auto output = destination(encoded.size());

        if (false == output.valid(encoded.size())) {
            LogOutput(OT_METHOD)(__FUNCTION__)(
                ": Unable to allocate encoded output space")
                .Flush();

            return false;
        }

        std::memcpy(output, encoded.data(), output);

        return true;
    } else {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Failed to calculate digest")
            .Flush();

        return false;
    }
}

auto Hash::digest(
    const proto::HashType type,
    const void* input,
    const std::size_t size,
    void* output) const noexcept -> bool
{
    switch (type) {
        case proto::HASHTYPE_SHA1:
        case proto::HASHTYPE_SHA256:
        case proto::HASHTYPE_SHA512: {
            return ssl_.Digest(
                type,
                static_cast<const std::uint8_t*>(input),
                size,
                static_cast<std::uint8_t*>(output));
        }
        case proto::HASHTYPE_BLAKE2B160:
        case proto::HASHTYPE_BLAKE2B256:
        case proto::HASHTYPE_BLAKE2B512: {
            return sodium_.Digest(
                type,
                static_cast<const std::uint8_t*>(input),
                size,
                static_cast<std::uint8_t*>(output));
        }
        case proto::HASHTYPE_RIPEMD160: {
            return ripe_.RIPEMD160(
                static_cast<const std::uint8_t*>(input),
                size,
                static_cast<std::uint8_t*>(output));
        }
        case proto::HASHTYPE_SHA256D: {
            return sha_256_double(input, size, output);
        }
        case proto::HASHTYPE_SHA256DC: {
            return sha_256_double_checksum(input, size, output);
        }
        case proto::HASHTYPE_BITCOIN: {
            return bitcoin_hash_160(input, size, output);
        }
        default: {
        }
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported hash type.").Flush();

    return false;
}

auto Hash::HMAC(
    const proto::HashType type,
    const ReadView key,
    const ReadView& data,
    const AllocateOutput digest) const noexcept -> bool
{
    auto output = allocate(type, digest);

    if (false == output.valid()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Unable to allocate output space.")
            .Flush();

        return false;
    }

    return HMAC(
        type,
        reinterpret_cast<const std::uint8_t*>(data.data()),
        data.size(),
        reinterpret_cast<const std::uint8_t*>(key.data()),
        key.size(),
        output.as<std::uint8_t>());
}

auto Hash::HMAC(
    const proto::HashType type,
    const std::uint8_t* input,
    const std::size_t size,
    const std::uint8_t* key,
    const std::size_t keySize,
    std::uint8_t* output) const noexcept -> bool
{
    switch (type) {
        case proto::HASHTYPE_SHA256:
        case proto::HASHTYPE_SHA512: {
            return ssl_.HMAC(type, input, size, key, keySize, output);
        }
        case proto::HASHTYPE_BLAKE2B160:
        case proto::HASHTYPE_BLAKE2B256:
        case proto::HASHTYPE_BLAKE2B512:
        case proto::HASHTYPE_SIPHASH24: {
            return sodium_.HMAC(type, input, size, key, keySize, output);
        }
        default: {
        }
    }

    LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported hash type.").Flush();

    return false;
}

auto Hash::MurmurHash3_32(
    const std::uint32_t& key,
    const Data& data,
    std::uint32_t& output) const noexcept -> void
{
    MurmurHash3_x86_32(data.data(), data.size(), key, &output);
}

auto Hash::PKCS5_PBKDF2_HMAC(
    const Data& input,
    const Data& salt,
    const std::size_t iterations,
    const proto::HashType type,
    const std::size_t bytes,
    Data& output) const noexcept -> bool
{
    output.SetSize(bytes);

    return pbkdf2_.PKCS5_PBKDF2_HMAC(
        input.data(),
        input.size(),
        salt.data(),
        salt.size(),
        iterations,
        type,
        bytes,
        output.data());
}

auto Hash::PKCS5_PBKDF2_HMAC(
    const Secret& input,
    const Data& salt,
    const std::size_t iterations,
    const proto::HashType type,
    const std::size_t bytes,
    Data& output) const noexcept -> bool
{
    output.SetSize(bytes);
    const auto data = input.Bytes();

    return pbkdf2_.PKCS5_PBKDF2_HMAC(
        data.data(),
        data.size(),
        salt.data(),
        salt.size(),
        iterations,
        type,
        bytes,
        output.data());
}

auto Hash::PKCS5_PBKDF2_HMAC(
    const std::string& input,
    const Data& salt,
    const std::size_t iterations,
    const proto::HashType type,
    const std::size_t bytes,
    Data& output) const noexcept -> bool
{
    output.SetSize(bytes);

    return pbkdf2_.PKCS5_PBKDF2_HMAC(
        input.data(),
        input.size(),
        salt.data(),
        salt.size(),
        iterations,
        type,
        bytes,
        output.data());
}

auto Hash::sha_256_double(
    const void* input,
    const std::size_t size,
    void* output) const noexcept -> bool
{
    auto temp = space(Provider::HashSize(proto::HASHTYPE_SHA256));

    if (false == digest(proto::HASHTYPE_SHA256, input, size, temp.data())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate intermediate hash.")
            .Flush();

        return false;
    }

    return digest(proto::HASHTYPE_SHA256, temp.data(), temp.size(), output);
}

auto Hash::sha_256_double_checksum(
    const void* input,
    const std::size_t size,
    void* output) const noexcept -> bool
{
    auto temp = space(Provider::HashSize(proto::HASHTYPE_SHA256));

    if (false == sha_256_double(input, size, temp.data())) {
        LogOutput(OT_METHOD)(__FUNCTION__)(
            ": Failed to calculate intermediate hash.")
            .Flush();

        return false;
    }

    std::memcpy(output, temp.data(), 4);

    return true;
}
}  // namespace opentxs::api::crypto::implementation
