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

#include "stdafx.hpp"

#include "opentxs/crypto/library/LegacySymmetricProvider.hpp"

#include "opentxs/core/String.hpp"

namespace opentxs::crypto
{
String LegacySymmetricProvider::ModeToString(const Mode Mode)
{
    String modeString;

    switch (Mode) {
        case LegacySymmetricProvider::AES_128_CBC:
            modeString = "aes-128-cbc";
            break;
        case LegacySymmetricProvider::AES_256_CBC:
            modeString = "aes-256-cbc";
            break;
        case LegacySymmetricProvider::AES_256_ECB:
            modeString = "aes-256-ecb";
            break;
        case LegacySymmetricProvider::AES_128_GCM:
            modeString = "aes-128-gcm";
            break;
        case LegacySymmetricProvider::AES_256_GCM:
            modeString = "aes-256-gcm";
            break;
        default:
            modeString = "error";
    }
    return modeString;
}

LegacySymmetricProvider::Mode LegacySymmetricProvider::StringToMode(
    const String& Mode)
{
    if (Mode.Compare("aes-128-cbc"))
        return LegacySymmetricProvider::AES_128_CBC;
    if (Mode.Compare("aes-256-cbc"))
        return LegacySymmetricProvider::AES_256_CBC;
    if (Mode.Compare("aes-256-ecb"))
        return LegacySymmetricProvider::AES_256_ECB;
    if (Mode.Compare("aes-128-gcm"))
        return LegacySymmetricProvider::AES_128_GCM;
    if (Mode.Compare("aes-256-gcm"))
        return LegacySymmetricProvider::AES_256_GCM;
    return LegacySymmetricProvider::ERROR_MODE;
}

std::uint32_t LegacySymmetricProvider::KeySize(const Mode Mode)
{
    std::uint32_t keySize;

    switch (Mode) {
        case LegacySymmetricProvider::AES_128_CBC:
            keySize = 16;
            break;
        case LegacySymmetricProvider::AES_256_CBC:
            keySize = 32;
            break;
        case LegacySymmetricProvider::AES_256_ECB:
            keySize = 32;
            break;
        case LegacySymmetricProvider::AES_128_GCM:
            keySize = 16;
            break;
        case LegacySymmetricProvider::AES_256_GCM:
            keySize = 32;
            break;
        default:
            keySize = 0;
    }
    return keySize;
}

std::uint32_t LegacySymmetricProvider::IVSize(const Mode Mode)
{
    return KeySize(Mode);
}

std::uint32_t LegacySymmetricProvider::TagSize(const Mode Mode)
{
    std::uint32_t tagSize;

    switch (Mode) {
        case LegacySymmetricProvider::AES_128_GCM:
            tagSize = 16;
            break;
        case LegacySymmetricProvider::AES_256_GCM:
            tagSize = 16;
            break;
        default:
            tagSize = 0;
    }
    return tagSize;
}
}  // namespace opentxs::crypto
