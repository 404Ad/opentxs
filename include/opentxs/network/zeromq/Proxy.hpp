// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_ZEROMQ_PROXY_HPP
#define OPENTXS_NETWORK_ZEROMQ_PROXY_HPP

#include "opentxs/Forward.hpp"

#ifdef SWIG
// clang-format off
%ignore opentxs::network::zeromq::Proxy::Factory;
%ignore opentxs::Pimpl<opentxs::network::zeromq::Proxy>::Pimpl(opentxs::network::zeromq::Proxy const &);
%ignore opentxs::Pimpl<opentxs::network::zeromq::Proxy>::operator opentxs::network::zeromq::Proxy&;
%ignore opentxs::Pimpl<opentxs::network::zeromq::Proxy>::operator const opentxs::network::zeromq::Proxy &;
%rename(assign) operator=(const opentxs::network::zeromq::Proxy&);
%rename(ZMQProxy) opentxs::network::zeromq::Proxy;
%template(OTZMQProxy) opentxs::Pimpl<opentxs::network::zeromq::Proxy>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
class Proxy
{
public:
    static OTZMQProxy Factory(
        const Context& context,
        Socket& frontend,
        Socket& backend);

    EXPORT virtual ~Proxy() = default;

protected:
    Proxy() = default;

private:
    friend OTZMQProxy;

    virtual Proxy* clone() const = 0;

    Proxy(const Proxy&) = delete;
    Proxy(Proxy&&) = default;
    Proxy& operator=(const Proxy&) = delete;
    Proxy& operator=(Proxy&&) = default;
};
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif
