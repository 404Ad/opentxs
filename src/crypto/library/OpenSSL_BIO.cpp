// Copyright (c) 2010-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "stdafx.hpp"

#include "OpenSSL_BIO.hpp"

#include "opentxs/core/Log.hpp"
#include "opentxs/core/String.hpp"

#include <cstddef>
#include <vector>

#define READ_AMOUNT 256

#define OT_METHOD "opentxs::crypto::implementation::OpenSSL_BIO::"

namespace opentxs::crypto::implementation
{
// OpenSSL_BIO
BIO* OpenSSL_BIO::assertBioNotNull(BIO* pBIO)
{
    if (nullptr == pBIO) OT_FAIL;
    return pBIO;
}

OpenSSL_BIO::OpenSSL_BIO(BIO* pBIO)
    : m_refBIO(*assertBioNotNull(pBIO))
    , bCleanup(true)
    , bFreeOnly(false)
{
}

OpenSSL_BIO::~OpenSSL_BIO()
{
    if (bCleanup) {
        if (bFreeOnly) {
            BIO_free(&m_refBIO);
        } else {
            BIO_free_all(&m_refBIO);
        }
    }
}

OpenSSL_BIO::operator BIO*() const { return (&m_refBIO); }

void OpenSSL_BIO::release() { bCleanup = false; }

void OpenSSL_BIO::setFreeOnly() { bFreeOnly = true; }

void OpenSSL_BIO::read_bio(
    const std::size_t amount,
    std::size_t& read,
    std::size_t& total,
    std::vector<std::byte>& output)
{
    output.resize(output.size() + amount);
    read = BIO_read(*this, &output[total], amount);
    total += read;
}

std::vector<std::byte> OpenSSL_BIO::ToBytes()
{
    std::size_t read{0};
    std::size_t total{0};
    std::vector<std::byte> output{};
    read_bio(READ_AMOUNT, read, total, output);

    if (0 == read) {
        LogOutput(OT_METHOD)(__FUNCTION__)(": Read failed").Flush();

        return {};
    }

    while (READ_AMOUNT == read) { read_bio(READ_AMOUNT, read, total, output); }

    output.resize(total);
    LogInsane(OT_METHOD)(__FUNCTION__)(": Read ")(total)(" bytes").Flush();

    return output;
}

OTString OpenSSL_BIO::ToString()
{
    auto output = String::Factory();
    auto bytes = ToBytes();
    const auto size = bytes.size();

    if (0 < size) {
        bytes.resize(size + 1);
        bytes[size] = static_cast<std::byte>(0x0);
        output->Set(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

    return output;
}
}  // namespace opentxs::crypto::implementation
