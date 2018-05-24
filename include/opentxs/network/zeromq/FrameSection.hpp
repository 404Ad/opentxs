/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#ifndef OPENTXS_NETWORK_ZEROMQ_FRAMESECTION_HPP
#define OPENTXS_NETWORK_ZEROMQ_FRAMESECTION_HPP

#include "opentxs/Forward.hpp"

#include <atomic>

#ifdef SWIG
// clang-format off
%rename(ZMQFrameSection) opentxs::network::zeromq::FrameSection;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
class FrameSection
{
public:
    using difference_type = std::size_t;
    using value_type = Frame;
    using pointer = Frame*;
    using reference = Frame&;
    using iterator_category = std::forward_iterator_tag;

    FrameSection(const FrameSection&);
    FrameSection(const Message* parent, std::size_t position, std::size_t size);

    EXPORT const Frame& at(const std::size_t index) const;
    EXPORT FrameIterator begin() const;
    EXPORT FrameIterator end() const;
    EXPORT std::size_t size() const;

    EXPORT virtual ~FrameSection() = default;

protected:
    FrameSection() = default;

private:
    const Message* parent_{nullptr};
    std::atomic<std::size_t> position_{0};
    std::atomic<std::size_t> size_{0};

    FrameSection(FrameSection&&) = delete;
    FrameSection& operator=(const FrameSection&) = delete;
    FrameSection& operator=(FrameSection&&) = delete;
};
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif  // OPENTXS_NETWORK_ZEROMQ_FRAMESECTION_HPP
