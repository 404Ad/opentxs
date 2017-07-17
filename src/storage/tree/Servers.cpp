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

#include "opentxs/core/stdafx.hpp"

#include "opentxs/storage/tree/Servers.hpp"

#include "opentxs/storage/Storage.hpp"
#include "opentxs/storage/StoragePlugin.hpp"

namespace opentxs
{
namespace storage
{
Servers::Servers(const StorageDriver& storage, const std::string& hash)
    : Node(storage, hash)
{
    if (check_hash(hash)) {
        init(hash);
    } else {
        version_ = 2;
        root_ = Node::BLANK_HASH;
    }
}

std::string Servers::Alias(const std::string& id) const
{
    return get_alias(id);
}

bool Servers::Delete(const std::string& id) { return delete_item(id); }

void Servers::init(const std::string& hash)
{
    std::shared_ptr<proto::StorageServers> serialized;
    driver_.LoadProto(hash, serialized);

    if (!serialized) {
        std::cerr << __FUNCTION__ << ": Failed to load servers index file."
                  << std::endl;
        abort();
    }

    version_ = serialized->version();

    // Upgrade to version 2
    if (2 > version_) {
        version_ = 2;
    }

    for (const auto& it : serialized->server()) {
        item_map_.emplace(
            it.itemid(), Metadata{it.hash(), it.alias(), 0, false});
    }
}

bool Servers::Load(
    const std::string& id,
    std::shared_ptr<proto::ServerContract>& output,
    std::string& alias,
    const bool checking) const
{
    return load_proto<proto::ServerContract>(id, output, alias, checking);
}

void Servers::Map(ServerLambda lambda) const
{
    map<proto::ServerContract>(lambda);
}

bool Servers::save(const std::unique_lock<std::mutex>& lock) const
{
    if (!verify_write_lock(lock)) {
        std::cerr << __FUNCTION__ << ": Lock failure." << std::endl;
        abort();
    }

    auto serialized = serialize();

    if (!proto::Validate(serialized, VERBOSE)) {
        return false;
    }

    return driver_.StoreProto(serialized, root_);
}

proto::StorageServers Servers::serialize() const
{
    proto::StorageServers serialized;
    serialized.set_version(version_);

    for (const auto item : item_map_) {
        const bool goodID = !item.first.empty();
        const bool goodHash = check_hash(std::get<0>(item.second));
        const bool good = goodID && goodHash;

        if (good) {
            serialize_index(item.first, item.second, *serialized.add_server());
        }
    }

    return serialized;
}

bool Servers::SetAlias(const std::string& id, const std::string& alias)
{
    return set_alias(id, alias);
}

bool Servers::Store(
    const proto::ServerContract& data,
    const std::string& alias,
    std::string& plaintext)
{
    return store_proto(data, data.id(), alias, plaintext);
}
}  // namespace storage
}  // namespace opentxs
