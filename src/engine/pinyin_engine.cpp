#include "pinyin_engine.h"

#include <algorithm>
#include <limits>
#include <filesystem>
#include <unordered_set>

namespace localpinyin {
namespace {

constexpr int kExactMatchBonus = 100000;
constexpr int kCompleteSegmentationBonus = 2000;
constexpr int kExcessiveSegmentationPenalty = 9000;
constexpr int kFallbackScore = 1;
constexpr size_t kSentenceBeamWidth = 5;
constexpr size_t kMaxInputLength = 128;

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

std::vector<SegmentPath> best_segmentations(const Dictionary& dictionary, const CandidateRanker& ranker,
                                          const UserLearning& learning, const std::wstring& pinyin) {
    std::vector<std::vector<SegmentPath>> dp(pinyin.size() + 1);
    dp[0].push_back({});

    for (size_t offset = 0; offset < pinyin.size(); ++offset) {
        if (dp[offset].empty()) {
            continue;
        }

        for (const auto& part_pinyin : dictionary.matching_pinyins_at(pinyin, offset)) {
            const auto part_candidates = ranker.rank(part_pinyin, dictionary.lookup(part_pinyin), &learning);
            if (part_candidates.empty()) {
                continue;
            }

            const size_t next = offset + part_pinyin.size();
            auto& paths = dp[next];
            for (const auto& path : dp[offset]) {
                for (size_t index = 0; index < std::min<size_t>(3, part_candidates.size()); ++index) {
                    const auto& candidate = part_candidates[index];
                    // Apply the fragment penalty during decoding so a chain of
                    // individually common characters cannot beat a natural phrase.
                    const int score = path.score + std::min(candidate.base_score, 20000) + candidate.user_frequency -
                                      (path.segment_count > 0 ? kExcessiveSegmentationPenalty : 0);
                    SegmentPath proposed{score, path.segment_count + 1, path.text + candidate.text};
                    const auto existing = std::find_if(paths.begin(), paths.end(), [&](const auto& other) {
                        return other.text == proposed.text;
                    });
                    if (existing == paths.end()) paths.push_back(std::move(proposed));
                    else if (score > existing->score) *existing = std::move(proposed);
                }
            }
            std::stable_sort(paths.begin(), paths.end(), [](const auto& left, const auto& right) {
                return left.score != right.score ? left.score > right.score : left.segment_count < right.segment_count;
            });
            if (paths.size() > kSentenceBeamWidth) paths.resize(kSentenceBeamWidth);
        }
    }

    return std::move(dp.back());
}

Candidate candidate_from_path(const SegmentPath& path) {
    Candidate candidate;
    candidate.base_score = path.score;
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
}

PinyinEngine::PinyinEngine(DictionaryLoadMode dictionary_load_mode)
    : dictionary_(dictionary_load_mode) {
    open_learning(Dictionary::default_user_lexicon_path());
}

void PinyinEngine::open_learning(const std::wstring& user_lexicon_path) {
    if (user_lexicon_path.empty()) return;
    const auto parent = std::filesystem::path(user_lexicon_path).parent_path();
    learning_.open((parent / L"user_learning.tsv").wstring());
}

bool PinyinEngine::load_dictionary(const std::wstring& path) {
    dictionary_.clear();
    const auto extension = path.substr(path.find_last_of(L'.') == std::wstring::npos ? path.size() : path.find_last_of(L'.'));
    if (extension == L".tsv" || extension == L".TSV") {
        return dictionary_.load_from_tsv_file(path);
    }
    return dictionary_.load_from_json_file(path);
}

bool PinyinEngine::load_dictionary_resource_directory(const std::wstring& directory,
                                                      const std::wstring& user_lexicon_path) {
    dictionary_.clear();
    const auto user_path = user_lexicon_path.empty() ? Dictionary::default_user_lexicon_path() : user_lexicon_path;
    open_learning(user_path);
    return dictionary_.load_from_resource_directory(directory, user_path);
}

std::vector<Candidate> PinyinEngine::lookup(const std::wstring& pinyin) const {
    const std::wstring normalized = normalize_input(pinyin);
    if (normalized.empty()) {
        return {};
    }
    if (normalized.size() > kMaxInputLength) return {Candidate{normalized, kFallbackScore, 0}};
    learning_.refresh_if_changed();

    std::vector<Candidate> candidates = dictionary_.lookup(normalized);
    for (auto& candidate : candidates) {
        candidate.exact_match_bonus = kExactMatchBonus;
        candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
    }

    const auto segmented = best_segmentations(dictionary_, ranker_, learning_, normalized);
    for (const auto& path : segmented) {
        if (path.segment_count > 1) candidates.push_back(candidate_from_path(path));
    }

    if (candidates.empty()) candidates = dictionary_.lookup_abbreviation(normalized);
    if (candidates.empty()) candidates = dictionary_.lookup_completion(normalized);
    if (candidates.empty()) candidates = typo_candidates(dictionary_, normalized);
    if (candidates.empty()) {
        candidates.push_back(Candidate{normalized, kFallbackScore, 0});
    }

    return deduplicate_and_limit(ranker_.rank(normalized, std::move(candidates), &learning_));
}

UserLexiconRefreshResult PinyinEngine::refresh_user_lexicon_if_needed(bool composition_active) {
    return dictionary_.refresh_user_lexicon_if_changed(composition_active);
}

void PinyinEngine::learn(const std::wstring& pinyin, const std::wstring& selected_word) {
    const auto normalized = normalize_input(pinyin);
    if (normalized.empty() || normalized == selected_word) return;
    if (!learning_.record_selection(normalized, selected_word)) ranker_.increment_frequency(normalized, selected_word);
}

}  // namespace localpinyin
