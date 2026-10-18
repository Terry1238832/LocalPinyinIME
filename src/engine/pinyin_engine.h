#pragma once

#include "candidate.h"
#include "candidate_ranker.h"
#include "dictionary.h"
#include "user_learning.h"
#include "context_model.h"
#include "continuation_model.h"
#include "domain_lexicon.h"
#include "english_terms.h"
#include "input_options.h"

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
    // English words and abbreviations for Chinese-English mixed input.
    bool load_english_terms(const std::wstring& path);
    // Calibration only (LocalPinyinImeEval --weights): replaces the sentence
    // decoding weights for every engine in the process. The IME never calls it.
    static bool set_evaluation_weights(const std::wstring& spec);
    // Fuzzy pinyin, spelling correction, mixed input, double pinyin and the
    // usage scenes; a changed choice of scenes loads their word packs.
    void set_input_options(const EngineInputOptions& options) noexcept;
    [[nodiscard]] const EngineInputOptions& input_options() const noexcept { return options_; }
    [[nodiscard]] std::vector<Candidate> predict(const std::wstring& preceding_text,
                                               size_t limit = kMaxCandidateCount) const;
    // Candidates for the typed keys (full pinyin, or double pinyin keys when
    // that scheme is on). A prefix candidate's consumed_pinyin counts typed
    // characters, whatever the scheme.
    [[nodiscard]] std::vector<Candidate> lookup(const std::wstring& typed,
                                               const std::wstring& preceding_text = L"") const;
    UserLexiconRefreshResult refresh_user_lexicon_if_needed(bool composition_active);
    // Typed pinyin with syllable separators for display, e.g. nihaoshij ->
    // ni'hao'shi'j. Typed apostrophes are kept; unparseable text is unchanged.
    [[nodiscard]] static std::wstring display_pinyin(const std::wstring& typed);
    // The typed keys as the candidate window shows them under the current
    // options: double pinyin as full pinyin, English terms set apart by spaces
    // (wo'yong python xie'dai'ma).
    [[nodiscard]] std::wstring display_input(const std::wstring& typed) const;
    void learn(const std::wstring& typed, const std::wstring& selected_word);
    // 造词: a phrase the user built by choosing its pieces one after another
    // (量 + 甲 + 成 for liang, jia, cheng) is remembered for the whole typed
    // pinyin, unless the engine would already offer it first there. Returns
    // whether it was learned.
    bool learn_phrase(const std::wstring& typed, const std::wstring& phrase, const std::wstring& preceding_text = L"");

private:
    void open_learning(const std::wstring& user_lexicon_path);
    void rebuild_context_model(const std::wstring& context_path);
    void refresh_learning() const;
    [[nodiscard]] std::vector<Candidate> lookup_pinyin(const std::wstring& pinyin, const std::wstring& preceding_text,
                                                      const EngineInputOptions& options) const;
    // Dictionary words for a complete key, with the chosen scenes applied.
    [[nodiscard]] std::vector<Candidate> words_for(const std::wstring& pinyin) const;
    // The typed keys as full pinyin (the same text unless double pinyin is on).
    [[nodiscard]] std::wstring full_pinyin(const std::wstring& typed) const;
    Dictionary dictionary_;
    CandidateRanker ranker_;
    mutable UserLearning learning_;
    mutable ContextModel context_model_;
    mutable ContinuationModel continuation_model_;
    mutable size_t learning_revision_ = static_cast<size_t>(-1);
    std::wstring context_path_;
    EnglishTerms english_;
    EngineInputOptions options_;
    // dictionary/domains next to the loaded dictionary.
    std::wstring domain_directory_;
    DomainLexicon domains_;
};

}  // namespace localpinyin
