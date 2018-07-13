/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#ifndef OPENTXS_CRYPTO_LIBRARY_SECP256K1_HPP
#define OPENTXS_CRYPTO_LIBRARY_SECP256K1_HPP

#include "Internal.hpp"

#if OT_CRYPTO_USING_LIBSECP256K1
#include "opentxs/crypto/library/AsymmetricProvider.hpp"
#include "opentxs/crypto/library/EcdsaProvider.hpp"

namespace opentxs::crypto
{
class Secp256k1 : virtual public AsymmetricProvider,
                  virtual public EcdsaProvider
{
public:
    EXPORT virtual void Init() = 0;

    EXPORT virtual ~Secp256k1() = default;

protected:
    Secp256k1() = default;

private:
    Secp256k1(const Secp256k1&) = delete;
    Secp256k1(Secp256k1&&) = delete;
    Secp256k1& operator=(const Secp256k1&) = delete;
    Secp256k1& operator=(Secp256k1&&) = delete;
};
}  // namespace opentxs::crypto
#endif  // OT_CRYPTO_USING_LIBSECP256K1
#endif  // OPENTXS_CORE_CRYPTO_OTCRYPTO_HPP
