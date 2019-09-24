// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_ZEROMQ_LISTENCALLBACK_HPP
#define OPENTXS_NETWORK_ZEROMQ_LISTENCALLBACK_HPP

#include "opentxs/Forward.hpp"

#include <functional>

#ifdef SWIG
// clang-format off
%ignore opentxs::Pimpl<opentxs::network::zeromq::ListenCallback>::Pimpl(opentxs::network::zeromq::ListenCallback const &);
%ignore opentxs::Pimpl<opentxs::network::zeromq::ListenCallback>::operator opentxs::network::zeromq::ListenCallback&;
%ignore opentxs::Pimpl<opentxs::network::zeromq::ListenCallback>::operator const opentxs::network::zeromq::ListenCallback &;
%rename(assign) operator=(const opentxs::network::zeromq::ListenCallback&);
%rename(ZMQListenCallback) opentxs::network::zeromq::ListenCallback;
%template(OTZMQListenCallback) opentxs::Pimpl<opentxs::network::zeromq::ListenCallback>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
class ListenCallback
{
public:
    using ReceiveCallback = std::function<void(Message&)>;

#ifndef SWIG
    EXPORT static OTZMQListenCallback Factory(ReceiveCallback callback);
    EXPORT static OTZMQListenCallback Factory();
#endif
    EXPORT static opentxs::Pimpl<opentxs::network::zeromq::ListenCallback>
    Factory(ListenCallbackSwig* callback);

    EXPORT virtual void Process(Message& message) const = 0;

    EXPORT virtual ~ListenCallback() = default;

protected:
    ListenCallback() = default;

private:
    friend OTZMQListenCallback;

    virtual ListenCallback* clone() const = 0;

    ListenCallback(const ListenCallback&) = delete;
    ListenCallback(ListenCallback&&) = default;
    ListenCallback& operator=(const ListenCallback&) = delete;
    ListenCallback& operator=(ListenCallback&&) = default;
};
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif
