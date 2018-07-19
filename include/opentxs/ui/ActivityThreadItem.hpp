// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_ACTIVITYTHREADITEM_HPP
#define OPENTXS_UI_ACTIVITYTHREADITEM_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/Types.hpp"

#include <cstdint>
#include <chrono>
#include <string>

#include "ListRow.hpp"

#ifdef SWIG
// clang-format off
%extend opentxs::ui::ActivityThreadItem {
    int Timestamp() const
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
class ActivityThreadItem : virtual public ListRow
{
public:
    EXPORT virtual opentxs::Amount Amount() const = 0;
    EXPORT virtual std::string DisplayAmount() const = 0;
    EXPORT virtual bool Loading() const = 0;
    EXPORT virtual bool MarkRead() const = 0;
    EXPORT virtual std::string Memo() const = 0;
    EXPORT virtual bool Pending() const = 0;
    EXPORT virtual std::string Text() const = 0;
    EXPORT virtual std::chrono::system_clock::time_point Timestamp() const = 0;
    EXPORT virtual StorageBox Type() const = 0;

    EXPORT virtual ~ActivityThreadItem() = default;

protected:
    ActivityThreadItem() = default;

private:
    ActivityThreadItem(const ActivityThreadItem&) = delete;
    ActivityThreadItem(ActivityThreadItem&&) = delete;
    ActivityThreadItem& operator=(const ActivityThreadItem&) = delete;
    ActivityThreadItem& operator=(ActivityThreadItem&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif  // OPENTXS_UI_ACTIVITYTHREADITEM_HPP
