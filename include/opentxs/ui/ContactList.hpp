// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_UI_CONTACTLIST_HPP
#define OPENTXS_UI_CONTACTLIST_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/ui/Widget.hpp"

#ifdef SWIG
// clang-format off
%rename(UIContactList) opentxs::ui::ContactList;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace ui
{
class ContactList : virtual public Widget
{
public:
    EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ContactListItem> First()
        const = 0;
    EXPORT virtual opentxs::SharedPimpl<opentxs::ui::ContactListItem> Next()
        const = 0;

    EXPORT virtual ~ContactList() = default;

protected:
    ContactList() = default;

private:
    ContactList(const ContactList&) = delete;
    ContactList(ContactList&&) = delete;
    ContactList& operator=(const ContactList&) = delete;
    ContactList& operator=(ContactList&&) = delete;
};
}  // namespace ui
}  // namespace opentxs
#endif  // OPENTXS_UI_CONTACTLIST_HPP
