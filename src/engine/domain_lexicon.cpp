#include "domain_lexicon.h"

#include "input_options.h"
#include "pinyin_spelling.h"
#include "../common/utf_utils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace localpinyin {

std::vector<std::pair<std::wstring, std::wstring>> DomainLexicon::read_pack(const std::wstring& path) {
    std::vector<std::pair<std::wstring, std::wstring>> result;
    std::ifstream file{std::filesystem::path(path), std::ios::binary};
    std::string line;
    while (file && std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
        if (line.empty() || line[0] == '#') continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        std::wstring pinyin = utf8_to_wide(line.substr(0, tab));
        std::wstring word = utf8_to_wide(line.substr(tab + 1));
        if (pinyin.empty() || word.empty() ||
            !std::all_of(pinyin.begin(), pinyin.end(), [](wchar_t ch) { return ch >= L'a' && ch <= L'z'; })) continue;
        spell_ue(pinyin);
        result.emplace_back(std::move(pinyin), std::move(word));
    }
    return result;
}

bool DomainLexicon::load(const std::wstring& directory, unsigned mask) {
    mask_ = mask;
    words_.clear();
    for (unsigned bit = 1; bit & kDomainAll; bit <<= 1) {
        if (!(mask & bit)) continue;
        const auto path = std::filesystem::path(directory) / (std::wstring(usage_domain_id(static_cast<UsageDomain>(bit))) + L".tsv");
        for (auto& [pinyin, word] : read_pack(path.wstring())) {
            auto& words = words_[pinyin];
            if (std::find(words.begin(), words.end(), word) == words.end()) words.push_back(std::move(word));
        }
    }
    return !words_.empty();
}

void DomainLexicon::apply(const std::wstring& pinyin, std::vector<Candidate>& candidates) const {
    if (words_.empty()) return;
    const auto found = words_.find(pinyin);
    if (found == words_.end()) return;
    for (const auto& word : found->second) {
        auto it = std::find_if(candidates.begin(), candidates.end(), [&](const Candidate& candidate) {
            return candidate.text == word;
        });
        const int floor = word.size() == 1 ? kCharacterFrequency : kWordFrequency;
        if (it == candidates.end()) {
            candidates.push_back(Candidate{word, floor, 0});
            it = std::prev(candidates.end());
        } else if (it->is_user_lexicon_entry) {
            continue;
        }
        it->base_score = std::max(it->base_score, floor);
        if (it->decoding_frequency > 0) it->decoding_frequency = std::max(it->decoding_frequency, floor);
        it->domain = true;
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.base_score > right.base_score;
    });
}

}  // namespace localpinyin
