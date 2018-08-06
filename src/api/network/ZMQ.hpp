// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::api::network::implementation
{
class ZMQ : virtual public opentxs::api::network::ZMQ
{
public:
    const opentxs::network::zeromq::Context& Context() const override;
    proto::AddressType DefaultAddressType() const override;
    std::chrono::seconds KeepAlive() const override;
    void KeepAlive(const std::chrono::seconds duration) const override;
    std::chrono::seconds Linger() const override;
    OTZMQContext NewContext() const override;
    std::chrono::seconds ReceiveTimeout() const override;
    void RefreshConfig() const override;
    const Flag& Running() const override;
    std::chrono::seconds SendTimeout() const override;

    opentxs::network::ServerConnection& Server(
        const std::string& id) const override;
    bool SetSocksProxy(const std::string& proxy) const override;
    std::string SocksProxy() const override;
    bool SocksProxy(std::string& proxy) const override;
    ConnectionState Status(const std::string& server) const override;

    ~ZMQ();

private:
    friend opentxs::Factory;

    const opentxs::network::zeromq::Context& context_;
    const api::Settings& config_;
    const api::Wallet& wallet_;
    const Flag& running_;
    mutable std::atomic<std::chrono::seconds> linger_;
    mutable std::atomic<std::chrono::seconds> receive_timeout_;
    mutable std::atomic<std::chrono::seconds> send_timeout_;
    mutable std::atomic<std::chrono::seconds> keep_alive_;
    mutable std::mutex lock_;
    mutable std::string socks_proxy_;
    mutable std::map<std::string, OTServerConnection> server_connections_;
    OTZMQPublishSocket status_publisher_;

    bool verify_lock(const Lock& lock) const;

    void init(const Lock& lock) const;

    ZMQ(const opentxs::network::zeromq::Context& context,
        const api::Settings& config,
        const api::Wallet& wallet,
        const Flag& running);
    ZMQ() = delete;
    ZMQ(const ZMQ&) = delete;
    ZMQ(ZMQ&&) = delete;
    ZMQ& operator=(const ZMQ&) = delete;
    ZMQ& operator=(const ZMQ&&) = delete;
};
}  // namespace opentxs::api::network::implementation
