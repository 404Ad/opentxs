// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CRYPTO_KEY_KEYPAIR_HPP
#define OPENTXS_CRYPTO_KEY_KEYPAIR_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Proto.hpp"
#include "opentxs/Types.hpp"

#include <cstdint>
#include <list>
#include <memory>

namespace opentxs
{
namespace crypto
{
namespace key
{
class Keypair
{
public:
    using Keys = std::list<const Asymmetric*>;

    EXPORT virtual operator bool() const noexcept = 0;

    EXPORT virtual bool CheckCapability(const NymCapability& capability) const
        noexcept = 0;
    /// throws std::runtime_error if private key is missing
    EXPORT virtual const Asymmetric& GetPrivateKey() const noexcept(false) = 0;
    /// throws std::runtime_error if public key is missing
    EXPORT virtual const Asymmetric& GetPublicKey() const noexcept(false) = 0;
    // inclusive means, return keys when theSignature has no metadata.
    EXPORT virtual std::int32_t GetPublicKeyBySignature(
        Keys& listOutput,
        const Signature& theSignature,
        bool bInclusive = false) const noexcept = 0;
    EXPORT virtual std::shared_ptr<proto::AsymmetricKey> GetSerialized(
        bool privateKey) const noexcept = 0;
    EXPORT virtual bool GetTransportKey(
        Data& publicKey,
        OTPassword& privateKey,
        const PasswordPrompt& reason) const noexcept = 0;

    EXPORT virtual ~Keypair() = default;

protected:
    Keypair() = default;

private:
    friend OTKeypair;

    virtual Keypair* clone() const = 0;

    Keypair(const Keypair&) = delete;
    Keypair(Keypair&&) = delete;
    Keypair& operator=(const Keypair&) = delete;
    Keypair& operator=(Keypair&&) = delete;
};
}  // namespace key
}  // namespace crypto
}  // namespace opentxs
#endif
