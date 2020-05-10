// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/api/crypto/Encode.cpp"

#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

#include "opentxs/Types.hpp"
#include "opentxs/api/crypto/Encode.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/String.hpp"

namespace opentxs
{
namespace api
{
class Crypto;
}  // namespace api

class Factory;
class OTPassword;
}  // namespace opentxs

namespace opentxs::api::crypto::implementation
{
class Encode final : virtual public api::crypto::Encode
{
public:
    auto DataEncode(const std::string& input) const -> std::string final;
    auto DataEncode(const Data& input) const -> std::string final;
    auto DataDecode(const std::string& input) const -> std::string final;
    auto IdentifierEncode(const Data& input) const -> std::string final;
    auto IdentifierDecode(const std::string& input) const -> std::string final;
    auto IsBase62(const std::string& str) const -> bool final;
    auto Nonce(const std::uint32_t size) const -> OTString final;
    auto Nonce(const std::uint32_t size, Data& rawOutput) const
        -> OTString final;
    auto RandomFilename() const -> std::string final;
    auto SanatizeBase58(const std::string& input) const -> std::string final;
    auto SanatizeBase64(const std::string& input) const -> std::string final;
    auto Z85Encode(const Data& input) const -> std::string final;
    auto Z85Encode(const std::string& input) const -> std::string final;
    auto Z85Decode(const Data& input) const -> OTData final;
    auto Z85Decode(const std::string& input) const -> std::string final;

    ~Encode() final = default;

private:
    friend opentxs::Factory;

    static const std::uint8_t LineWidth{72};

    const api::Crypto& crypto_;

    auto Base64Encode(
        const std::uint8_t* inputStart,
        const std::size_t& inputSize) const -> std::string;
    auto Base64Decode(const std::string&& input, RawData& output) const -> bool;
    auto BreakLines(const std::string& input) const -> std::string;
    auto IdentifierEncode(const OTPassword& input) const -> std::string;

    Encode(const api::Crypto& crypto);
    Encode() = delete;
    Encode(const Encode&) = delete;
    auto operator=(const Encode&) -> Encode& = delete;
};
}  // namespace opentxs::api::crypto::implementation
