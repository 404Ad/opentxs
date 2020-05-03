// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_ZEROMQ_MESSAGE_HPP
#define OPENTXS_NETWORK_ZEROMQ_MESSAGE_HPP

#include "opentxs/Forward.hpp"  // IWYU pragma: associated

#include "opentxs/network/zeromq/Frame.hpp"
#include "opentxs/Pimpl.hpp"
#include "opentxs/Proto.hpp"

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
class Message;
}  // namespace zeromq
}  // namespace network

using OTZMQMessage = Pimpl<network::zeromq::Message>;
}  // namespace opentxs

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Message
{
public:
    OPENTXS_EXPORT static Pimpl<Message> Factory();

    OPENTXS_EXPORT virtual const Frame& at(const std::size_t index) const = 0;
    OPENTXS_EXPORT virtual FrameIterator begin() const = 0;
    OPENTXS_EXPORT virtual const FrameSection Body() const = 0;
    OPENTXS_EXPORT virtual const Frame& Body_at(
        const std::size_t index) const = 0;
    OPENTXS_EXPORT virtual FrameIterator Body_begin() const = 0;
    OPENTXS_EXPORT virtual FrameIterator Body_end() const = 0;
    OPENTXS_EXPORT virtual FrameIterator end() const = 0;
    OPENTXS_EXPORT virtual const FrameSection Header() const = 0;
    OPENTXS_EXPORT virtual const Frame& Header_at(
        const std::size_t index) const = 0;
    OPENTXS_EXPORT virtual FrameIterator Header_begin() const = 0;
    OPENTXS_EXPORT virtual FrameIterator Header_end() const = 0;
    OPENTXS_EXPORT virtual std::size_t size() const = 0;

    OPENTXS_EXPORT virtual Frame& AddFrame() = 0;
    OPENTXS_EXPORT virtual Frame& AddFrame(const ProtobufType& input) = 0;
#ifndef SWIG
    template <
        typename Input,
        std::enable_if_t<
            std::is_pointer<decltype(std::declval<Input&>().data())>::value,
            int> = 0,
        std::enable_if_t<
            std::is_integral<decltype(std::declval<Input&>().size())>::value,
            int> = 0>
    OPENTXS_EXPORT auto AddFrame(const Input& input) -> Frame&
    {
        return AddFrame(input.data(), input.size());
    }
    template <
        typename Input,
        std::enable_if_t<std::is_trivially_copyable<Input>::value, int> = 0>
    OPENTXS_EXPORT auto AddFrame(const Input& input) -> Frame&
    {
        return AddFrame(&input, sizeof(input));
    }
    template <typename Input>
    OPENTXS_EXPORT auto AddFrame(const Pimpl<Input>& input) -> Frame&
    {
        return AddFrame(input.get());
    }
    OPENTXS_EXPORT virtual auto AddFrame(
        const void* input,
        const std::size_t size) -> Frame& = 0;
#endif
    OPENTXS_EXPORT virtual Frame& at(const std::size_t index) = 0;
    OPENTXS_EXPORT virtual FrameSection Body() = 0;
    OPENTXS_EXPORT virtual void EnsureDelimiter() = 0;
    OPENTXS_EXPORT virtual FrameSection Header() = 0;
    OPENTXS_EXPORT virtual void PrependEmptyFrame() = 0;
    OPENTXS_EXPORT virtual Frame& Replace(
        const std::size_t index,
        OTZMQFrame&& frame) = 0;

    OPENTXS_EXPORT virtual ~Message() = default;

protected:
    Message() = default;

private:
    friend OTZMQMessage;

#ifdef _WIN32
public:
#endif
    OPENTXS_EXPORT virtual Message* clone() const = 0;
#ifdef _WIN32
private:
#endif

    Message(const Message&) = delete;
    Message(Message&&) = default;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = default;
};
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif
