// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                   // IWYU pragma: associated
#include "1_Internal.hpp"                 // IWYU pragma: associated
#include "blockchain/client/Network.hpp"  // IWYU pragma: associated

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include "internal/blockchain/Params.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Endpoints.hpp"
#include "opentxs/api/Factory.hpp"
#include "opentxs/api/client/Manager.hpp"
#include "opentxs/blockchain/block/Header.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/identifier/Nym.hpp"
#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/network/zeromq/FrameSection.hpp"
#include "opentxs/network/zeromq/Message.hpp"
#include "opentxs/network/zeromq/Pipeline.hpp"
#include "opentxs/protobuf/BlockchainTransactionProposal.pb.h"
#include "opentxs/protobuf/BlockchainTransactionProposedOutput.pb.h"
#include "util/ScopeGuard.hpp"

#define OT_METHOD "opentxs::blockchain::client::implementation::Network::"

namespace opentxs::blockchain::client::implementation
{
constexpr auto proposal_version_ = VersionNumber{1};
constexpr auto output_version_ = VersionNumber{1};

Network::Network(
    const api::client::Manager& api,
    const api::client::internal::Blockchain& blockchain,
    const Type type,
    const std::string& seednode,
    const std::string& shutdown) noexcept
    : Worker(api, std::chrono::seconds(0))
    , shutdown_sender_(api.ZeroMQ(), shutdown_endpoint())
    , database_p_(factory::BlockchainDatabase(
          api,
          blockchain,
          *this,
          blockchain.BlockchainDB(),
          type))
    , header_p_(factory::HeaderOracle(api, *this, *database_p_, type))
    , peer_p_(factory::BlockchainPeerManager(
          api,
          *this,
          *database_p_,
          blockchain.IO(),
          type,
          seednode,
          shutdown_sender_.endpoint_))
    , block_p_(factory::BlockOracle(
          api,
          *this,
          *database_p_,
          type,
          shutdown_sender_.endpoint_))
    , filter_p_(factory::BlockchainFilterOracle(
          api,
          *this,
          *header_p_,
          *database_p_,
          type,
          shutdown_sender_.endpoint_))
    , wallet_p_(factory::BlockchainWallet(
          api,
          blockchain,
          *this,
          type,
          shutdown_sender_.endpoint_))
    , blockchain_(blockchain)
    , chain_(type)
    , database_(*database_p_)
    , filters_(*filter_p_)
    , header_(*header_p_)
    , peer_(*peer_p_)
    , block_(*block_p_)
    , wallet_(*wallet_p_)
    , parent_(blockchain)
    , local_chain_height_(0)
    , remote_chain_height_(0)
    , waiting_for_headers_(Flag::Factory(false))
    , processing_headers_(Flag::Factory(false))
    , headers_requested_(Clock::now())
    , wallet_initialized_(false)
    , init_promise_()
    , init_(init_promise_.get_future())
{
    OT_ASSERT(database_p_);
    OT_ASSERT(filter_p_);
    OT_ASSERT(header_p_);
    OT_ASSERT(peer_p_);
    OT_ASSERT(block_p_);
    OT_ASSERT(wallet_p_);

    database_.SetDefaultFilterType(filters_.DefaultType());
    header_.Init();
    init_executor({api_.Endpoints().InternalBlockchainFilterUpdated(chain_)});
}

auto Network::AddPeer(const p2p::Address& address) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.AddPeer(address);
}

auto Network::BroadcastTransaction(
    const block::bitcoin::Transaction& tx) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.BroadcastTransaction(tx);
}

auto Network::Connect() noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.Connect();
}

auto Network::Disconnect() noexcept -> bool
{
    // TODO

    return false;
}

auto Network::FeeRate() const noexcept -> Amount
{
    try {

        return params::Data::chains_.at(chain_).default_fee_rate_;
    } catch (...) {

        return 0;
    }
}

auto Network::GetConfirmations(const std::string& txid) const noexcept
    -> ChainHeight
{
    // TODO

    return -1;
}

auto Network::GetPeerCount() const noexcept -> std::size_t
{
    if (false == running_.get()) { return false; }

    return peer_.GetPeerCount();
}

auto Network::init() noexcept -> void
{
    local_chain_height_.store(header_.BestChain().first);

    {
        const auto best = database_.CurrentBest();

        OT_ASSERT(best);

        const auto position = best->Position();
        LogNormal(blockchain::internal::DisplayString(chain_))(
            " chain initialized with best hash ")(position.second->asHex())(
            " at height ")(position.first)
            .Flush();
    }

    peer_.init();
    block_.Init();
    filters_.Start();
    init_promise_.set_value();
    trigger();
}

auto Network::pipeline(zmq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    OT_ASSERT(0 < body.size());

    switch (body.at(0).as<Task>()) {
        case Task::SubmitFilterHeader: {
            process_cfheader(in);
        } break;
        case Task::SubmitFilter: {
            process_filter(in);
        } break;
        case Task::SubmitBlock: {
            process_block(in);
        } break;
        case Task::SubmitBlockHeader: {
            process_header(in);
            [[fallthrough]];
        }
        case Task::StateMachine: {
            do_work();
        } break;
        case Task::FilterUpdate: {
            process_filter_update(in);
        } break;
        case Task::Heartbeat: {
            block_.Heartbeat();
            filters_.Heartbeat();
            peer_.Heartbeat();
            do_work();
        } break;
        case Task::Shutdown: {
            shutdown(shutdown_promise_);
        } break;
        default: {
            OT_FAIL;
        }
    }
}

auto Network::process_block(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    const auto body = in.Body();

    if (2 > body.size()) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid block").Flush();

        return;
    }

    block_.SubmitBlock(body.at(1));
}

auto Network::process_cfheader(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    filters_.AddHeaders(in);
}

auto Network::process_filter(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    filters_.AddFilter(in);
}

auto Network::process_filter_update(network::zeromq::Message& in) noexcept
    -> void
{
    if (false == running_.get()) { return; }

    const auto& body = in.Body();

    OT_ASSERT(2 < body.size());

    const auto height = body.at(2).as<block::Height>();
    const auto target =
        std::max(local_chain_height_.load(), remote_chain_height_.load());

    {
        const auto progress = (double(height) / double(target)) * double{100};
        auto display = std::stringstream{};
        display << std::setprecision(3) << progress << "%";
        LogNormal(blockchain::internal::DisplayString(chain_))(
            " chain sync progress: ")(display.str())
            .Flush();
    }

    blockchain_.ReportProgress(chain_, height, target);
}

auto Network::process_header(network::zeromq::Message& in) noexcept -> void
{
    if (false == running_.get()) { return; }

    processing_headers_->On();
    waiting_for_headers_->Off();
    auto postcondition = ScopeGuard{[&] { processing_headers_->Off(); }};
    using Promise = std::promise<void>;
    auto pPromise = std::unique_ptr<Promise>{};
    auto input = std::vector<ReadView>{};

    {
        auto counter{-1};
        const auto body = in.Body();

        if (2 > body.size()) {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Invalid message").Flush();

            return;
        }

        for (const auto& frame : body) {
            switch (++counter) {
                case 0: {
                    break;
                }
                case 1: {
                    pPromise.reset(
                        reinterpret_cast<Promise*>(frame.as<std::uintptr_t>()));
                } break;
                default: {
                    input.emplace_back(frame.Bytes());
                }
            }
        }
    }

    OT_ASSERT(pPromise);

    auto& promise = *pPromise;
    auto headers = std::vector<std::unique_ptr<block::Header>>{};

    for (const auto& header : input) {
        headers.emplace_back(instantiate_header(header));
    }

    if (false == headers.empty()) { header_.AddHeaders(headers); }

    processing_headers_->Off();
    promise.set_value();
}

auto Network::RequestBlock(const block::Hash& block) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestBlock(block);
}

auto Network::RequestFilterHeaders(
    const filter::Type type,
    const block::Height start,
    const block::Hash& stop) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestFilterHeaders(type, start, stop);
}

auto Network::RequestFilters(
    const filter::Type type,
    const block::Height start,
    const block::Hash& stop) const noexcept -> bool
{
    if (false == running_.get()) { return false; }

    return peer_.RequestFilters(type, start, stop);
}

auto Network::SendToAddress(
    const opentxs::identifier::Nym& sender,
    const std::string& address,
    const Amount amount,
    const std::string& memo) const noexcept -> PendingOutgoing
{
    const auto [data, style, chains] = blockchain_.DecodeAddress(address);

    if (0 == chains.count(chain_)) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Address ")(address)(
            " not valid for ")(blockchain::internal::DisplayString(chain_))
            .Flush();

        return {};
    }

    auto id = api_.Factory().Identifier();
    id->Randomize(32);
    auto proposal = proto::BlockchainTransactionProposal{};
    proposal.set_version(proposal_version_);
    proposal.set_id(id->str());
    proposal.set_initiator(sender.str());
    proposal.set_expires(
        Clock::to_time_t(Clock::now() + std::chrono::hours(1)));
    proposal.set_memo(memo);
    using Style = api::client::blockchain::AddressStyle;
    auto& output = *proposal.add_output();
    output.set_version(output_version_);
    output.set_amount(amount);

    switch (style) {
        case Style::P2PKH: {
            output.set_pubkeyhash(data->str());
        } break;
        case Style::P2SH: {
            output.set_scripthash(data->str());
        } break;
        default: {
            LogOutput(OT_METHOD)(__FUNCTION__)(": Unsupported address type")
                .Flush();

            return {};
        }
    }

    return wallet_.ConstructTransaction(proposal);
}

auto Network::shutdown(std::promise<void>& promise) noexcept -> void
{
    init_.get();

    if (running_->Off()) {
        shutdown_sender_.Activate();

        if (false == wallet_initialized_) { wallet_.Init(); }

        wallet_.Shutdown().get();
        wallet_initialized_ = false;
        block_.Shutdown().get();
        peer_.Shutdown().get();
        filters_.Shutdown().get();
        shutdown_sender_.Close();

        try {
            promise.set_value();
        } catch (...) {
        }
    }
}

auto Network::shutdown_endpoint() noexcept -> std::string
{
    return std::string{"inproc://"} + Identifier::Random()->str();
}

auto Network::state_machine() noexcept -> bool
{
    LogTrace(OT_METHOD)(__FUNCTION__).Flush();

    if (false == running_.get()) { return false; }

    if (processing_headers_.get()) { return false; }

    if (IsSynchronized() && (0 < GetHeight())) {
        if (false == wallet_initialized_) {
            wallet_.Init();
            wallet_initialized_ = true;
        }

        return false;
    }

    if (waiting_for_headers_.get()) {
        const auto timeout =
            (Clock::now() - headers_requested_) > std::chrono::seconds{10};

        if (false == timeout) { return false; }
    }

    waiting_for_headers_->On();
    headers_requested_ = Clock::now();
    peer_.RequestHeaders();

    return false;
}

auto Network::Submit(network::zeromq::Message& work) const noexcept -> void
{
    if (false == running_.get()) { return; }

    pipeline_->Push(work);
}

auto Network::UpdateHeight(const block::Height height) const noexcept -> void
{
    if (false == running_.get()) { return; }

    remote_chain_height_.store(std::max(height, remote_chain_height_.load()));
    trigger();
}

auto Network::UpdateLocalHeight(const block::Position position) const noexcept
    -> void
{
    if (false == running_.get()) { return; }

    const auto& [height, hash] = position;
    LogNormal(blockchain::internal::DisplayString(chain_))(
        " block header chain updated to hash ")(hash->asHex())(" at height ")(
        height)
        .Flush();
    local_chain_height_.store(height);
    trigger();
}

Network::~Network() { Shutdown().get(); }
}  // namespace opentxs::blockchain::client::implementation
