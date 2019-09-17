// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_LIST_HPP
#define OPENTXS_UI_LIST_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/ui/Widget.hpp"
#include "opentxs/Types.hpp"

#ifdef SWIG
// clang-format off
%rename(UIList) opentxs::ui::List;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class List : virtual public Widget
{
public:
    EXPORT virtual ~List() = default;

protected:
    List() noexcept = default;

private:
    List(const List&) = delete;
    List(List&&) = delete;
    List& operator=(const List&) = delete;
    List& operator=(List&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
