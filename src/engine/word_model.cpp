#include "word_model.h"

#include <windows.h>

#include <algorithm>
#include <cstring>

namespace localpinyin {
namespace {

// Version 1 stores pairs; version 2 adds runs of three words.
constexpr size_t kVersion1HeaderBytes = 20;
constexpr size_t kVersion2HeaderBytes = 24;
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
    word_keys_ = pair_keys_ = triple_keys_ = nullptr;
    pair_scores_ = triple_scores_ = nullptr;
    word_count_ = pair_count_ = triple_count_ = 0;
    storage_.reset();
}

bool WordModel::load(const std::wstring& path) {
    reset();
    auto storage = std::make_shared<Storage>();
    storage->file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (storage->file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(storage->file, &size) || size.QuadPart < static_cast<LONGLONG>(kVersion1HeaderBytes) ||
        size.QuadPart > 256LL * 1024 * 1024) return false;
    storage->mapping = CreateFileMappingW(storage->file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!storage->mapping) return false;
    storage->view = MapViewOfFile(storage->mapping, FILE_MAP_READ, 0, 0, 0);
    if (!storage->view) return false;
    const auto* bytes = static_cast<const unsigned char*>(storage->view);
    std::uint32_t header[5]{};
    std::memcpy(header, bytes + 4, sizeof(std::uint32_t) * 4);
    if (std::memcmp(bytes, "LPWM", 4) != 0 || (header[0] != 1 && header[0] != 2)) return false;
    const size_t header_bytes = header[0] == 1 ? kVersion1HeaderBytes : kVersion2HeaderBytes;
    if (header[0] == 2 && size.QuadPart >= static_cast<LONGLONG>(kVersion2HeaderBytes))
        std::memcpy(&header[4], bytes + 20, sizeof(std::uint32_t));
    const size_t words = header[2];
    const size_t pairs = header[3];
    const size_t triples = header[4];
    if (header_bytes + words * 4 + (pairs + triples) * 6 != static_cast<size_t>(size.QuadPart)) return false;
    const auto* base = bytes + header_bytes;
    word_keys_ = reinterpret_cast<const std::uint32_t*>(base);
    pair_keys_ = reinterpret_cast<const std::uint32_t*>(base + words * 4);
    triple_keys_ = reinterpret_cast<const std::uint32_t*>(base + (words + pairs) * 4);
    pair_scores_ = reinterpret_cast<const std::int16_t*>(base + (words + pairs + triples) * 4);
    triple_scores_ = reinterpret_cast<const std::int16_t*>(base + (words + pairs + triples) * 4 + pairs * 2);
    word_count_ = words;
    pair_count_ = pairs;
    triple_count_ = triples;
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

int WordModel::triple_score(std::wstring_view first, std::wstring_view second, std::wstring_view word) const noexcept {
    if (triple_count_ == 0 || first.empty() || second.empty() || word.empty()) return 0;
    const wchar_t separator = 1;
    const std::wstring_view mark(&separator, 1);
    const auto key = static_cast<std::uint32_t>(fnv(word, fnv(mark, fnv(second, fnv(mark, fnv(first))))) >> 32);
    size_t index = 0;
    return contains(triple_keys_, triple_count_, key, &index) ? triple_scores_[index] : 0;
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
