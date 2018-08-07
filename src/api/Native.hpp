// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace
{
extern "C" {
INTERNAL_PASSWORD_CALLBACK default_pass_cb;
INTERNAL_PASSWORD_CALLBACK souped_up_pass_cb;
}
}  // namespace

namespace opentxs::api::implementation
{
/** \brief Singlton class for providing an interface to process-level resources.
 *  \ingroup native
 */
class Native final : api::internal::Native
{
public:
    const api::client::Manager& Client() const override;
    const api::Settings& Config(const std::string& path) const override;
    const api::Crypto& Crypto() const override;
    void HandleSignals(ShutdownCallback* shutdown) const override;
    const api::Legacy& Legacy() const override;
    const api::server::Manager& Server() const override;
    bool ServerMode() const override;

    INTERNAL_PASSWORD_CALLBACK* GetInternalPasswordCallback() const override;
    OTCaller& GetPasswordCaller() const override;

private:
    friend opentxs::Factory;
    friend class opentxs::OT;

    typedef std::map<std::string, std::unique_ptr<api::Settings>> ConfigMap;

    Flag& running_;
    const bool recover_{false};
    const bool server_mode_{false};
    const std::chrono::seconds gc_interval_{0};
    OTPassword word_list_{};
    OTPassword passphrase_{};
    mutable std::mutex config_lock_;
    mutable std::mutex task_list_lock_;
    mutable std::mutex signal_handler_lock_;
    std::unique_ptr<api::client::internal::Manager> client_;
    mutable ConfigMap config_;
    std::unique_ptr<api::Crypto> crypto_;
    std::unique_ptr<api::Legacy> legacy_;
    std::unique_ptr<api::server::Manager> server_;
    OTZMQContext zmq_context_;
    mutable std::unique_ptr<Signals> signal_handler_;
    const ArgList server_args_;
    mutable ShutdownCallback* shutdown_callback_{nullptr};
    std::unique_ptr<OTCallback> null_callback_{nullptr};
    std::unique_ptr<OTCaller> default_external_password_callback_{nullptr};
    OTCaller* external_password_callback_{nullptr};

    explicit Native(
        Flag& running,
        const ArgList& args,
        const bool recover,
        const bool serverMode,
        const std::chrono::seconds gcInterval,
        OTCaller* externalPasswordCallback = nullptr);
    Native() = delete;
    Native(const Native&) = delete;
    Native(Native&&) = delete;
    Native& operator=(const Native&) = delete;
    Native& operator=(Native&&) = delete;

    void setup_default_external_password_callback();

    void Init_Api();
    void Init_Crypto();
    void Init_Log();
    void Init_Server();
    void Init() override;
    void recover();
    void shutdown() override;

    ~Native() = default;
};
}  // namespace opentxs::api::implementation
