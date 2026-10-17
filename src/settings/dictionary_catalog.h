#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace localpinyin {

enum class DictionaryCatalogSource { Core, LocalCore, Standard };

struct DictionaryCatalogEntry {
    std::wstring pinyin;
    std::wstring word;
    int weight = 0;
    DictionaryCatalogSource source = DictionaryCatalogSource::Core;
};

struct DictionaryCatalogStats {
    size_t core_entries = 0;
    size_t local_core_entries = 0;
    size_t standard_entries = 0;
    size_t total_unique_entries = 0;
    size_t invalid_rows = 0;
};

class DictionaryCatalog {
public:
    bool load(const std::wstring& resource_directory);
    // The counts load() would report, several times faster: for showing the
    // dictionary's size without building the searchable catalog.
    [[nodiscard]] static DictionaryCatalogStats count(const std::wstring& resource_directory);
    [[nodiscard]] const DictionaryCatalogStats& stats() const noexcept { return stats_; }
    [[nodiscard]] const std::vector<DictionaryCatalogEntry>& entries() const noexcept { return entries_; }
    [[nodiscard]] std::vector<DictionaryCatalogEntry> preview(std::wstring_view query,
                                                             size_t maximum = 500) const;

private:
    std::vector<DictionaryCatalogEntry> entries_;
    DictionaryCatalogStats stats_;
};

[[nodiscard]] std::wstring dictionary_catalog_source_text(DictionaryCatalogSource source);

}  // namespace localpinyin
