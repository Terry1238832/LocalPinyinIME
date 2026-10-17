#pragma once

#include "candidate.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace localpinyin {

// Words of the usage scenes (常用场景) the user chose, from
// dictionary/domains/<id>.tsv (pinyin, word). The dictionary itself is left
// untouched: lookups apply the chosen words on top, so choosing or dropping
// a scene takes effect at once and never disturbs personal lexicon overlays.
class DomainLexicon {
public:
    // A scene word ranks at least this high, as a direct candidate and as a
    // sentence token; a single character a little lower, so a particle such
    // as 嘛 still yields to a far more common homophone (吗).
    static constexpr int kWordFrequency = 9000;
    static constexpr int kCharacterFrequency = 8000;

    // Loads the packs of the scenes in `mask` (UsageDomain bits) from
    // `directory`/<id>.tsv. A missing pack is skipped. Returns false when no
    // chosen scene has a pack.
    bool load(const std::wstring& directory, unsigned mask);
    [[nodiscard]] unsigned mask() const noexcept { return mask_; }
    [[nodiscard]] bool empty() const noexcept { return words_.empty(); }
    // Raises the chosen words among `candidates` for `pinyin` (adding those
    // the dictionary lacks) and marks them as scene words. Explicit personal
    // lexicon scores are kept.
    void apply(const std::wstring& pinyin, std::vector<Candidate>& candidates) const;
    // The words of one pack file, in file order, for display.
    static std::vector<std::pair<std::wstring, std::wstring>> read_pack(const std::wstring& path);

private:
    unsigned mask_ = 0;
    std::unordered_map<std::wstring, std::vector<std::wstring>> words_;
};

}  // namespace localpinyin
