#include "pinyin_engine.h"

#include <algorithm>
#include <limits>
#include <filesystem>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace localpinyin {
namespace {

constexpr int kExactMatchBonus = 100000;
constexpr int kCompleteSegmentationBonus = 2000;
constexpr int kFallbackScore = 1;
constexpr size_t kSentenceBeamWidth = 32;
constexpr size_t kMaxInputLength = 256;

struct ParsedInput {
    std::wstring pinyin;
    std::vector<size_t> raw_ends;
    std::vector<size_t> boundaries;
};

ParsedInput parse_input(const std::wstring& raw) {
    ParsedInput result;
    for (size_t index = 0; index < raw.size(); ++index) {
        wchar_t ch = raw[index];
        if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
        if ((ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9')) {
            result.pinyin.push_back(ch);
            result.raw_ends.push_back(index + 1);
        } else if (ch == L'\'' && !result.pinyin.empty()) {
            result.boundaries.push_back(result.pinyin.size());
        }
    }
    return result;
}

size_t raw_prefix_end(const ParsedInput& input, const std::wstring& raw, size_t normalized_end) {
    size_t end = input.raw_ends[normalized_end - 1];
    while (end < raw.size() && raw[end] == L'\'') ++end;
    return end;
}

int lexical_score(const Candidate& candidate) {
    int word_prior = candidate.text.size() >= 2 && candidate.text.size() <= 4 ? 350 : 0;
    // A deliberately frequent project/user expression is useful evidence, even
    // when it spans more than a conventional word. Keep the preference modest:
    // every character still receives the same context score as any other path.
    if (candidate.text.size() >= 5 && candidate.text.size() <= 8 && candidate.base_score >= 8000) word_prior = 2000;
    const int frequency = std::clamp(candidate.decoding_frequency > 0 ? candidate.decoding_frequency : candidate.base_score, 0, 20000);
    // Project frequencies encode a deliberate common-word preference. Keep
    // that evidence meaningful beside ngram scores while limiting each token.
    const int frequency_prior = (frequency - 9000) / 4;
    return static_cast<int>(std::log1p(static_cast<double>(frequency)) * 150) - 1450 + word_prior + frequency_prior;
}

bool can_compose_token(const Candidate& candidate, bool whole_key_match) {
    // Imported Latin names have spelling keys, which can also be valid pinyin
    // syllables. Keep explicit whole-name lookup and personal choices, while
    // avoiding accidental Latin fragments in automatically composed Chinese.
    return whole_key_match || candidate.is_user_lexicon_entry || candidate.user_frequency > 0 ||
           std::none_of(candidate.text.begin(), candidate.text.end(), [](wchar_t ch) {
               return (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z');
           });
}

std::wstring transition_history(const std::wstring& preceding, const std::wstring& text) {
    if (text.size() >= 3) return text.substr(text.size() - 3);
    const auto joined = preceding.substr(preceding.size() > 3 ? preceding.size() - 3 : 0) + text;
    return joined.substr(joined.size() > 3 ? joined.size() - 3 : 0);
}

std::wstring normalize_input(const std::wstring& input) {
    std::wstring normalized;
    normalized.reserve(input.size());
    for (wchar_t ch : input) {
        if (ch >= L'A' && ch <= L'Z') {
            normalized.push_back(static_cast<wchar_t>(ch - L'A' + L'a'));
        } else if (ch >= L'a' && ch <= L'z') {
            normalized.push_back(ch);
        } else if (ch >= L'0' && ch <= L'9') {
            normalized.push_back(ch);
        }
    }
    return normalized;
}

struct SegmentPath {
    int score = 0;
    int segment_count = 0;
    std::wstring text;
};

void prune_paths(std::vector<SegmentPath>& paths) {
    std::stable_sort(paths.begin(), paths.end(), [](const auto& left, const auto& right) {
        return left.score != right.score ? left.score > right.score : left.segment_count < right.segment_count;
    });
    std::vector<SegmentPath> kept;
    std::unordered_map<std::wstring, size_t> state_counts;
    for (auto& path : paths) {
        const auto tail = path.text.substr(path.text.size() > 3 ? path.text.size() - 3 : 0);
        // Paths with the same three final characters have identical future
        // 4gram transitions; preserve different states before widening ties.
        if (state_counts[tail]++ >= 2) continue;
        kept.push_back(std::move(path));
        if (kept.size() == kSentenceBeamWidth) break;
    }
    paths = std::move(kept);
}

void add_path(std::vector<SegmentPath>& paths, std::unordered_map<std::wstring, size_t>& indices,
              SegmentPath proposed) {
    const auto existing = indices.find(proposed.text);
    if (existing == indices.end()) {
        indices.emplace(proposed.text, paths.size());
        paths.push_back(std::move(proposed));
    } else if (proposed.score > paths[existing->second].score) {
        paths[existing->second] = std::move(proposed);
    }
}

std::vector<std::vector<SegmentPath>> decode(const Dictionary& dictionary, const CandidateRanker& ranker,
                                           const UserLearning& learning, const ContextModel& context,
                                           const ParsedInput& input, const std::wstring& preceding) {
    const auto& pinyin = input.pinyin;
    std::vector<std::vector<SegmentPath>> dp(pinyin.size() + 1);
    dp[0].push_back({});
    std::vector<int> split_penalties(pinyin.size() + 1);
    for (size_t start = 0; start < pinyin.size(); ++start) {
        for (size_t length = 2; length <= 6 && start + length <= pinyin.size(); ++length) {
            if (!Dictionary::is_syllable(pinyin.substr(start, length))) continue;
            for (size_t boundary = start + 1; boundary < start + length; ++boundary) split_penalties[boundary] = 1800;
        }
    }
    for (size_t boundary : input.boundaries) split_penalties[boundary] = 0;

    for (size_t offset = 0; offset < pinyin.size(); ++offset) {
        if (dp[offset].empty()) {
            continue;
        }
        prune_paths(dp[offset]);

        for (const auto& part_pinyin : dictionary.matching_pinyins_at(pinyin, offset)) {
            auto part_candidates = dictionary.lookup(part_pinyin);
            ranker.apply_learning(part_pinyin, part_candidates, &learning);
            if (part_candidates.empty()) {
                continue;
            }

            const size_t next = offset + part_pinyin.size();
            auto& paths = dp[next];
            std::vector<size_t> boundaries;
            for (size_t boundary : input.boundaries) {
                if (boundary > offset && boundary < next) boundaries.push_back(boundary - offset);
            }
            struct PreparedCandidate { Candidate candidate; int prior; };
            std::vector<PreparedCandidate> prepared;
            prepared.reserve(part_candidates.size());
            for (auto& candidate : part_candidates) {
                if (!can_compose_token(candidate, offset == 0 && next == pinyin.size())) continue;
                if (!dictionary.reading_matches_boundaries(part_pinyin, candidate.text, boundaries)) continue;
                const int prior = lexical_score(candidate) + candidate.user_frequency / 3;
                prepared.push_back({std::move(candidate), prior});
            }
            std::stable_sort(prepared.begin(), prepared.end(), [](const auto& left, const auto& right) {
                return left.prior > right.prior;
            });
            if (prepared.size() > 32) prepared.resize(32);
            std::unordered_map<std::wstring, size_t> indices;
            indices.reserve(paths.size() + dp[offset].size() * prepared.size());
            for (size_t index = 0; index < paths.size(); ++index) indices.emplace(paths[index].text, index);
            for (const auto& path : dp[offset]) {
                const auto history = transition_history(preceding, path.text);
                for (const auto& item : prepared) {
                    const int score = path.score + item.prior - split_penalties[next] + context.extend_score(history, item.candidate.text);
                    add_path(paths, indices, SegmentPath{score, path.segment_count + 1, path.text + item.candidate.text});
                }
            }
            if (paths.size() > kSentenceBeamWidth * 2) prune_paths(paths);
        }
    }
    prune_paths(dp.back());
    return dp;
}

Candidate candidate_from_path(const SegmentPath& path, const ContextModel& context) {
    Candidate candidate;
    candidate.base_score = path.score + context.finish_score(path.text);
    candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
    candidate.text = path.text;
    return candidate;
}

std::vector<Candidate> typo_candidates(const Dictionary& dictionary, const std::wstring& input) {
    if (input.size() < 4 || input.size() > 32) return {};
    std::vector<Candidate> result;
    std::unordered_set<std::wstring> probed;
    const auto probe = [&](const std::wstring& key) {
        if (result.size() >= 96 || !probed.insert(key).second) return;
        auto matches = dictionary.lookup(key);
        for (size_t index = 0; index < std::min<size_t>(3, matches.size()); ++index) {
            matches[index].segmentation_penalty = 4000;
            result.push_back(std::move(matches[index]));
        }
    };
    for (size_t offset = 0; offset < input.size(); ++offset) {
        auto edited = input;
        edited.erase(offset, 1);
        probe(edited);
        if (offset + 1 < input.size()) {
            edited = input;
            std::swap(edited[offset], edited[offset + 1]);
            probe(edited);
        }
        for (wchar_t letter = L'a'; letter <= L'z'; ++letter) {
            edited = input;
            edited[offset] = letter;
            probe(edited);
        }
    }
    for (size_t offset = 0; offset <= input.size(); ++offset) {
        for (wchar_t letter = L'a'; letter <= L'z'; ++letter) {
            auto edited = input;
            edited.insert(offset, 1, letter);
            probe(edited);
        }
    }
    return result;
}

std::vector<Candidate> deduplicate_and_limit(std::vector<Candidate> ranked) {
    std::vector<Candidate> result;
    std::unordered_set<std::wstring> seen;
    for (const auto& candidate : ranked) {
        if (candidate.text.empty()) {
            continue;
        }
        if (!seen.insert(candidate.text).second) {
            continue;
        }
        result.push_back(candidate);
        if (result.size() >= kMaxCandidateCount) {
            break;
        }
    }
    return result;
}

}  // namespace

PinyinEngine::PinyinEngine() {
    open_learning(Dictionary::default_user_lexicon_path());
    rebuild_context_model((std::filesystem::path(Dictionary::default_resource_path()).parent_path() / L"context_zh.tsv").wstring());
}

PinyinEngine::PinyinEngine(DictionaryLoadMode dictionary_load_mode)
    : dictionary_(dictionary_load_mode) {
    open_learning(Dictionary::default_user_lexicon_path());
    rebuild_context_model(dictionary_load_mode == DictionaryLoadMode::Empty ? L"" :
                          (std::filesystem::path(Dictionary::default_resource_path()).parent_path() / L"context_zh.tsv").wstring());
}

void PinyinEngine::rebuild_context_model(const std::wstring& context_path) {
    context_path_ = context_path;
    context_model_.reset(dictionary_);
    if (!context_path.empty()) context_model_.load(context_path);
    continuation_model_.reset(dictionary_);
    if (!context_path.empty()) continuation_model_.load((std::filesystem::path(context_path).parent_path() / L"continuations_zh.tsv").wstring());
    learning_revision_ = static_cast<size_t>(-1);
}

bool PinyinEngine::load_context_model(const std::wstring& path) {
    context_path_ = path;
    context_model_.reset(dictionary_);
    learning_revision_ = static_cast<size_t>(-1);
    return context_model_.load(path);
}

bool PinyinEngine::load_continuation_model(const std::wstring& path) {
    return continuation_model_.load(path);
}

std::vector<Candidate> PinyinEngine::predict(const std::wstring& preceding_text, size_t limit) const {
    learning_.refresh_if_changed();
    if (learning_revision_ != learning_.revision()) {
        const auto selections = learning_.selected_texts();
        context_model_.set_learning(selections);
        continuation_model_.set_learning(selections);
        learning_revision_ = learning_.revision();
    }
    return continuation_model_.predict(preceding_text, limit);
}

void PinyinEngine::open_learning(const std::wstring& user_lexicon_path) {
    if (user_lexicon_path.empty()) return;
    const auto parent = std::filesystem::path(user_lexicon_path).parent_path();
    learning_.open((parent / L"user_learning.tsv").wstring());
}

bool PinyinEngine::load_dictionary(const std::wstring& path) {
    dictionary_.clear();
    const auto extension = path.substr(path.find_last_of(L'.') == std::wstring::npos ? path.size() : path.find_last_of(L'.'));
    const bool loaded = extension == L".tsv" || extension == L".TSV" ?
                        dictionary_.load_from_tsv_file(path) : dictionary_.load_from_json_file(path);
    rebuild_context_model((std::filesystem::path(path).parent_path() / L"context_zh.tsv").wstring());
    return loaded;
}

bool PinyinEngine::load_dictionary_resource_directory(const std::wstring& directory,
                                                      const std::wstring& user_lexicon_path,
                                                      UserLexiconCreateMode create_mode) {
    dictionary_.clear();
    const auto user_path = user_lexicon_path.empty() ? Dictionary::default_user_lexicon_path() : user_lexicon_path;
    open_learning(user_path);
    const auto resources = std::filesystem::path(directory) / L"dictionary";
    const bool loaded = dictionary_.load_from_layered_files(
        (resources / L"core_zh_pinyin.tsv").wstring(),
        (resources / L"local_core_zh_pinyin.tsv").wstring(), user_path, create_mode,
        (resources / L"standard_zh_pinyin.tsv").wstring());
    rebuild_context_model((std::filesystem::path(directory) / L"dictionary" / L"context_zh.tsv").wstring());
    return loaded;
}

std::vector<Candidate> PinyinEngine::lookup(const std::wstring& pinyin, const std::wstring& preceding_text) const {
    const auto context = ContinuationModel::context_suffix(preceding_text);
    const auto parsed = parse_input(pinyin);
    const auto& normalized = parsed.pinyin;
    if (normalized.empty()) {
        return {};
    }
    if (normalized.size() > kMaxInputLength) return {Candidate{normalized, kFallbackScore, 0}};
    learning_.refresh_if_changed();
    if (learning_revision_ != learning_.revision()) {
        const auto selections = learning_.selected_texts();
        context_model_.set_learning(selections);
        continuation_model_.set_learning(selections);
        learning_revision_ = learning_.revision();
    }

    auto dp = decode(dictionary_, ranker_, learning_, context_model_, parsed, context);
    std::vector<Candidate> candidates;
    for (const auto& path : dp.back()) candidates.push_back(candidate_from_path(path, context_model_));
    // A standalone short word keeps its familiar frequency order. An explicit
    // personal phrase at the recommended common-word frequency also preserves
    // a complete key match, even if sentence decoding pruned that whole phrase.
    // Lower-frequency personal phrases and system sentences keep their normal
    // contextual ranking.
    for (auto candidate : dictionary_.lookup(normalized)) {
        const bool standalone_word = context.empty() && candidate.text.size() <= 4;
        const bool preferred_personal_phrase = candidate.is_user_lexicon_entry && candidate.text.size() > 4 &&
                                               candidate.base_score >= kPreferredUserLexiconFrequency;
        if ((!standalone_word && !preferred_personal_phrase) ||
            !dictionary_.reading_matches_boundaries(normalized, candidate.text, parsed.boundaries)) continue;
        candidate.exact_match_bonus = kExactMatchBonus;
        candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
        candidates.push_back(std::move(candidate));
    }

    // Complete an unfinished final syllable in its sentence context. This also
    // preserves a fully spelled sentence: xi is not extended to xiao and ji
    // is not extended to jian when the complete input already has a reading.
    for (size_t offset = normalized.size() > 12 ? normalized.size() - 12 : 0;
         dp.back().empty() && offset < normalized.size(); ++offset) {
        if (dp[offset].empty()) continue;
        const auto suffix = normalized.substr(offset);
        auto completions = ranker_.rank(suffix, dictionary_.lookup_completion(suffix, true), &learning_);
        completions.erase(std::remove_if(completions.begin(), completions.end(), [&](const auto& word) {
            return !can_compose_token(word, offset == 0);
        }), completions.end());
        if (completions.empty()) continue;
        for (const auto& path : dp[offset]) {
            for (size_t index = 0; index < std::min<size_t>(12, completions.size()); ++index) {
                const auto& word = completions[index];
                SegmentPath completed{path.score + lexical_score(word) + word.user_frequency / 3 +
                                      context_model_.extend_score(transition_history(context, path.text), word.text) - 600,
                                      path.segment_count + 1, path.text + word.text};
                candidates.push_back(candidate_from_path(completed, context_model_));
            }
        }
        if (candidates.size() > 256) candidates = deduplicate_and_limit(ranker_.rank(normalized, std::move(candidates), &learning_));
    }

    if (candidates.empty()) candidates = dictionary_.lookup_abbreviation(normalized);
    if (candidates.empty()) candidates = typo_candidates(dictionary_, normalized);
    auto ranked = deduplicate_and_limit(ranker_.rank(normalized, std::move(candidates), &learning_));

    // Keep one explicit complete long word available beside sentence decoding.
    // It remains an optional candidate: do not replace the contextual first
    // choice, and leave a ninth slot for selecting a shorter prefix.
    auto complete_words = dictionary_.lookup(normalized);
    complete_words.erase(std::remove_if(complete_words.begin(), complete_words.end(), [&](const auto& word) {
        return word.text.size() <= 4 || word.text.size() > 16 || word.base_score < kPreferredUserLexiconFrequency ||
               !dictionary_.reading_matches_boundaries(normalized, word.text, parsed.boundaries);
    }), complete_words.end());
    complete_words = ranker_.rank(normalized, std::move(complete_words), &learning_);
    for (auto& word : complete_words) {
        if (std::any_of(ranked.begin(), ranked.end(), [&](const auto& item) { return item.text == word.text; })) continue;
        if (ranked.size() >= kMaxCandidateCount - 1) ranked.resize(kMaxCandidateCount - 2);
        ranked.push_back(std::move(word));
        break;
    }

    std::vector<Candidate> prefixes;
    if (ranked.empty()) {
        // A missing dictionary item must not discard an otherwise useful
        // converted prefix. The user can commit it and continue with the tail.
        for (size_t end = normalized.size() - 1; end > 0; --end) {
            if (dp[end].empty()) continue;
            for (const auto& path : dp[end]) {
                auto candidate = candidate_from_path(path, context_model_);
                candidate.consumed_pinyin = raw_prefix_end(parsed, pinyin, end);
                prefixes.push_back(std::move(candidate));
            }
            prefixes = ranker_.rank(normalized.substr(0, end), std::move(prefixes), &learning_);
            break;
        }
    } else {
        // Reserve a choice for a first word/phrase even while a full sentence
        // is available, enabling the normal select-prefix-and-continue workflow.
        for (const auto& key : dictionary_.matching_pinyins_at(normalized, 0)) {
            if (key.size() >= normalized.size() || key.size() > 12) continue;
            auto words = ranker_.rank(key, dictionary_.lookup(key), &learning_);
            std::vector<size_t> boundaries;
            for (size_t boundary : parsed.boundaries) {
                if (boundary < key.size()) boundaries.push_back(boundary);
            }
            for (auto& word : words) {
                if (word.text.size() > 4 || !dictionary_.reading_matches_boundaries(key, word.text, boundaries)) continue;
                word.consumed_pinyin = raw_prefix_end(parsed, pinyin, key.size());
                prefixes.push_back(std::move(word));
            }
            if (!prefixes.empty()) break;
        }
    }
    if (!prefixes.empty()) {
        if (ranked.size() >= kMaxCandidateCount) {
            // An eligible complete word may already be ranked ninth. The
            // earlier retention pass then leaves it in place; protect it here
            // before its slot is reassigned to a shorter prefix. Preserve its
            // original score/commit metadata and the first seven choices.
            if (std::any_of(complete_words.begin(), complete_words.end(), [&](const auto& word) {
                    return word.text == ranked[kMaxCandidateCount - 1].text;
                })) {
                ranked[kMaxCandidateCount - 2] = ranked[kMaxCandidateCount - 1];
            }
            ranked.resize(kMaxCandidateCount - 1);
        }
        std::unordered_set<std::wstring> seen;
        for (const auto& candidate : ranked) seen.insert(candidate.text);
        for (auto& candidate : prefixes) {
            if (!seen.insert(candidate.text).second) continue;
            ranked.push_back(std::move(candidate));
            if (ranked.size() >= kMaxCandidateCount) break;
        }
    }
    if (ranked.empty()) ranked.push_back(Candidate{normalized, kFallbackScore, 0});
    return ranked;
}

UserLexiconRefreshResult PinyinEngine::refresh_user_lexicon_if_needed(bool composition_active) {
    auto result = dictionary_.refresh_user_lexicon_if_changed(composition_active);
    if (result.reloaded) rebuild_context_model(context_path_);
    return result;
}

void PinyinEngine::learn(const std::wstring& pinyin, const std::wstring& selected_word) {
    const auto normalized = normalize_input(pinyin);
    if (normalized.empty() || normalized == selected_word) return;
    if (!learning_.record_selection(normalized, selected_word)) ranker_.increment_frequency(normalized, selected_word);
}

}  // namespace localpinyin
