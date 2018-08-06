// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::api::server::implementation
{
class Wallet final : public api::implementation::Wallet
{
public:
    std::shared_ptr<const opentxs::ClientContext> ClientContext(
        const Identifier& localNymID,
        const Identifier& remoteNymID) const override;
    std::shared_ptr<const opentxs::Context> Context(
        const Identifier& notaryID,
        const Identifier& clientNymID) const override;
    Editor<opentxs::ClientContext> mutable_ClientContext(
        const Identifier& localNymID,
        const Identifier& remoteNymID) const override;
    Editor<opentxs::Context> mutable_Context(
        const Identifier& notaryID,
        const Identifier& clientNymID) const override;

    ~Wallet() = default;

private:
    friend opentxs::Factory;

    using ot_super = api::implementation::Wallet;

    const api::server::Manager& server_;

    void instantiate_client_context(
        const proto::Context& serialized,
        const std::shared_ptr<const opentxs::Nym>& localNym,
        const std::shared_ptr<const opentxs::Nym>& remoteNym,
        std::shared_ptr<opentxs::Context>& output) const override;
    bool load_legacy_account(
        const std::string& dataFolder,
        const Identifier& accountID,
        const eLock& lock,
        AccountLock& row) const override;
    std::shared_ptr<const opentxs::Nym> signer_nym(
        const Identifier& id) const override;

    Wallet(
        const api::server::Manager& server,
        const api::storage::Storage& storage,
        const api::Factory& factory,
        const api::HDSeed& seeds,
        const api::Legacy& legacy,
        const opentxs::network::zeromq::Context& zmq);
    Wallet() = delete;
    Wallet(const Wallet&) = delete;
    Wallet(Wallet&&) = delete;
    Wallet& operator=(const Wallet&) = delete;
    Wallet& operator=(Wallet&&) = delete;
};
}  // namespace opentxs::api::server::implementation
