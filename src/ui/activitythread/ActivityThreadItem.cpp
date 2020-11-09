// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "0_stdafx.hpp"                              // IWYU pragma: associated
#include "1_Internal.hpp"                            // IWYU pragma: associated
#include "ui/activitythread/ActivityThreadItem.hpp"  // IWYU pragma: associated

#include <tuple>
#include <utility>

#include "internal/api/client/Client.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/api/client/Activity.hpp"
#include "opentxs/core/Flag.hpp"
#include "opentxs/core/Identifier.hpp"
#include "ui/base/Widget.hpp"
#if OT_QT
#include "util/Polarity.hpp"  // IWYU pragma: keep
#endif                        // OT_QT

namespace opentxs::ui::implementation
{
ActivityThreadItem::ActivityThreadItem(
    const ActivityThreadInternalInterface& parent,
    const api::client::internal::Manager& api,
    const identifier::Nym& nymID,
    const ActivityThreadRowID& rowID,
    const ActivityThreadSortKey& sortKey,
    CustomData& custom,
    const bool loading,
    const bool pending) noexcept
    : ActivityThreadItemRow(parent, api, rowID, true)
    , nym_id_(nymID)
    , time_(std::get<0>(sortKey))
    , item_id_(std::get<0>(row_id_))
    , box_(std::get<1>(row_id_))
    , account_id_(std::get<2>(row_id_))
    , text_(extract_custom<std::string>(custom))
    , loading_(Flag::Factory(loading))
    , pending_(Flag::Factory(pending))
{
}

auto ActivityThreadItem::MarkRead() const noexcept -> bool
{
    return api_.Activity().MarkRead(
        nym_id_, Identifier::Factory(parent_.ThreadID()), item_id_);
}

#if OT_QT
QVariant ActivityThreadItem::qt_data(const int column, int role) const noexcept
{
    switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
                case ActivityThreadQt::TextColumn: {
                    return Text().c_str();
                }
                case ActivityThreadQt::AmountColumn: {
                    return DisplayAmount().c_str();
                }
                case ActivityThreadQt::MemoColumn: {
                    return Memo().c_str();
                }
                case ActivityThreadQt::TimeColumn: {
                    QDateTime qdatetime;
                    qdatetime.setSecsSinceEpoch(Clock::to_time_t(Timestamp()));
                    return qdatetime;
                }
                default: {
                    return {};
                }
            }
        }
        case ActivityThreadQt::PolarityRole: {
            return polarity(Amount());
        }
        case ActivityThreadQt::TypeRole: {
            return static_cast<int>(Type());
        }
        case Qt::CheckStateRole: {
            switch (column) {
                case ActivityThreadQt::LoadingColumn: {
                    return Loading();
                }
                case ActivityThreadQt::PendingColumn: {
                    return Pending();
                }
                default: {
                }
            }

            [[fallthrough]];
        }
        default: {
            return {};
        }
    }
}
#endif

void ActivityThreadItem::reindex(
    const ActivityThreadSortKey&,
    CustomData& custom) noexcept
{
    const auto text = extract_custom<std::string>(custom);

    if (false == text.empty()) {
        eLock lock(shared_lock_);
        text_ = text;
    }
}

auto ActivityThreadItem::Text() const noexcept -> std::string
{
    sLock lock(shared_lock_);

    return text_;
}

auto ActivityThreadItem::Timestamp() const noexcept -> Time
{
    sLock lock(shared_lock_);

    return time_;
}
}  // namespace opentxs::ui::implementation
