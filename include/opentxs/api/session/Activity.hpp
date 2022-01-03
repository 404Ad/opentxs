// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Version.hpp"  // IWYU pragma: associated

#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "opentxs/Types.hpp"
#include "opentxs/core/contract/UnitDefinition.hpp"
#include "opentxs/util/Bytes.hpp"
#include "opentxs/util/Time.hpp"

namespace opentxs
{
namespace api
{
namespace session
{
namespace internal
{
class Activity;
class Session;
}  // namespace internal
}  // namespace session

class Session;
}  // namespace api

namespace blockchain
{
namespace block
{
namespace bitcoin
{
class Transaction;
}  // namespace bitcoin
}  // namespace block
}  // namespace blockchain

namespace identifier
{
class Nym;
}  // namespace identifier

namespace proto
{
class StorageThread;
}  // namespace proto

class Identifier;
class PasswordPrompt;
class PeerObject;
}  // namespace opentxs

namespace opentxs::api::session
{
class OPENTXS_EXPORT Activity
{
public:
    virtual auto AddBlockchainTransaction(
        const blockchain::block::bitcoin::Transaction& transaction)
        const noexcept -> bool = 0;
    virtual auto AddPaymentEvent(
        const identifier::Nym& nymID,
        const Identifier& threadID,
        const StorageBox type,
        const Identifier& itemID,
        const Identifier& workflowID,
        Time time) const noexcept -> bool = 0;
    OPENTXS_NO_EXPORT virtual auto Internal() const noexcept
        -> const internal::Activity& = 0;
    /**   Obtain a list of mail objects in a specified box
     *
     *    \param[in] nym the identifier of the nym who owns the mail box
     *    \param[in] box the box to be listed
     */
    virtual auto Mail(const identifier::Nym& nym, const StorageBox box)
        const noexcept -> ObjectList = 0;
    /**   Delete a mail object
     *
     *    \param[in] nym the identifier of the nym who owns the mail box
     *    \param[in] mail the mail object to be stored
     *    \param[in] box the box from which to retrieve the mail object
     *    \returns The id of the stored message. The string will be empty if
     *             the mail object can not be stored.
     */
    virtual auto MailRemove(
        const identifier::Nym& nym,
        const Identifier& id,
        const StorageBox box) const noexcept -> bool = 0;
    /**   Retrieve the text from a message
     *
     *    \param[in] nym the identifier of the nym who owns the mail box
     *    \param[in] id the identifier of the mail object
     *    \param[in] box the box from which to retrieve the mail object
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    virtual auto MailText(
        const identifier::Nym& nym,
        const Identifier& id,
        const StorageBox& box,
        const PasswordPrompt& reason) const noexcept
        -> std::shared_future<std::string> = 0;
    /**   Mark a thread item as read
     *
     *    \param[in] nymId the identifier of the nym who owns the thread
     *    \param[in] threadId the thread containing the item to be marked
     *    \param[in] itemId the identifier of the item to be marked read
     *    \returns False if the nym, thread, or item does not exist
     */
    virtual auto MarkRead(
        const identifier::Nym& nymId,
        const Identifier& threadId,
        const Identifier& itemId) const noexcept -> bool = 0;
    /**   Mark a thread item as unread
     *
     *    \param[in] nymId the identifier of the nym who owns the thread
     *    \param[in] threadId the thread containing the item to be marked
     *    \param[in] itemId the identifier of the item to be marked unread
     *    \returns False if the nym, thread, or item does not exist
     */
    virtual auto MarkUnread(
        const identifier::Nym& nymId,
        const Identifier& threadId,
        const Identifier& itemId) const noexcept -> bool = 0;
    /**   Summarize a payment workflow event in human-friendly test form
     *
     *    \param[in] nym the identifier of the nym who owns the thread
     *    \param[in] id the identifier of the payment item
     *    \param[in] workflow the identifier of the payment workflow
     *    \returns A smart pointer to the object. The smart pointer will not be
     *             instantiated if the object does not exist or is invalid.
     */
    virtual auto PaymentText(
        const identifier::Nym& nym,
        const std::string& id,
        const std::string& workflow) const noexcept
        -> std::shared_ptr<const std::string> = 0;
    /**   Asynchronously cache the most recent items in each of a nym's threads
     *
     *    \param[in] nymID the identifier of the nym who owns the thread
     *    \param[in] count the number of items to preload in each thread
     */
    virtual auto PreloadActivity(
        const identifier::Nym& nymID,
        const std::size_t count,
        const PasswordPrompt& reason) const noexcept -> void = 0;
    /**   Asynchronously cache the items in an activity thread
     *
     *    \param[in] nymID the identifier of the nym who owns the thread
     *    \param[in] threadID the thread containing the items to be cached
     *    \param[in] start the first item to be cached
     *    \param[in] count the number of items to cache
     */
    virtual auto PreloadThread(
        const identifier::Nym& nymID,
        const Identifier& threadID,
        const std::size_t start,
        const std::size_t count,
        const PasswordPrompt& reason) const noexcept -> void = 0;
    OPENTXS_NO_EXPORT virtual auto Thread(
        const identifier::Nym& nymID,
        const Identifier& threadID,
        proto::StorageThread& serialized) const noexcept -> bool = 0;
    virtual auto Thread(
        const identifier::Nym& nymID,
        const Identifier& threadID,
        AllocateOutput output) const noexcept -> bool = 0;
    /**   Obtain a list of thread ids for the specified nym
     *
     *    \param[in] nym the identifier of the nym
     *    \param[in] unreadOnly if true, only return threads with unread items
     */
    virtual auto Threads(
        const identifier::Nym& nym,
        const bool unreadOnly = false) const noexcept -> ObjectList = 0;
    /**   Return the total number of unread thread items for a nym
     *
     *    \param[in] nymId
     */
    virtual auto UnreadCount(const identifier::Nym& nym) const noexcept
        -> std::size_t = 0;
    /** Activity thread update notification
     *
     *  A subscribe socket can connect to this endpoint to receive
     *  ActivityThreadUpdated tagged messages
     *
     *  See opentxs/util/WorkTypes.hpp for message format documentation
     */
    virtual auto ThreadPublisher(const identifier::Nym& nym) const noexcept
        -> std::string = 0;

    OPENTXS_NO_EXPORT virtual auto Internal() noexcept
        -> internal::Activity& = 0;

    OPENTXS_NO_EXPORT virtual ~Activity() = default;

protected:
    Activity() = default;

private:
    Activity(const Activity&) = delete;
    Activity(Activity&&) = delete;
    auto operator=(const Activity&) -> Activity& = delete;
    auto operator=(Activity&&) -> Activity& = delete;
};
}  // namespace opentxs::api::session