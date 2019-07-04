// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_PROTO_TPP
#define OPENTXS_PROTO_TPP

#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/Armored.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/Proto.hpp"

#include <iostream>

namespace opentxs
{
namespace proto
{
std::string ToString(const ProtobufType& input);

template <typename Output>
Output Factory(const void* input, const std::size_t size);
template <typename Output, typename Input>
Output Factory(const Pimpl<Input>& input);
template <typename Output, typename Input>
Output Factory(const Input& input);

template <typename Output>
Output Factory(const void* input, const std::size_t size)
{
    Output serialized;
    serialized.ParseFromArray(input, size);

    return serialized;
}

template <typename Output, typename Input>
Output Factory(const Pimpl<Input>& input)
{
    return Factory<Output>(input.get());
}

template <typename Output, typename Input>
Output Factory(const Input& input)
{
    return Factory<Output>(input.data(), input.size());
}

template <typename Output>
std::unique_ptr<Output> DynamicFactory(
    const void* input,
    const std::size_t size)
{
    auto output = std::make_unique<Output>();

    if (output) { output->ParseFromArray(input, size); }

    return output;
}

template <typename Output, typename Input>
std::unique_ptr<Output> DynamicFactory(const Pimpl<Input>& input)
{
    return DynamicFactory<Output>(input.get());
}

template <typename Output, typename Input>
std::unique_ptr<Output> DynamicFactory(const Input& input)
{
    return DynamicFactory<Output>(input.data(), input.size());
}

template <typename Output>
Output StringToProto(const String& input)
{
    auto armored = Armored::Factory();
    OTString unconstInput = String::Factory(input.Get());

    if (!armored->LoadFromString(unconstInput)) {
        std::cerr << __FUNCTION__ << "Failed to decode armored protobuf."
                  << std::endl;

        return Output();
    } else {

        return Factory<Output>(Data::Factory(armored));
    }
}
}  // namespace proto
}  // namespace opentxs
#endif
