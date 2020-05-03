// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACTIVITYTHREADITEM_HPP
#define OPENTXS_UI_ACTIVITYTHREADITEM_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include <cstdint>
#include <chrono>
#include <string>

#include "opentxs/SharedPimpl.hpp"
#include "opentxs/Types.hpp"
#include "ListRow.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ActivityThreadItem {
    int Timestamp() const noexcept
    {
        return std::chrono::system_clock::to_time_t($self->Timestamp());
    }
}
%ignore opentxs::ui::ActivityThreadItem::Timestamp;
%template(OTUIActivityThreadItem) opentxs::SharedPimpl<opentxs::ui::ActivityThreadItem>;
%rename(UIActivityThreadItem) opentxs::ui::ActivityThreadItem;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ActivityThreadItem;
}  // namespace ui

using OTUIActivityThreadItem = SharedPimpl<ui::ActivityThreadItem>;
}  // namespace opentxs

namespace opentxs
{
namespace ui
{
class ActivityThreadItem : virtual public ListRow
{
public:
    OPENTXS_EXPORT virtual opentxs::Amount Amount() const noexcept = 0;
    OPENTXS_EXPORT virtual bool Deposit() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string DisplayAmount() const noexcept = 0;
    OPENTXS_EXPORT virtual bool Loading() const noexcept = 0;
    OPENTXS_EXPORT virtual bool MarkRead() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Memo() const noexcept = 0;
    OPENTXS_EXPORT virtual bool Pending() const noexcept = 0;
    OPENTXS_EXPORT virtual std::string Text() const noexcept = 0;
    OPENTXS_EXPORT virtual std::chrono::system_clock::time_point Timestamp()
        const noexcept = 0;
    OPENTXS_EXPORT virtual StorageBox Type() const noexcept = 0;

    OPENTXS_EXPORT ~ActivityThreadItem() override = default;

protected:
    ActivityThreadItem() noexcept = default;

private:
    ActivityThreadItem(const ActivityThreadItem&) = delete;
    ActivityThreadItem(ActivityThreadItem&&) = delete;
    ActivityThreadItem& operator=(const ActivityThreadItem&) = delete;
    ActivityThreadItem& operator=(ActivityThreadItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif
