// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"       // IWYU pragma: associated
#include "1_Internal.hpp"     // IWYU pragma: associated
#include "opentxs/Bytes.hpp"  // IWYU pragma: associated

#include "opentxs/core/Log.hpp"
#include "opentxs/core/LogSource.hpp"

namespace opentxs
{
auto preallocated(const std::size_t size, void* out) noexcept -> AllocateOutput
{
    return [=](const auto in) -> WritableView {
        if (in <= size) {

            return {out, in};
        } else {
            LogOutput("preallocated(): Requested ")(in)(" bytes but only ")(
                size)(" are available")
                .Flush();

            return {nullptr, 0};
        }
    };
}
auto reader(const Space& in) noexcept -> ReadView
{
    return {reinterpret_cast<const char*>(in.data()), in.size()};
}
auto reader(const WritableView& in) noexcept -> ReadView
{
    return {in.as<const char>(), in.size()};
}
auto space(const std::size_t size) noexcept -> Space
{
    auto output = Space{};
    output.assign(size, std::byte{51});

    return output;
}
auto space(const ReadView bytes) noexcept -> Space
{
    if ((nullptr == bytes.data()) || (0 == bytes.size())) { return {}; }

    auto it = reinterpret_cast<const std::byte*>(bytes.data());

    return {it, it + bytes.size()};
}
auto writer(std::string& in) noexcept -> AllocateOutput
{
    return [&in](const auto size) -> WritableView {
        in.resize(size, 51);

        return {in.data(), in.size()};
    };
}
auto writer(Space& in) noexcept -> AllocateOutput
{
    return [&in](const auto size) -> WritableView {
        in.resize(size, std::byte{51});

        return {in.data(), in.size()};
    };
}
}  // namespace opentxs
