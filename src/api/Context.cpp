// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"     // IWYU pragma: associated
#include "1_Internal.hpp"   // IWYU pragma: associated
#include "api/Context.hpp"  // IWYU pragma: associated

#ifndef _WIN32
extern "C" {
#include <sys/resource.h>
}
#endif  // _WIN32

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "2_Factory.hpp"
#include "internal/api/Api.hpp"
#include "internal/api/client/Client.hpp"
#include "internal/api/client/Factory.hpp"
#if OT_RPC
#include "internal/rpc/RPC.hpp"
#endif  // OT_RPC
#include "opentxs/OT.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/Context.hpp"
#include "opentxs/api/Factory.hpp"
#if OT_CRYPTO_WITH_BIP32
#include "opentxs/api/HDSeed.hpp"
#endif  // OT_CRYPTO_WITH_BIP32
#include "opentxs/api/Primitives.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/crypto/OTCallback.hpp"
#include "opentxs/core/crypto/OTCaller.hpp"
#include "opentxs/protobuf/RPCResponse.pb.h"
#include "opentxs/util/PIDFile.hpp"
#include "opentxs/util/Signals.hpp"

// #define OT_METHOD "opentxs::api::implementation::Context::"

namespace opentxs::factory
{
auto Context(
    Flag& running,
    const ArgList& args,
    const std::chrono::seconds gcInterval,
    OTCaller* externalPasswordCallback) noexcept
    -> std::unique_ptr<api::internal::Context>
{
    using ReturnType = api::implementation::Context;

    return std::make_unique<ReturnType>(
        running, args, gcInterval, externalPasswordCallback);
}
}  // namespace opentxs::factory

namespace opentxs::api
{
auto Context::SuggestFolder(const std::string& app) noexcept -> std::string
{
    return Legacy::SuggestFolder(app);
}
}  // namespace opentxs::api

namespace opentxs::api::implementation
{
Context::Context(
    Flag& running,
    const ArgList& args,
    [[maybe_unused]] const std::chrono::seconds gcInterval,
    OTCaller* externalPasswordCallback)
    : api::internal::Context()
    , Lockable()
    , Periodic(running)
    , home_(get_arg(args, OPENTXS_ARG_HOME))
    , config_lock_()
    , task_list_lock_()
    , signal_handler_lock_()
    , config_()
    , zmq_context_(opentxs::Factory::ZMQContext())
    , signal_handler_(nullptr)
    , log_(factory::Log(zmq_context_, get_arg(args, OPENTXS_ARG_LOGENDPOINT)))
    , crypto_(nullptr)
    , factory_(nullptr)
    , legacy_(factory::Legacy(home_))
    , zap_(nullptr)
    , args_(args)
    , shutdown_callback_(nullptr)
    , null_callback_(nullptr)
    , default_external_password_callback_(nullptr)
    , external_password_callback_{externalPasswordCallback}
    , pid_(nullptr)
    , server_()
    , client_()
#if OT_RPC
    , rpc_(opentxs::Factory::RPC(*this))
#endif  // OT_RPC
{
    // NOTE: OT_ASSERT is not available until Init() has been called
    assert(legacy_);
    assert(log_);

    if (nullptr == external_password_callback_) {
        setup_default_external_password_callback();
    }

    assert(nullptr != external_password_callback_);
#if OT_RPC
    assert(rpc_);
#endif  // OT_RPC
}

auto Context::client_instance(const int count) -> int
{
    // NOTE: Instance numbers must not collide between clients and servers.
    // Clients use even numbers and servers use odd numbers.
    return (2 * count);
}

auto Context::Client(const int instance) const
    -> const api::client::internal::Manager&
{
    auto& output = client_.at(instance);

    OT_ASSERT(output);

    return *output;
}

auto Context::Config(const std::string& path) const -> const api::Settings&
{
    std::unique_lock<std::mutex> lock(config_lock_);
    auto& config = config_[path];

    if (!config) {
        config = factory::Settings(*legacy_, String::Factory(path));
    }

    OT_ASSERT(config);

    lock.unlock();

    return *config;
}

auto Context::Crypto() const -> const api::Crypto&
{
    OT_ASSERT(crypto_)

    return *crypto_;
}

auto Context::Factory() const -> const api::Primitives&
{
    OT_ASSERT(factory_)

    return *factory_;
}

auto Context::get_arg(const ArgList& args, const std::string& argName)
    -> std::string
{
    auto argIt = args.find(argName);

    if (args.end() != argIt) {
        const auto& argItems = argIt->second;

        OT_ASSERT(2 > argItems.size());
        OT_ASSERT(0 < argItems.size());

        return *argItems.cbegin();
    }

    return {};
}

auto Context::GetPasswordCaller() const -> OTCaller&
{
    OT_ASSERT(nullptr != external_password_callback_)

    return *external_password_callback_;
}

void Context::HandleSignals(ShutdownCallback* callback) const
{
#ifdef _WIN32
    LogOutput("Signal handling is not supported on Windows").Flush();
#else
    Lock lock(signal_handler_lock_);

    if (nullptr != callback) { shutdown_callback_ = callback; }

    if (false == bool(signal_handler_)) {
        signal_handler_.reset(new Signals(running_));
    }
#endif
}

void Context::Init()
{
    std::int32_t argLevel{-2};

    try {
        argLevel = std::stoi(get_arg(args_, OPENTXS_ARG_LOGLEVEL));
    } catch (...) {
    }

    Init_Log(argLevel);
    Init_Crypto();
    Init_Factory();
#ifndef _WIN32
    Init_Rlimit();
#endif  // _WIN32
    Init_Zap();
}

void Context::Init_Crypto()
{
    crypto_.reset(
        opentxs::Factory::Crypto(Config(legacy_->CryptoConfigFilePath())));

    OT_ASSERT(crypto_);
}

void Context::Init_Factory()
{
    factory_ = factory::Primitives(*crypto_);

    OT_ASSERT(factory_);
}

void Context::Init_Log(const std::int32_t argLevel)
{
    OT_ASSERT(legacy_)

    const auto& config = Config(legacy_->LogConfigFilePath());
    bool notUsed{false};
    std::int64_t level{0};

    if (-1 > argLevel) {
        config.CheckSet_long(
            String::Factory("logging"),
            String::Factory(OPENTXS_ARG_LOGLEVEL),
            0,
            level,
            notUsed);
    } else {
        config.Set_long(
            String::Factory("logging"),
            String::Factory(OPENTXS_ARG_LOGLEVEL),
            argLevel,
            notUsed);
        level = argLevel;
    }
}

void Context::init_pid(const Lock& lock) const
{
    if (false == bool(pid_)) {
        OT_ASSERT(legacy_);

        pid_.reset(opentxs::Factory::PIDFile(legacy_->PIDFilePath()));

        OT_ASSERT(pid_);

        pid_->Open();
    }

    OT_ASSERT(pid_);
    OT_ASSERT(pid_->isOpen());
}

#ifndef _WIN32
void Context::Init_Rlimit() noexcept
{
    auto original = ::rlimit{};
    auto desired = ::rlimit{};
    auto result = ::rlimit{};
    desired.rlim_cur = 32768;
    desired.rlim_max = 32768;

    if (0 != ::getrlimit(RLIMIT_NOFILE, &original)) {
        LogNormal("Failed to query resource limits").Flush();

        return;
    }

    LogVerbose("Current open files limit: ")(original.rlim_cur)(" / ")(
        original.rlim_max)
        .Flush();

    if (0 != ::setrlimit(RLIMIT_NOFILE, &desired)) {
        LogNormal("Failed to set open file limit to 32768. You must increase "
                  "this user account's resource limits via the method "
                  "appropriate for your operating system.")
            .Flush();

        return;
    }

    if (0 != ::getrlimit(RLIMIT_NOFILE, &result)) {
        LogNormal("Failed to query resource limits").Flush();

        return;
    }

    LogVerbose("Adjusted open files limit: ")(result.rlim_cur)(" / ")(
        result.rlim_max)
        .Flush();
}
#endif  // _WIN32

void Context::Init_Zap()
{
    zap_.reset(opentxs::Factory::ZAP(zmq_context_));

    OT_ASSERT(zap_);
}

auto Context::merge_arglist(const ArgList& args) const -> const ArgList
{
    ArgList arguments{args_};

    for (const auto& [arg, val] : args) { arguments[arg] = val; }

    return arguments;
}

auto Context::RPC(const proto::RPCCommand& command) const -> proto::RPCResponse
{
#if OT_RPC  // OT_RPC
    OT_ASSERT(rpc_);

    return rpc_->Process(command);
#else   // OT_RPC
    return {};
#endif  // OT_RPC
}

auto Context::server_instance(const int count) -> int
{
    // NOTE: Instance numbers must not collide between clients and servers.
    // Clients use even numbers and servers use odd numbers.
    return (2 * count) + 1;
}

auto Context::Server(const int instance) const -> const api::server::Manager&
{
    auto& output = server_.at(instance);

    OT_ASSERT(output);

    return *output;
}

void Context::setup_default_external_password_callback()
{
    // NOTE: OT_ASSERT is not available yet because we're too early
    // in the startup process

    null_callback_.reset(opentxs::Factory::NullCallback());

    assert(null_callback_);

    default_external_password_callback_.reset(new OTCaller);

    assert(default_external_password_callback_);

    default_external_password_callback_->SetCallback(null_callback_.get());

    assert(default_external_password_callback_->HaveCallback());

    external_password_callback_ = default_external_password_callback_.get();
}

void Context::shutdown()
{
    running_.Off();

    if (nullptr != shutdown_callback_) {
        ShutdownCallback& callback = *shutdown_callback_;
        callback();
    }

    server_.clear();
    client_.clear();
    crypto_.reset();

    for (auto& config : config_) { config.second.reset(); }

    config_.clear();
}

void Context::start_client(const Lock& lock, const ArgList& args) const
{
    OT_ASSERT(verify_lock(lock))
    OT_ASSERT(crypto_);
    OT_ASSERT(legacy_);

    init_pid(lock);
    auto merged_args = merge_arglist(args);
    const int next = client_.size();
    const auto instance = client_instance(next);
    client_.emplace_back(factory::ClientManager(
        *this,
        running_,
        merged_args,
        Config(legacy_->ClientConfigFilePath(next)),
        *crypto_,
        zmq_context_,
        legacy_->ClientDataFolder(next),
        instance));
}

auto Context::StartClient(const ArgList& args, const int instance) const
    -> const api::client::internal::Manager&
{
    Lock lock(lock_);

    const std::size_t count = std::max(0, instance);
    const std::size_t effective = std::min(count, client_.size());

    if (effective == client_.size()) {
        ArgList arguments{args};
        start_client(lock, arguments);
    }

    const auto& output = client_.at(effective);

    OT_ASSERT(output)

    return *output;
}

#if OT_CRYPTO_WITH_BIP32
auto Context::StartClient(
    const ArgList& args,
    const int instance,
    const std::string& recoverWords,
    const std::string& recoverPassphrase) const
    -> const api::client::internal::Manager&
{
    const auto& client = StartClient(args, instance);
    auto reason = client.Factory().PasswordPrompt("Recovering a BIP-39 seed");

    if (0 < recoverWords.size()) {
        auto wordList =
            opentxs::Context().Factory().SecretFromText(recoverWords);
        auto phrase =
            opentxs::Context().Factory().SecretFromText(recoverPassphrase);
        client.Seeds().ImportSeed(wordList, phrase, reason);
    }

    return client;
}
#endif  // OT_CRYPTO_WITH_BIP32

void Context::start_server(const Lock& lock, const ArgList& args) const
{
    OT_ASSERT(verify_lock(lock))
    OT_ASSERT(crypto_);

    init_pid(lock);
    const auto merged_args = merge_arglist(args);
    const auto next{server_.size()};
    const auto instance{server_instance(next)};
    server_.emplace_back(opentxs::Factory::ServerManager(
        *this,
        running_,
        merged_args,
        *crypto_,
        Config(legacy_->ServerConfigFilePath(next)),
        zmq_context_,
        legacy_->ServerDataFolder(next),
        instance));
}

auto Context::StartServer(
    const ArgList& args,
    const int instance,
    const bool inproc) const -> const api::server::Manager&
{
    Lock lock(lock_);

    const std::size_t count = std::max(0, instance);
    const std::size_t effective = std::min(count, server_.size());

    if (effective == server_.size()) {
        ArgList arguments{args};

        if (inproc) {
            auto& inprocSet = arguments[OPENTXS_ARG_INPROC];
            inprocSet.clear();
            inprocSet.insert(std::to_string(server_instance(effective)));
        }

        start_server(lock, arguments);
    }

    const auto& output = server_.at(effective);

    OT_ASSERT(output)

    return *output;
}

auto Context::ZAP() const -> const api::network::ZAP&
{
    OT_ASSERT(zap_);

    return *zap_;
}

Context::~Context()
{
    client_.clear();
    server_.clear();

    if (pid_) { pid_->Close(); }

    LogSource::Shutdown();
    log_.reset();
}
}  // namespace opentxs::api::implementation
