// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <memory>

#include "Helpers.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/client/Blockchain.hpp"
#include "opentxs/api/client/Manager.hpp"

namespace
{
TEST_F(Test_StartStop, init_opentxs) {}

TEST_F(Test_StartStop, regtest)
{
    EXPECT_TRUE(api_.Blockchain().Start(b::Type::UnitTest));
    EXPECT_TRUE(api_.Blockchain().Stop(b::Type::UnitTest));
}
}  // namespace
