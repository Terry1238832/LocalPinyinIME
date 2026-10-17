#include "dictionary_catalog.h"

#include "../common/utf_utils.h"

#include <algorithm>
#include <climits>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_set>

namespace localpinyin {
namespace {

std::wstring trim_ascii(std::wstring value) {
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    const size_t last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::wstring> split_tsv(const std::wstring& line) {
    std::vector<std::wstring> fields;
    size_t start = 0;
    while (true) {
        const size_t tab = line.find(L'\t', start);
        fields.push_back(line.substr(start, tab == std::wstring::npos ? std::wstring::npos : tab - start));
        if (tab == std::wstring::npos) break;
        start = tab + 1;
    }
    return fields;
}

bool parse_weight(const std::wstring& text, int* value) {
    wchar_t* end = nullptr;
    const long parsed = std::wcstol(text.c_str(), &end, 10);
    if (!end || *end != L'\0' || parsed < 0 || parsed > INT_MAX) return false;
    *value = static_cast<int>(parsed);
    return true;
}

// Every valid row of one layer file goes to `accept(pinyin, word, weight)`.
template <typename Accept>
void read_file(const std::filesystem::path& path, size_t* valid, size_t* invalid, Accept accept) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;
    std::string raw;
    while (std::getline(file, raw)) {
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        std::wstring line = trim_ascii(utf8_to_wide(raw));
        if (line.empty() || line.front() == L'#') continue;
        const auto fields = split_tsv(line);
        int weight = 0;
        if (fields.size() != 3 || trim_ascii(fields[0]).empty() || trim_ascii(fields[1]).empty() ||
            !parse_weight(trim_ascii(fields[2]), &weight)) {
            ++*invalid;
            continue;
        }
        accept(trim_ascii(fields[0]), trim_ascii(fields[1]), weight);
        ++*valid;
    }
}

void load_file(const std::filesystem::path& path, DictionaryCatalogSource source,
               std::map<std::pair<std::wstring, std::wstring>, DictionaryCatalogEntry>* merged,
               size_t* valid, size_t* invalid) {
    read_file(path, valid, invalid, [&](std::wstring pinyin, std::wstring word, int weight) {
        DictionaryCatalogEntry entry{std::move(pinyin), std::move(word), weight, source};
        (*merged)[{entry.pinyin, entry.word}] = entry;
    });
}

}  // namespace

bool DictionaryCatalog::load(const std::wstring& resource_directory) {
    entries_.clear();
    stats_ = {};
    std::map<std::pair<std::wstring, std::wstring>, DictionaryCatalogEntry> merged;
    const std::filesystem::path directory(resource_directory);
    load_file(directory / L"standard_zh_pinyin.tsv", DictionaryCatalogSource::Standard,
              &merged, &stats_.standard_entries, &stats_.invalid_rows);
    load_file(directory / L"core_zh_pinyin.tsv", DictionaryCatalogSource::Core,
              &merged, &stats_.core_entries, &stats_.invalid_rows);
    load_file(directory / L"local_core_zh_pinyin.tsv", DictionaryCatalogSource::LocalCore,
              &merged, &stats_.local_core_entries, &stats_.invalid_rows);
    for (auto& [key, entry] : merged) entries_.push_back(std::move(entry));
    stats_.total_unique_entries = entries_.size();
    std::stable_sort(entries_.begin(), entries_.end(), [](const auto& left, const auto& right) {
        if (left.weight != right.weight) return left.weight > right.weight;
        if (left.pinyin != right.pinyin) return left.pinyin < right.pinyin;
        return left.word < right.word;
    });
    return !entries_.empty();
}

DictionaryCatalogStats DictionaryCatalog::count(const std::wstring& resource_directory) {
    // The same rows as load(), only counted: no sorted catalog is built.
    DictionaryCatalogStats stats;
    std::unordered_set<std::wstring> keys;
    keys.reserve(1u << 18);
    const auto collect = [&](const std::wstring& pinyin, const std::wstring& word, int) {
        keys.insert(pinyin + L'\t' + word);
    };
    const std::filesystem::path directory(resource_directory);
    read_file(directory / L"standard_zh_pinyin.tsv", &stats.standard_entries, &stats.invalid_rows, collect);
    read_file(directory / L"core_zh_pinyin.tsv", &stats.core_entries, &stats.invalid_rows, collect);
    read_file(directory / L"local_core_zh_pinyin.tsv", &stats.local_core_entries, &stats.invalid_rows, collect);
    stats.total_unique_entries = keys.size();
    return stats;
}

std::vector<DictionaryCatalogEntry> DictionaryCatalog::preview(std::wstring_view query, size_t maximum) const {
    const size_t limit = std::min<size_t>(maximum, query.empty() ? 200 : 500);
    std::vector<DictionaryCatalogEntry> result;
    for (const auto& entry : entries_) {
        if (!query.empty() && entry.pinyin.find(query) == std::wstring::npos &&
            entry.word.find(query) == std::wstring::npos) continue;
        result.push_back(entry);
        if (result.size() == limit) break;
    }
    return result;
}

std::wstring dictionary_catalog_source_text(DictionaryCatalogSource source) {
    switch (source) {
        case DictionaryCatalogSource::Core: return L"项目基础";
        case DictionaryCatalogSource::LocalCore: return L"项目扩展";
        case DictionaryCatalogSource::Standard: return L"开源基础";
    }
    return L"词库";
}

}  // namespace localpinyin
