// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/crypto/Asymmetric.hpp"
#include "opentxs/api/crypto/Symmetric.hpp"

namespace opentxs::api::crypto::internal
{
struct Asymmetric : virtual public api::crypto::Asymmetric {
    virtual ~Asymmetric() = default;
};
}  // namespace opentxs::api::crypto::internal
