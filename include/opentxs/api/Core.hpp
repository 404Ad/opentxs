// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_API_CORE_HPP
#define OPENTXS_API_CORE_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/api/Editor.hpp"
#include "opentxs/api/Periodic.hpp"

#include <chrono>

namespace opentxs
{
namespace api
{
class Core : virtual public Periodic
{
public:
    OPENTXS_EXPORT virtual const crypto::Asymmetric& Asymmetric() const = 0;
    OPENTXS_EXPORT virtual const api::Settings& Config() const = 0;
    OPENTXS_EXPORT virtual const api::Crypto& Crypto() const = 0;
    OPENTXS_EXPORT virtual const std::string& DataFolder() const = 0;
    OPENTXS_EXPORT virtual const api::Endpoints& Endpoints() const = 0;
    OPENTXS_EXPORT virtual const network::Dht& DHT() const = 0;
    OPENTXS_EXPORT virtual const api::Factory& Factory() const = 0;
    OPENTXS_EXPORT virtual int Instance() const = 0;
    OPENTXS_EXPORT virtual const api::HDSeed& Seeds() const = 0;
    OPENTXS_EXPORT virtual void SetMasterKeyTimeout(
        const std::chrono::seconds& timeout) const = 0;
    OPENTXS_EXPORT virtual const storage::Storage& Storage() const = 0;
    OPENTXS_EXPORT virtual const crypto::Symmetric& Symmetric() const = 0;
    OPENTXS_EXPORT virtual const api::Wallet& Wallet() const = 0;
    OPENTXS_EXPORT virtual const opentxs::network::zeromq::Context& ZeroMQ()
        const = 0;

    OPENTXS_EXPORT ~Core() override = default;

protected:
    Core() = default;

private:
    Core(const Core&) = delete;
    Core(Core&&) = delete;
    Core& operator=(const Core&) = delete;
    Core& operator=(Core&&) = delete;
};
}  // namespace api
}  // namespace opentxs
#endif
