// Copyright (c) 2010-2021 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                               // IWYU pragma: associated
#include "1_Internal.hpp"                             // IWYU pragma: associated
#include "opentxs/network/blockchain/sync/Query.hpp"  // IWYU pragma: associated

#include <memory>

#include "internal/network/blockchain/sync/Factory.hpp"
#include "network/blockchain/sync/Base.hpp"
#include "opentxs/network/blockchain/sync/Block.hpp"
#include "opentxs/network/blockchain/sync/MessageType.hpp"
#include "opentxs/network/blockchain/sync/State.hpp"

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Message;
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs

namespace opentxs::factory
{
using ReturnType = network::blockchain::sync::Query;

auto BlockchainSyncQuery() noexcept -> ReturnType
{
    return {std::make_unique<ReturnType::Imp>().release()};
}

auto BlockchainSyncQuery(int arg) noexcept -> ReturnType
{
    return {std::make_unique<ReturnType::Imp>(arg).release()};
}

auto BlockchainSyncQuery_p(int arg) noexcept -> std::unique_ptr<ReturnType>
{
    return std::make_unique<ReturnType>(
        std::make_unique<ReturnType::Imp>(arg).release());
}
}  // namespace opentxs::factory

namespace opentxs::network::blockchain::sync
{
class Query::Imp final : public Base::Imp
{
public:
    Query* parent_;

    auto asQuery() const noexcept -> const Query& final
    {
        if (nullptr != parent_) {

            return *parent_;
        } else {

            return Base::Imp::asQuery();
        }
    }

    auto serialize(zeromq::Message& out) const noexcept -> bool final
    {
        return serialize_type(out);
    }

    Imp() noexcept
        : Base::Imp()
        , parent_(nullptr)
    {
    }
    Imp(int) noexcept
        : Base::Imp(Base::Imp::default_version_, MessageType::query, {}, {}, {})
        , parent_(nullptr)
    {
    }

private:
    Imp(const Imp&) = delete;
    Imp(Imp&&) = delete;
    auto operator=(const Imp&) -> Imp& = delete;
    auto operator=(Imp&&) -> Imp& = delete;
};

Query::Query(Imp* imp) noexcept
    : Base(imp)
    , imp_(imp)
{
    imp_->parent_ = this;
}

Query::~Query()
{
    if (nullptr != Query::imp_) {
        delete Query::imp_;
        Query::imp_ = nullptr;
        Base::imp_ = nullptr;
    }
}
}  // namespace opentxs::network::blockchain::sync
