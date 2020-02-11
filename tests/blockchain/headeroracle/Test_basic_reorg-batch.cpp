// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Helpers.hpp"

TEST_F(Test_HeaderOracle, basic_reorg_batch)
{
    EXPECT_TRUE(create_blocks(create_2_));
    EXPECT_TRUE(apply_blocks_batch(sequence_2_));
    EXPECT_TRUE(verify_post_state(post_state_2_));
    EXPECT_TRUE(verify_siblings(siblings_2_));
}
