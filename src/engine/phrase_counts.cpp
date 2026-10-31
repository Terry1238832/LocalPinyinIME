#include "phrase_counts.h"

#include <windows.h>

#include <algorithm>
#include <cstring>

namespace localpinyin {
namespace {

constexpr size_t kHeaderBytes = 12;
constexpr std::uint64_t kFnvOffset = 0xCBF29CE484222325ULL;
constexpr std::uint64_t kFnvPrime = 0x100000001B3ULL;

}  // namespace

struct PhraseCounts::Storage {
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = nullptr;
    const void* view = nullptr;

    Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    ~Storage() {
        if (view) UnmapViewOfFile(view);
        if (mapping) CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    }
};

void PhraseCounts::reset() noexcept {
    keys_ = nullptr;
    levels_ = nullptr;
    count_ = 0;
    storage_.reset();
}

bool PhraseCounts::load(const std::wstring& path) {
    reset();
    auto storage = std::make_shared<Storage>();
    storage->file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (storage->file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(storage->file, &size) || size.QuadPart < static_cast<LONGLONG>(kHeaderBytes) ||
        size.QuadPart > 256LL * 1024 * 1024) return false;
    storage->mapping = CreateFileMappingW(storage->file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!storage->mapping) return false;
    storage->view = MapViewOfFile(storage->mapping, FILE_MAP_READ, 0, 0, 0);
    if (!storage->view) return false;
    const auto* bytes = static_cast<const unsigned char*>(storage->view);
    std::uint32_t header[2]{};
    std::memcpy(header, bytes + 4, sizeof(header));
    if (std::memcmp(bytes, "LPPC", 4) != 0 || header[0] != 1) return false;
    const size_t count = header[1];
    if (kHeaderBytes + count * 5 != static_cast<size_t>(size.QuadPart)) return false;
    keys_ = reinterpret_cast<const std::uint32_t*>(bytes + kHeaderBytes);
    levels_ = bytes + kHeaderBytes + count * 4;
    count_ = count;
    storage_ = std::move(storage);
    return true;
}

int PhraseCounts::level(std::wstring_view phrase) const noexcept {
    if (count_ == 0) return 0;
    std::uint64_t hash = kFnvOffset;
    for (const wchar_t unit : phrase) hash = (hash ^ static_cast<std::uint16_t>(unit)) * kFnvPrime;
    const auto key = static_cast<std::uint32_t>(hash >> 32);
    const auto* end = keys_ + count_;
    const auto* found = std::lower_bound(keys_, end, key);
    return found == end || *found != key ? 0 : levels_[found - keys_];
}

}  // namespace localpinyin
