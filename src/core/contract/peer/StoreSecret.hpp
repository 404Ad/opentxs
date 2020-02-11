// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::contract::peer::request::implementation
{
class StoreSecret final : public request::StoreSecret,
                          public peer::implementation::Request
{
public:
    StoreSecret(
        const api::internal::Core& api,
        const Nym_p& nym,
        const identifier::Nym& recipientID,
        const proto::SecretType type,
        const std::string& primary,
        const std::string& secondary,
        const identifier::Server& serverID);
    StoreSecret(
        const api::internal::Core& api,
        const Nym_p& nym,
        const proto::PeerRequest& serialized);

    ~StoreSecret() final = default;

private:
    friend opentxs::Factory;

    const proto::SecretType secret_type_;
    const std::string primary_;
    const std::string secondary_;

    StoreSecret* clone() const noexcept final { return new StoreSecret(*this); }
    SerializedType IDVersion(const Lock& lock) const final;

    StoreSecret() = delete;
    StoreSecret(const StoreSecret&);
    StoreSecret(StoreSecret&&) = delete;
    StoreSecret& operator=(const StoreSecret&) = delete;
    StoreSecret& operator=(StoreSecret&&) = delete;
};
}  // namespace opentxs::contract::peer::request::implementation
