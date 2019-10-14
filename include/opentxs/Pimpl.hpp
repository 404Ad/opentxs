// Copyright (c) 2019 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_PIMPL_HPP
#define OPENTXS_PIMPL_HPP

#include <cassert>
#include <memory>

#ifdef SWIG
%ignore opentxs::Pimpl::Pimpl(Pimpl&&);
%ignore opentxs::Pimpl::operator->();
%ignore opentxs::Pimpl::get() const;
%rename(assign) opentxs::Pimpl::operator=(const Pimpl&);
%rename(move) opentxs::Pimpl::operator=(Pimpl&&);
#endif

namespace opentxs
{
template <class C>
class Pimpl
{
public:
    using interface_type = C;

    explicit Pimpl(C* in) noexcept
        : pimpl_(in)
    {
        assert(pimpl_);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    Pimpl(const Pimpl& rhs) noexcept
        : pimpl_(
#ifndef _WIN32
              rhs.pimpl_->clone()
#else
              dynamic_cast<C*>(rhs.pimpl_->clone())
#endif
          )
    {
        assert(pimpl_);
    }
#pragma GCC diagnostic pop

    Pimpl(const C& rhs) noexcept
        : pimpl_(
#ifndef _WIN32
              rhs.clone()
#else
              dynamic_cast<C*>(rhs.clone())
#endif
          )
    {
        assert(pimpl_);
    }

    Pimpl(Pimpl&& rhs) noexcept
        : pimpl_(std::move(rhs.pimpl_))
    {
        assert(pimpl_);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
    Pimpl& operator=(const Pimpl& rhs) noexcept
    {
        pimpl_.reset(
#ifndef _WIN32
            rhs.pimpl_->clone()
#else
            dynamic_cast<C*>(rhs.pimpl_->clone())
#endif
        );
        assert(pimpl_);

        return *this;
    }
#pragma GCC diagnostic pop

    Pimpl& operator=(Pimpl&& rhs) noexcept
    {
        pimpl_ = std::move(rhs.pimpl_);
        assert(pimpl_);

        return *this;
    }

    Pimpl& operator=(const C& rhs) noexcept
    {
        pimpl_.reset(
#ifndef _WIN32
            rhs.clone()
#else
            dynamic_cast<C*>(rhs.clone())
#endif
        );
        assert(pimpl_);

        return *this;
    }

    operator C&() noexcept { return *pimpl_; }
    operator const C&() const noexcept { return *pimpl_; }

    C* operator->() { return pimpl_.get(); }
    const C* operator->() const { return pimpl_.get(); }

    C& get() noexcept { return *pimpl_; }
    const C& get() const noexcept { return *pimpl_; }

    ~Pimpl() = default;

#ifdef SWIG_VERSION
    Pimpl() = default;
#endif

private:
    std::unique_ptr<C> pimpl_{nullptr};

#ifndef SWIG_VERSION
    Pimpl() = delete;
#endif
};  // class Pimpl
}  // namespace opentxs

#endif
