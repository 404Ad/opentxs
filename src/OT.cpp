// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"    // IWYU pragma: associated
#include "1_Internal.hpp"  // IWYU pragma: associated
#include "opentxs/OT.hpp"  // IWYU pragma: associated

#include <memory>
#include <stdexcept>

#include "internal/api/Api.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Log.hpp"

namespace opentxs
{
namespace api
{
class Context;
}  // namespace api

class OTCaller;

api::internal::Context* instance_pointer_{nullptr};
OTFlag running_{Flag::Factory(true)};

auto Context() -> const api::Context&
{
    if (nullptr == instance_pointer_) {
        std::runtime_error("Context is not initialized");
    }

    return *instance_pointer_;
}

void Cleanup()
{
    if (nullptr != instance_pointer_) {
        instance_pointer_->shutdown();
        delete instance_pointer_;
        instance_pointer_ = nullptr;
    }
}

auto InitContext(
    const ArgList& args,
    const std::chrono::seconds gcInterval,
    OTCaller* externalPasswordCallback) -> const api::Context&
{
    if (nullptr != instance_pointer_) {
        std::runtime_error("Context is not initialized");
    }

    instance_pointer_ =
        factory::Context(running_, args, gcInterval, externalPasswordCallback)
            .release();

    OT_ASSERT(nullptr != instance_pointer_);

    instance_pointer_->Init();

    return *instance_pointer_;
}

void Join()
{
    while (nullptr != instance_pointer_) {
        Sleep(std::chrono::milliseconds(250));
    }
}
}  // namespace opentxs
