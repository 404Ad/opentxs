// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "OTTestEnvironment.hpp"

#include "opentxs/opentxs.hpp"

// Insure FAIL macro has correct definition
#include <gtest/gtest.h>

void OTTestEnvironment::SetUp()
{
    opentxs::ArgList args;
    opentxs::OT::ClientFactory(args);
}

void OTTestEnvironment::TearDown() { opentxs::OT::Cleanup(); }

OTTestEnvironment::~OTTestEnvironment()
{
    // TODO Auto-generated destructor stub
}
