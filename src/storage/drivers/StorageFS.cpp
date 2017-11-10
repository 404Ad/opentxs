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
#include "opentxs/stdafx.hpp"

#include "opentxs/storage/drivers/StorageFS.hpp"

#if OT_STORAGE_FS
#include "opentxs/storage/StorageConfig.hpp"

#include <boost/filesystem.hpp>

#include <cstdio>
#include <fstream>
#include <ios>
#include <iostream>
#include <thread>
#include <vector>

extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

#define PATH_SEPERATOR "/"

#define OT_METHOD "opentxs::StorageFS::"

namespace opentxs
{

StorageFS::StorageFS(
    const StorageConfig& config,
    const Digest& hash,
    const Random& random,
    const std::string& folder,
    const std::atomic<bool>& bucket)
    : ot_super(config, hash, random, bucket)
    , folder_(folder)
    , path_seperator_(PATH_SEPERATOR)
    , ready_(false)
{
    Init_StorageFS();
}

void StorageFS::Cleanup() { Cleanup_StorageFS(); }

void StorageFS::Cleanup_StorageFS()
{
    // future cleanup actions go here
}

void StorageFS::Init_StorageFS()
{
    // future init actions go here
}

bool StorageFS::LoadFromBucket(
    const std::string& key,
    std::string& value,
    const bool bucket) const
{
    value.clear();
    std::string directory{};
    const auto filename = calculate_path(key, bucket, directory);

    if (false == boost::filesystem::exists(filename)) {

        return false;
    }

    if (ready_.load() && false == folder_.empty()) {
        value = read_file(filename);
    }

    return false == value.empty();
}

std::string StorageFS::LoadRoot() const
{
    if (ready_.load() && false == folder_.empty()) {

        return read_file(root_filename());
    }

    return "";
}

std::string StorageFS::prepare_read(const std::string& input) const
{
    return input;
}

std::string StorageFS::prepare_write(const std::string& input) const
{
    return input;
}

std::string StorageFS::read_file(const std::string& filename) const
{
    if (false == boost::filesystem::exists(filename)) {

        return {};
    }

    std::ifstream file(
        filename, std::ios::in | std::ios::ate | std::ios::binary);

    if (file.good()) {
        std::ifstream::pos_type pos = file.tellg();

        if ((0 >= pos) || (0xFFFFFFFF <= pos)) {
            return {};
        }

        std::uint32_t size(pos);
        file.seekg(0, std::ios::beg);
        std::vector<char> bytes(size);
        file.read(&bytes[0], size);

        return prepare_read(std::string(&bytes[0], size));
    }

    return {};
}

void StorageFS::store(
    const bool,
    const std::string& key,
    const std::string& value,
    const bool bucket,
    std::promise<bool>* promise) const
{
    OT_ASSERT(nullptr != promise);

    if (ready_.load() && false == folder_.empty()) {
        std::string directory{};
        const auto filename = calculate_path(key, bucket, directory);
        promise->set_value(write_file(directory, filename, value));
    } else {
        promise->set_value(false);
    }
}

bool StorageFS::StoreRoot(const bool, const std::string& hash) const
{
    if (ready_.load() && false == folder_.empty()) {

        return write_file(folder_, root_filename(), hash);
    }

    return false;
}

bool StorageFS::sync(const std::string& path) const
{
    class FileDescriptor
    {
    public:
        FileDescriptor(const std::string& path)
            : fd_(::open(path.c_str(), O_DIRECTORY | O_RDONLY))
        {
        }

        operator bool() const { return good(); }
        operator int() const { return fd_; }

        ~FileDescriptor()
        {
            if (good()) {
                ::close(fd_);
            }
        }

    private:
        int fd_{-1};

        bool good() const { return (-1 != fd_); }

        FileDescriptor() = delete;
        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor(FileDescriptor&&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;
        FileDescriptor& operator=(FileDescriptor&&) = delete;
    };

    FileDescriptor fd(path);

    if (!fd) {
        otErr << OT_METHOD << __FUNCTION__ << ": Failed to open " << path
              << std::endl;

        return false;
    }

    return sync(fd);
}

bool StorageFS::sync(File& file) const { return sync(file->handle()); }

bool StorageFS::sync(int fd) const
{
#if defined(__APPLE__)
    // This is a Mac OS X system which does not implement
    // fsync as such.
    return 0 == ::fcntl(fd, F_FULLFSYNC);
#else
    return 0 == ::fsync(fd);
#endif
}

bool StorageFS::write_file(
    const std::string& directory,
    const std::string& filename,
    const std::string& contents) const
{
    if (false == filename.empty()) {
        boost::filesystem::path filePath(filename);
        File file(filePath);
        const auto data = prepare_write(contents);

        if (file.good()) {
            file.write(data.c_str(), data.size());

            if (false == sync(file)) {
                otErr << OT_METHOD << __FUNCTION__ << ": Failed to sync file "
                      << filename << std::endl;
            }

            if (false == sync(directory)) {
                otErr << OT_METHOD << __FUNCTION__
                      << ": Failed to sync directory " << directory
                      << std::endl;
            }

            file.close();

            return true;
        } else {
            otErr << OT_METHOD << __FUNCTION__ << ": Failed to write file."
                  << std::endl;
        }
    } else {
        otErr << OT_METHOD << __FUNCTION__
              << ": Failed to write empty filename." << std::endl;
    }

    return false;
}

StorageFS::~StorageFS() { Cleanup_StorageFS(); }

}  // namespace opentxs
#endif
