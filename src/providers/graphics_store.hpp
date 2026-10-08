#pragma once
#include "graphics_record.hpp"
#include <filesystem>
#include <cstddef>
#include <cstring>
#include <memory>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace mcd2::providers {

enum class StoreStatus { Ok, Missing, Invalid, IoError, Busy, Conflict, PublishedSyncUncertain };
struct StoreResult {
    StoreStatus status = StoreStatus::IoError;
    RecordStamp stamp;
};

namespace store_detail {
#ifdef _WIN32
using Handle = HANDLE;
inline const Handle badHandle = INVALID_HANDLE_VALUE;
inline void closeHandle(Handle h) { if (h != badHandle) CloseHandle(h); }
inline bool regularHandle(Handle h, bool requireLinked = true) {
    BY_HANDLE_FILE_INFORMATION info{};
    return GetFileInformationByHandle(h, &info) &&
           (requireLinked ? info.nNumberOfLinks == 1 : info.nNumberOfLinks <= 1) &&
           !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}
#else
using Handle = int;
inline constexpr Handle badHandle = -1;
inline void closeHandle(Handle h) { if (h != badHandle) ::close(h); }
inline bool regularHandle(Handle h, bool requireLinked = true) {
    struct stat info{};
    return !fstat(h, &info) && S_ISREG(info.st_mode) &&
           (requireLinked ? info.st_nlink == 1 : info.st_nlink <= 1);
}
#endif
struct File {
    Handle handle = badHandle;
    explicit File(Handle h = badHandle) : handle(h) {}
    ~File() { closeHandle(handle); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;
};

// Caller supplies an existing, trusted private directory. This is not a
// defense against another program modifying that directory as the same user.
inline bool directoryReady(const std::filesystem::path& root) {
    if (!root.is_absolute()) return false;
    std::error_code error;
    if (std::filesystem::symlink_status(root, error).type() != std::filesystem::file_type::directory || error)
        return false;
#ifdef _WIN32
    const auto attributes = GetFileAttributesW(root.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
#endif
    return true;
}
inline StoreResult read(const std::filesystem::path& path, DecodedGraphicsRecord& out) {
#ifdef _WIN32
    File file(CreateFileW(path.c_str(), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                         FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (file.handle == badHandle) {
        const auto error = GetLastError();
        return {error == ERROR_FILE_NOT_FOUND ? StoreStatus::Missing : StoreStatus::IoError, {}};
    }
    LARGE_INTEGER size{};
    // A replacement may unlink the old filename after this reader opened it.
    // That handle still owns a complete old snapshot, not an invalid file.
    if (!regularHandle(file.handle, false)) return {StoreStatus::Invalid, {}};
    if (!GetFileSizeEx(file.handle, &size)) return {StoreStatus::IoError, {}};
    if (size.QuadPart != 128) return {StoreStatus::Invalid, {}};
    GraphicsRecord bytes{};
    DWORD count = 0;
    if (!ReadFile(file.handle, bytes.data(), DWORD(bytes.size()), &count, nullptr) || count != bytes.size())
        return {StoreStatus::IoError, {}};
#else
    File file(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (file.handle == badHandle) return {errno == ENOENT ? StoreStatus::Missing : StoreStatus::IoError, {}};
    if (!regularHandle(file.handle, false)) return {StoreStatus::Invalid, {}};
    struct stat info{};
    if (fstat(file.handle, &info)) return {StoreStatus::IoError, {}};
    if (info.st_size != 128) return {StoreStatus::Invalid, {}};
    GraphicsRecord bytes{};
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto count = ::read(file.handle, bytes.data()+offset, bytes.size()-offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return {StoreStatus::IoError, {}};
        offset += std::size_t(count);
    }
#endif
    DecodedGraphicsRecord next;
    if (!decodeGraphicsRecord(bytes, next)) return {StoreStatus::Invalid, {}};
    out = next;
    return {StoreStatus::Ok, next.bootstrap.stamp};
}

// A persistent, empty lock file with an OS-held lock. Never unlink it: removing
// the inode would allow two writers to hold different locks on the same path.
// Process death releases the lock, unlike a CREATE_NEW lock-file sentinel.
struct WriterLock {
    File file;
    StoreStatus status = StoreStatus::IoError;
    explicit WriterLock(const std::filesystem::path& path)
#ifdef _WIN32
        : file(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_FLAG_OPEN_REPARSE_POINT, nullptr)) {
        if (file.handle == badHandle || !regularHandle(file.handle)) return;
        OVERLAPPED range{};
        if (!LockFileEx(file.handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &range)) {
            if (GetLastError() == ERROR_LOCK_VIOLATION) status = StoreStatus::Busy;
            return;
        }
#else
        : file(::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600)) {
        if (file.handle == badHandle || !regularHandle(file.handle)) return;
        if (flock(file.handle, LOCK_EX | LOCK_NB)) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) status = StoreStatus::Busy;
            return;
        }
#endif
        status = StoreStatus::Ok;
    }
};

inline bool discardPending(const std::filesystem::path& path) {
    std::error_code error;
    const auto type = std::filesystem::symlink_status(path, error).type();
    if (type == std::filesystem::file_type::not_found) return true;
    if (error || type != std::filesystem::file_type::regular) return false;
#ifdef _WIN32
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
#endif
    return std::filesystem::remove(path, error) && !error;
}
inline bool writeFlushed(const std::filesystem::path& path, const GraphicsRecord& bytes) {
#ifdef _WIN32
    File file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (file.handle == badHandle || !regularHandle(file.handle)) return false;
    DWORD count = 0;
    return WriteFile(file.handle, bytes.data(), DWORD(bytes.size()), &count, nullptr) &&
           count == bytes.size() && FlushFileBuffers(file.handle);
#else
    File file(::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (file.handle == badHandle || !regularHandle(file.handle)) return false;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto count = ::write(file.handle, bytes.data()+offset, bytes.size()-offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        offset += std::size_t(count);
    }
    return !fsync(file.handle);
#endif
}
inline bool replace(const std::filesystem::path& pending, const std::filesystem::path& committed) {
#ifdef _WIN32
    // MoveFileEx's legacy replacement can fail while an old reader is open,
    // even with delete sharing. Windows 10+ POSIX rename semantics explicitly
    // preserve old handles while subsequent opens see the new file. If the
    // filesystem/API does not support it, refuse rather than delete or copy.
    File file(CreateFileW(pending.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (file.handle == badHandle || !regularHandle(file.handle)) return false;
    const auto filename = committed.native();
    if (filename.empty() || filename.size() > 32767) return false;
    // Own spelling of the documented union layout also compiles when an older
    // header exposes ReplaceIfExists but not the Flags union member. The real
    // SDK field offsets remain compile-time checked; no private NT API is used.
    struct RenameRequest { DWORD flags; HANDLE root; DWORD filenameBytes; WCHAR name[1]; };
    static_assert(offsetof(RenameRequest, root) == offsetof(FILE_RENAME_INFO, RootDirectory));
    static_assert(offsetof(RenameRequest, filenameBytes) == offsetof(FILE_RENAME_INFO, FileNameLength));
    static_assert(offsetof(RenameRequest, name) == offsetof(FILE_RENAME_INFO, FileName));
    constexpr DWORD replaceExisting = 1, posixSemantics = 2;
    const auto nameBytes = DWORD(filename.size()*sizeof(wchar_t));
    const auto size = DWORD(sizeof(RenameRequest)+nameBytes);
    auto storage = std::make_unique<std::uint64_t[]>((size+7)/8);
    auto info = new (storage.get()) RenameRequest{};
    info->flags = replaceExisting | posixSemantics;
    info->filenameBytes = nameBytes;
    std::memcpy(reinterpret_cast<std::uint8_t*>(info)+offsetof(RenameRequest, name), filename.data(), nameBytes);
    return SetFileInformationByHandle(file.handle, FileRenameInfoEx, info, size);
#else
    return !::rename(pending.c_str(), committed.c_str());
#endif
}
inline bool syncDirectory(const std::filesystem::path& root) {
#ifdef _WIN32
    (void)root; // File bytes were flushed; no portable Windows directory fsync.
    return true;
#else
    File directory(::open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW));
    return directory.handle != badHandle && !fsync(directory.handle);
#endif
}
} // namespace store_detail

class GraphicsStore {
    std::filesystem::path root_;
public:
    explicit GraphicsStore(std::filesystem::path directory) : root_(std::move(directory)) {}
    // Names live in a dedicated provider-settings directory, not legacy .sav
    // slots. No discovery, directory creation, backup deletion or save migration
    // happens as a side effect of constructing/reading the store.
    std::filesystem::path committedPath() const { return root_ / "intent-v5.bin"; }
    std::filesystem::path pendingPath() const { return root_ / "intent-v5.pending"; }
    std::filesystem::path lockPath() const { return root_ / "intent-v5.lock"; }
    StoreResult load(DecodedGraphicsRecord& out) const {
        if (!store_detail::directoryReady(root_)) return {StoreStatus::IoError, {}};
        return store_detail::read(committedPath(), out);
    }
    StoreResult publish(const GraphicsIntent& intent, RecordStamp expected) const {
        GraphicsRecord bytes;
        if (!encodeGraphicsRecord(intent, bytes)) return {StoreStatus::Invalid, {}};
        if (!store_detail::directoryReady(root_)) return {StoreStatus::IoError, {}};
        store_detail::WriterLock lock(lockPath());
        if (lock.status != StoreStatus::Ok) return {lock.status, {}};
        DecodedGraphicsRecord before;
        const auto current = load(before);
        if (current.status != StoreStatus::Ok && current.status != StoreStatus::Missing) return current;
        if (current.stamp != expected || intent.revision <= expected.revision)
            return {StoreStatus::Conflict, current.stamp};
        // Discard only our reserved pending file under the writer lock. Readers
        // never activate this file, including after a crash before commit.
        if (!store_detail::discardPending(pendingPath())) return {StoreStatus::IoError, current.stamp};
        if (!store_detail::writeFlushed(pendingPath(), bytes)) return {StoreStatus::IoError, current.stamp};
        DecodedGraphicsRecord staged;
        if (store_detail::read(pendingPath(), staged).status != StoreStatus::Ok || staged.intent != intent)
            return {StoreStatus::IoError, current.stamp};
        if (!store_detail::replace(pendingPath(), committedPath())) return {StoreStatus::IoError, current.stamp};
        // After replacement, the new record may already be visible even if the
        // durability check fails. Callers must reload; do not blindly retry the
        // old expected stamp or label this result a rollback.
        const auto status = store_detail::syncDirectory(root_) ? StoreStatus::Ok : StoreStatus::PublishedSyncUncertain;
        return {status, staged.bootstrap.stamp};
    }
};

} // namespace mcd2::providers
