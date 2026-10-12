#pragma once

#include "candidate.h"
#include "candidate_ranker.h"
#include "dictionary.h"
#include "user_learning.h"

#include <string>
#include <vector>

namespace localpinyin {

class PinyinEngine {
public:
    PinyinEngine();
    explicit PinyinEngine(DictionaryLoadMode dictionary_load_mode);

    bool load_dictionary(const std::wstring& path);
    bool load_dictionary_resource_directory(const std::wstring& directory,
                                            const std::wstring& user_lexicon_path = L"");
    [[nodiscard]] std::vector<Candidate> lookup(const std::wstring& pinyin) const;
    UserLexiconRefreshResult refresh_user_lexicon_if_needed(bool composition_active);
    void learn(const std::wstring& pinyin, const std::wstring& selected_word);

private:
    void open_learning(const std::wstring& user_lexicon_path);
    Dictionary dictionary_;
    CandidateRanker ranker_;
    mutable UserLearning learning_;
};

}  // namespace localpinyin
