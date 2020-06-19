// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: private
// IWYU pragma: friend ".*src/blockchain/p2p/bitcoin/message/Reject.cpp"

#pragma once

#include <memory>
#include <set>
#include <string>

#include "blockchain/p2p/bitcoin/Message.hpp"
#include "internal/blockchain/p2p/bitcoin/Bitcoin.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"

namespace opentxs
{
namespace api
{
namespace client
{
class Manager;
}  // namespace client
}  // namespace api

namespace blockchain
{
namespace p2p
{
namespace bitcoin
{
class Header;
}  // namespace bitcoin
}  // namespace p2p
}  // namespace blockchain
}  // namespace opentxs

namespace opentxs::blockchain::p2p::bitcoin::message
{
class Reject final : virtual public bitcoin::Message
{
public:
    auto getMessage() const noexcept -> const std::string& { return message_; }
    auto getRejectCode() const noexcept -> bitcoin::RejectCode { return code_; }
    auto getReason() const noexcept -> const std::string& { return reason_; }
    auto getExtraData() const noexcept -> OTData
    {
        return Data::Factory(extra_);
    }

    Reject(
        const api::client::Manager& api,
        const blockchain::Type network,
        const std::string& message,
        const bitcoin::RejectCode code,
        const std::string& reason,
        const Data& extra) noexcept;
    Reject(
        const api::client::Manager& api,
        std::unique_ptr<Header> header,
        const std::string& message,
        const bitcoin::RejectCode code,
        const std::string& reason,
        const Data& extra) noexcept(false);

    ~Reject() final = default;

private:
    const std::string message_;
    const bitcoin::RejectCode code_{};
    const std::string reason_;
    const OTData extra_;

    auto payload() const noexcept -> OTData final;

    Reject(const Reject&) = delete;
    Reject(Reject&&) = delete;
    auto operator=(const Reject&) -> Reject& = delete;
    auto operator=(Reject &&) -> Reject& = delete;
};
}  // namespace opentxs::blockchain::p2p::bitcoin::message
