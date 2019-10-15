// Copyright (c) 2010-2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_ZEROMQ_SOCKET_PUBLISH_HPP
#define OPENTXS_NETWORK_ZEROMQ_SOCKET_PUBLISH_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/network/zeromq/curve/Server.hpp"
#include "opentxs/network/zeromq/socket/Sender.hpp"

#ifdef SWIG
// clang-format off
%ignore opentxs::Pimpl<opentxs::network::zeromq::socket::Publish>::Pimpl(opentxs::network::zeromq::socket::Publish const &);
%ignore opentxs::Pimpl<opentxs::network::zeromq::socket::Publish>::operator opentxs::network::zeromq::socket::Publish&;
%ignore opentxs::Pimpl<opentxs::network::zeromq::socket::Publish>::operator const opentxs::network::zeromq::socket::Publish &;
%rename(assign) operator=(const opentxs::network::zeromq::socket::Publish&);
%rename(ZMQPublish) opentxs::network::zeromq::socket::Publish;
%template(OTZMQPublishSocket) opentxs::Pimpl<opentxs::network::zeromq::socket::Publish>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
namespace socket
{
class Publish : virtual public curve::Server, virtual public Sender
{
public:
    EXPORT ~Publish() override = default;

protected:
    Publish() noexcept = default;

private:
    friend OTZMQPublishSocket;

    virtual Publish* clone() const noexcept = 0;

    Publish(const Publish&) = delete;
    Publish(Publish&&) = delete;
    Publish& operator=(const Publish&) = delete;
    Publish& operator=(Publish&&) = delete;
};
}  // namespace socket
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif
