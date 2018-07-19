// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_NETWORK_ZEROMQ_PAIREVENTCALLBACK_HPP
#define OPENTXS_NETWORK_ZEROMQ_PAIREVENTCALLBACK_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/network/zeromq/ListenCallback.hpp"
#include "opentxs/Proto.hpp"

#include <functional>

#ifdef SWIG
// clang-format off
%ignore opentxs::Pimpl<opentxs::network::zeromq::PairEventCallback>::Pimpl(opentxs::network::zeromq::PairEventCallback const &);
%ignore opentxs::Pimpl<opentxs::network::zeromq::PairEventCallback>::operator opentxs::network::zeromq::PairEventCallback&;
%ignore opentxs::Pimpl<opentxs::network::zeromq::PairEventCallback>::operator const opentxs::network::zeromq::PairEventCallback &;
%rename(assign) operator=(const opentxs::network::zeromq::PairEventCallback&);
%rename(ZMQPairEventCallback) opentxs::network::zeromq::PairEventCallback;
%template(OTZMQPairEventCallback) opentxs::Pimpl<opentxs::network::zeromq::PairEventCallback>;
// clang-format on
#endif  // SWIG

namespace opentxs
{
namespace network
{
namespace zeromq
{
class PairEventCallback : virtual public ListenCallback
{
public:
    using ReceiveCallback = std::function<void(const proto::PairEvent&)>;

#ifndef SWIG
    EXPORT static OTZMQPairEventCallback Factory(ReceiveCallback callback);
#endif
    EXPORT static opentxs::Pimpl<opentxs::network::zeromq::PairEventCallback>
    Factory(PairEventCallbackSwig* callback);

    EXPORT virtual ~PairEventCallback() = default;

protected:
    PairEventCallback() = default;

private:
    friend OTZMQPairEventCallback;

    virtual PairEventCallback* clone() const override = 0;

    PairEventCallback(const PairEventCallback&) = delete;
    PairEventCallback(PairEventCallback&&) = default;
    PairEventCallback& operator=(const PairEventCallback&) = delete;
    PairEventCallback& operator=(PairEventCallback&&) = default;
};
}  // namespace zeromq
}  // namespace network
}  // namespace opentxs
#endif  // OPENTXS_NETWORK_ZEROMQ_PAIREVENTCALLBACK_HPP
