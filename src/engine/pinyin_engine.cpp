#include "pinyin_engine.h"

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace localpinyin {
namespace {

constexpr int kExactMatchBonus = 100000;
constexpr int kCompleteSegmentationBonus = 2000;
constexpr int kExcessiveSegmentationPenalty = 5000;
constexpr int kFallbackScore = 1;

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

struct SegmentPart {
    std::wstring pinyin;
    Candidate candidate;
};

struct SegmentPath {
    bool reachable = false;
    int score = std::numeric_limits<int>::min();
    int segment_count = 0;
    std::vector<SegmentPart> parts;
};

bool is_better_path(int new_score, int new_segment_count, const SegmentPath& current) {
    if (!current.reachable) {
        return true;
    }
    if (new_score != current.score) {
        return new_score > current.score;
    }
    return new_segment_count < current.segment_count;
}

SegmentPath best_segmentation(const Dictionary& dictionary, const std::wstring& pinyin) {
    std::vector<SegmentPath> dp(pinyin.size() + 1);
    dp[0].reachable = true;
    dp[0].score = 0;

    for (size_t offset = 0; offset < pinyin.size(); ++offset) {
        if (!dp[offset].reachable) {
            continue;
        }

        for (const auto& part_pinyin : dictionary.matching_pinyins_at(pinyin, offset)) {
            const auto part_candidates = dictionary.lookup(part_pinyin);
            if (part_candidates.empty()) {
                continue;
            }

            const size_t next = offset + part_pinyin.size();
            const int new_score = dp[offset].score + part_candidates.front().base_score;
            const int new_segment_count = dp[offset].segment_count + 1;
            if (!is_better_path(new_score, new_segment_count, dp[next])) {
                continue;
            }

            dp[next] = dp[offset];
            dp[next].reachable = true;
            dp[next].score = new_score;
            dp[next].segment_count = new_segment_count;
            dp[next].parts.push_back(SegmentPart{part_pinyin, part_candidates.front()});
        }
    }

    return dp.back();
}

Candidate candidate_from_path(const SegmentPath& path) {
    Candidate candidate;
    candidate.base_score = path.score;
    candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
    candidate.segmentation_penalty = std::max(0, path.segment_count - 1) * kExcessiveSegmentationPenalty;
    for (const auto& part : path.parts) {
        candidate.text += part.candidate.text;
    }
    return candidate;
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

PinyinEngine::PinyinEngine() = default;

PinyinEngine::PinyinEngine(DictionaryLoadMode dictionary_load_mode)
    : dictionary_(dictionary_load_mode) {}

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
    return dictionary_.load_from_resource_directory(directory, user_lexicon_path);
}

std::vector<Candidate> PinyinEngine::lookup(const std::wstring& pinyin) const {
    const std::wstring normalized = normalize_input(pinyin);
    if (normalized.empty()) {
        return {};
    }

    std::vector<Candidate> candidates = dictionary_.lookup(normalized);
    for (auto& candidate : candidates) {
        candidate.exact_match_bonus = kExactMatchBonus;
        candidate.complete_segmentation_bonus = kCompleteSegmentationBonus;
    }

    const SegmentPath segmented = best_segmentation(dictionary_, normalized);
    if (segmented.reachable && segmented.segment_count > 1) {
        candidates.push_back(candidate_from_path(segmented));
    }

    if (candidates.empty()) {
        candidates.push_back(Candidate{normalized, kFallbackScore, 0});
    }

    return deduplicate_and_limit(ranker_.rank(normalized, std::move(candidates)));
}

UserLexiconRefreshResult PinyinEngine::refresh_user_lexicon_if_needed(bool composition_active) {
    return dictionary_.refresh_user_lexicon_if_changed(composition_active);
}

void PinyinEngine::learn(const std::wstring& pinyin, const std::wstring& selected_word) {
    ranker_.increment_frequency(normalize_input(pinyin), selected_word);
}

}  // namespace localpinyin
