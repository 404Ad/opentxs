// Copyright (c) 2018 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENTXS_CONSENSUS_TRANSACTIONSTATEMENT_HPP
#define OPENTXS_CONSENSUS_TRANSACTIONSTATEMENT_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/String.hpp"
#include "opentxs/Types.hpp"

#include <set>
#include <string>

namespace opentxs
{

class TransactionStatement
{
private:
    std::string version_;
    std::string nym_id_;
    std::string notary_;
    std::set<TransactionNumber> available_;
    std::set<TransactionNumber> issued_;

    TransactionStatement() = delete;
    TransactionStatement(const TransactionStatement& rhs) = delete;
    TransactionStatement& operator=(const TransactionStatement& rhs) = delete;
    TransactionStatement& operator=(TransactionStatement&& rhs) = delete;

public:
    TransactionStatement(
        const std::string& notary,
        const std::set<TransactionNumber>& issued,
        const std::set<TransactionNumber>& available);
    TransactionStatement(const String& serialized);
    TransactionStatement(TransactionStatement&& rhs) = default;

    explicit operator String() const;

    const std::set<TransactionNumber>& Issued() const;
    const std::string& Notary() const;

    void Remove(const TransactionNumber& number);

    ~TransactionStatement() = default;
};
}  // namespace opentxs

#endif  // OPENTXS_CONSENSUS_TRANSACTIONSTATEMENT_HPP
