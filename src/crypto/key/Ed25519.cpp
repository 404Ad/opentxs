// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "Internal.hpp"

#if OT_CRYPTO_SUPPORTED_KEY_ED25519
#include "opentxs/api/crypto/Crypto.hpp"
#include "opentxs/api/Native.hpp"
#include "opentxs/core/util/Timer.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/crypto/key/Ed25519.hpp"
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/crypto/library/Sodium.hpp"
#include "opentxs/OT.hpp"

#include "Ed25519.hpp"

namespace opentxs
{
crypto::key::Ed25519* Factory::Ed25519Key(const proto::AsymmetricKey& input)
{
    return new crypto::key::implementation::Ed25519(input);
}

crypto::key::Ed25519* Factory::Ed25519Key(const String& input)
{
    return new crypto::key::implementation::Ed25519(input);
}

crypto::key::Ed25519* Factory::Ed25519Key(const proto::KeyRole input)
{
    return new crypto::key::implementation::Ed25519(input);
}
}  // namespace opentxs

namespace opentxs::crypto::key::implementation
{
Ed25519::Ed25519()
    : ot_super(proto::AKEYTYPE_ED25519, proto::KEYROLE_ERROR)
{
}

Ed25519::Ed25519(const proto::KeyRole role)
    : ot_super(proto::AKEYTYPE_ED25519, role)
{
}

Ed25519::Ed25519(const proto::AsymmetricKey& serializedKey)
    : ot_super(serializedKey)
{
}

Ed25519::Ed25519(const String& publicKey)
    : ot_super(proto::AKEYTYPE_ED25519, publicKey)
{
}

const crypto::EcdsaProvider& Ed25519::ECDSA() const
{
    return dynamic_cast<const crypto::Sodium&>(engine());
}

const crypto::AsymmetricProvider& Ed25519::engine() const
{
    return OT::App().Crypto().ED25519();
}

bool Ed25519::hasCapability(const NymCapability& capability) const
{
    switch (capability) {
        case (NymCapability::AUTHENTICATE_CONNECTION): {

            return true;
        }
        default: {
            return ot_super::hasCapability(capability);
        }
    }
}
}  // namespace opentxs::crypto::key::implementation
#endif  // OT_CRYPTO_SUPPORTED_KEY_ED25519
