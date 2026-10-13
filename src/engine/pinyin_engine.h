#pragma once

#include "candidate.h"
#include "candidate_ranker.h"
#include "dictionary.h"
#include "user_learning.h"
#include "context_model.h"
#include "continuation_model.h"

#include <string>
#include <vector>

namespace localpinyin {

class PinyinEngine {
public:
    PinyinEngine();
    explicit PinyinEngine(DictionaryLoadMode dictionary_load_mode);

    bool load_dictionary(const std::wstring& path);
    bool load_dictionary_resource_directory(const std::wstring& directory,
                                            const std::wstring& user_lexicon_path = L"",
                                            UserLexiconCreateMode create_mode = UserLexiconCreateMode::CreateIfMissing);
    bool load_context_model(const std::wstring& path);
    bool load_continuation_model(const std::wstring& path);
    [[nodiscard]] std::vector<Candidate> predict(const std::wstring& preceding_text,
                                               size_t limit = kMaxCandidateCount) const;
    [[nodiscard]] std::vector<Candidate> lookup(const std::wstring& pinyin,
                                               const std::wstring& preceding_text = L"") const;
    UserLexiconRefreshResult refresh_user_lexicon_if_needed(bool composition_active);
    void learn(const std::wstring& pinyin, const std::wstring& selected_word);

private:
    void open_learning(const std::wstring& user_lexicon_path);
    void rebuild_context_model(const std::wstring& context_path);
    Dictionary dictionary_;
    CandidateRanker ranker_;
    mutable UserLearning learning_;
    mutable ContextModel context_model_;
    mutable ContinuationModel continuation_model_;
    mutable size_t learning_revision_ = static_cast<size_t>(-1);
    std::wstring context_path_;
};

}  // namespace localpinyin
