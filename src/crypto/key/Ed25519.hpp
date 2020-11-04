// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "crypto/key/HD.hpp"
#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/Version.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/key/Ed25519.hpp"
#include "opentxs/protobuf/Enums.pb.h"

namespace opentxs
{
namespace api
{
namespace internal
{
struct Core;
}  // namespace internal
}  // namespace api

namespace crypto
{
namespace key
{
class Symmetric;
}  // namespace key

class EcdsaProvider;
}  // namespace crypto

namespace proto
{
class AsymmetricKey;
class HDPath;
}  // namespace proto

class Data;
class OTPassword;
class PasswordPrompt;
class Secret;
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
class Ed25519 final : virtual public key::Ed25519, public HD
{
public:
    auto CreateType() const -> NymParameterType final
    {
        return NymParameterType::ed25519;
    }
    auto TransportKey(
        Data& publicKey,
        Secret& privateKey,
        const PasswordPrompt& reason) const noexcept -> bool final;

    Ed25519(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const proto::KeyRole role,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
#if OT_CRYPTO_WITH_BIP32
    Ed25519(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const Secret& privateKey,
        const Secret& chainCode,
        const Data& publicKey,
        const proto::HDPath& path,
        const Bip32Fingerprint parent,
        const proto::KeyRole role,
        const VersionNumber version,
        key::Symmetric& sessionKey,
        const PasswordPrompt& reason) noexcept(false);
#endif  // OT_CRYPTO_WITH_BIP32
    Ed25519(
        const api::internal::Core& api,
        const crypto::EcdsaProvider& ecdsa,
        const proto::AsymmetricKey& serializedKey) noexcept(false);

    ~Ed25519() final = default;

private:
    using ot_super = HD;

    auto clone() const noexcept -> Ed25519* final { return new Ed25519(*this); }
    auto clone_ec() const noexcept -> Ed25519* final { return clone(); }
    auto get_public() const -> std::shared_ptr<proto::AsymmetricKey> final
    {
        return serialize_public(clone());
    }

    Ed25519() = delete;
    Ed25519(const Ed25519&) noexcept;
    Ed25519(Ed25519&&) = delete;
    auto operator=(const Ed25519&) -> Ed25519& = delete;
    auto operator=(Ed25519 &&) -> Ed25519& = delete;
};
}  // namespace opentxs::crypto::key::implementation
