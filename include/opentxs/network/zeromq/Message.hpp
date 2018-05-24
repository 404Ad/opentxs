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

#ifndef OPENTXS_NETWORK_ZEROMQ_MESSAGE_HPP
#define OPENTXS_NETWORK_ZEROMQ_MESSAGE_HPP

#include "opentxs/Forward.hpp"

#ifdef SWIG
// clang-format off
%ignore opentxs::Pimpl<opentxs::network::zeromq::Message>::Pimpl(opentxs::network::zeromq::Message const &);
%ignore opentxs::Pimpl<opentxs::network::zeromq::Message>::operator opentxs::network::zeromq::Message&;
%ignore opentxs::Pimpl<opentxs::network::zeromq::Message>::operator const opentxs::network::zeromq::Message &;
%ignore opentxs::network::zeromq::Message::at(const std::size_t) const;
%ignore opentxs::network::zeromq::Message::begin() const;
%ignore opentxs::network::zeromq::Message::end() const;
%rename(assign) operator=(const opentxs::network::zeromq::Message&);
%rename(ZMQMessage) opentxs::network::zeromq::Message;
%template(OTZMQMessage) opentxs::Pimpl<opentxs::network::zeromq::Message>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Message
{
public:
    EXPORT static Pimpl<Message> Factory();
    EXPORT static Pimpl<Message> Factory(const Data& input);
    EXPORT static Pimpl<Message> Factory(const std::string& input);
    EXPORT static Pimpl<Message> ReplyFactory(const Message& request);

    EXPORT virtual const Frame& at(const std::size_t index) const = 0;
    EXPORT virtual FrameIterator begin() const = 0;
    EXPORT virtual const FrameSection Body() const = 0;
    EXPORT virtual const Frame& Body_at(const std::size_t index) const = 0;
    EXPORT virtual FrameIterator Body_begin() const = 0;
    EXPORT virtual FrameIterator Body_end() const = 0;
    EXPORT virtual FrameIterator end() const = 0;
    EXPORT virtual const FrameSection Header() const = 0;
    EXPORT virtual const Frame& Header_at(const std::size_t index) const = 0;
    EXPORT virtual FrameIterator Header_begin() const = 0;
    EXPORT virtual FrameIterator Header_end() const = 0;
    EXPORT virtual std::size_t size() const = 0;

    EXPORT virtual Frame& AddFrame() = 0;
    EXPORT virtual Frame& AddFrame(const opentxs::Data& input) = 0;
    EXPORT virtual Frame& AddFrame(const std::string& input) = 0;
    EXPORT virtual Frame& at(const std::size_t index) = 0;

    EXPORT virtual ~Message() = default;

protected:
    Message() = default;

private:
    friend OTZMQMessage;

    virtual Message* clone() const = 0;

    Message(const Message&) = delete;
    Message(Message&&) = default;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = default;
};
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif  // OPENTXS_NETWORK_ZEROMQ_MESSAGE_HPP
