#include "word_model.h"

#include <windows.h>

#include <algorithm>
#include <cstring>

namespace localpinyin {
namespace {

constexpr std::uint32_t kVersion = 1;
constexpr size_t kHeaderBytes = 20;
constexpr std::uint64_t kFnvOffset = 0xCBF29CE484222325ULL;
constexpr std::uint64_t kFnvPrime = 0x100000001B3ULL;
constexpr size_t kMaxPreviousWord = 4;

std::uint64_t fnv(std::wstring_view text, std::uint64_t hash = kFnvOffset) noexcept {
    for (const wchar_t unit : text) hash = (hash ^ static_cast<std::uint16_t>(unit)) * kFnvPrime;
    return hash;
}

bool contains(const std::uint32_t* keys, size_t count, std::uint32_t key, size_t* index = nullptr) noexcept {
    const auto* end = keys + count;
    const auto* found = std::lower_bound(keys, end, key);
    if (found == end || *found != key) return false;
    if (index) *index = static_cast<size_t>(found - keys);
    return true;
}

}  // namespace

struct WordModel::Storage {
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

void WordModel::reset() noexcept {
    word_keys_ = pair_keys_ = nullptr;
    pair_scores_ = nullptr;
    word_count_ = pair_count_ = 0;
    storage_.reset();
}

bool WordModel::load(const std::wstring& path) {
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
    std::uint32_t header[4]{};
    std::memcpy(header, bytes + 4, sizeof(header));
    if (std::memcmp(bytes, "LPWM", 4) != 0 || header[0] != kVersion) return false;
    const size_t words = header[2];
    const size_t pairs = header[3];
    if (kHeaderBytes + words * 4 + pairs * 6 != static_cast<size_t>(size.QuadPart)) return false;
    word_keys_ = reinterpret_cast<const std::uint32_t*>(bytes + kHeaderBytes);
    pair_keys_ = reinterpret_cast<const std::uint32_t*>(bytes + kHeaderBytes + words * 4);
    pair_scores_ = reinterpret_cast<const std::int16_t*>(bytes + kHeaderBytes + words * 4 + pairs * 4);
    word_count_ = words;
    pair_count_ = pairs;
    storage_ = std::move(storage);
    return true;
}

int WordModel::pair_score(std::wstring_view previous, std::wstring_view word) const noexcept {
    if (pair_count_ == 0 || previous.empty() || word.empty()) return 0;
    const wchar_t separator = 1;
    const auto key = static_cast<std::uint32_t>(fnv(word, fnv(std::wstring_view(&separator, 1), fnv(previous))) >> 32);
    size_t index = 0;
    return contains(pair_keys_, pair_count_, key, &index) ? pair_scores_[index] : 0;
}

std::wstring WordModel::previous_word(std::wstring_view preceding) const {
    if (word_count_ == 0) return {};
    for (size_t length = std::min(kMaxPreviousWord, preceding.size()); length > 0; --length) {
        const auto suffix = preceding.substr(preceding.size() - length);
        if (contains(word_keys_, word_count_, static_cast<std::uint32_t>(fnv(suffix) >> 32))) return std::wstring(suffix);
    }
    return {};
}

}  // namespace localpinyin
