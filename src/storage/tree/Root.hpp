// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Internal.hpp"

#include "opentxs/api/Editor.hpp"
#include "opentxs/Types.hpp"

#include "Node.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <string>
#include <thread>

namespace opentxs
{
class StorageMultiplex;

namespace api
{
namespace storage
{
namespace implementation
{
class Storage;
}  // namespace implementation
}  // namespace storage
}  // namespace api

namespace storage
{
class Tree;

class Root : public Node
{
private:
    typedef Node ot_super;
    friend class opentxs::StorageMultiplex;
    friend class api::storage::implementation::Storage;

    const std::uint64_t gc_interval_{std::numeric_limits<std::int64_t>::max()};
    mutable std::string gc_root_;
    Flag& current_bucket_;
    mutable OTFlag gc_running_;
    mutable OTFlag gc_resume_;
    mutable std::atomic<std::uint64_t> last_gc_;
    mutable std::atomic<std::uint64_t> sequence_;
    mutable std::mutex gc_lock_;
    mutable std::unique_ptr<std::thread> gc_thread_;
    std::string tree_root_;
    mutable std::mutex tree_lock_;
    mutable std::unique_ptr<class Tree> tree_;

    proto::StorageRoot serialize() const;
    class Tree* tree() const;

    void cleanup() const;
    void collect_garbage(const opentxs::api::storage::Driver* to) const;
    void init(const std::string& hash) override;
    bool save(const Lock& lock, const opentxs::api::storage::Driver& to) const;
    bool save(const Lock& lock) const override;
    void save(class Tree* tree, const Lock& lock);

    Root(
        const opentxs::api::storage::Driver& storage,
        const std::string& hash,
        const std::int64_t interval,
        Flag& bucket);
    Root() = delete;
    Root(const Root&) = delete;
    Root(Root&&) = delete;
    Root operator=(const Root&) = delete;
    Root operator=(Root&&) = delete;

public:
    const class Tree& Tree() const;

    Editor<class Tree> mutable_Tree();

    bool Migrate(const opentxs::api::storage::Driver& to) const override;
    bool Save(const opentxs::api::storage::Driver& to) const;
    std::uint64_t Sequence() const;

    ~Root() = default;
};
}  // namespace storage
}  // namespace opentxs
