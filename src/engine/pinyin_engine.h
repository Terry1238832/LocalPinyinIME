#pragma once

#include "candidate.h"
#include "candidate_ranker.h"
#include "dictionary.h"

#include <string>
#include <vector>

namespace localpinyin {

class PinyinEngine {
public:
    PinyinEngine();
    explicit PinyinEngine(DictionaryLoadMode dictionary_load_mode);

    bool load_dictionary(const std::wstring& path);
    [[nodiscard]] std::vector<Candidate> lookup(const std::wstring& pinyin) const;
    void learn(const std::wstring& pinyin, const std::wstring& selected_word);

private:
    Dictionary dictionary_;
    CandidateRanker ranker_;
};

}  // namespace localpinyin
