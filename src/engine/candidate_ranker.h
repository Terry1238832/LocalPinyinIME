#pragma once

#include "candidate.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace localpinyin {
class UserLearning;

class CandidateRanker {
public:
    void set_frequency(const std::wstring& pinyin, const std::wstring& word, int frequency);
    void increment_frequency(const std::wstring& pinyin, const std::wstring& word);
    void apply_learning(const std::wstring& pinyin, std::vector<Candidate>& candidates,
                        const UserLearning* learning = nullptr) const;
    // `before`: the character before the input; 0 without preceding text, a
    // space after text that does not end in a Han character. A word chosen right
    // after that same character is placed by that choice. Otherwise, after
    // text, a learned single character keeps only its score bonus instead
    // of being placed first: one choice of 她 or 种 says little about the
    // next sentence, where the preceding text decides (他说, 中间).
    [[nodiscard]] std::vector<Candidate> rank(const std::wstring& pinyin, std::vector<Candidate> candidates,
                                             const UserLearning* learning = nullptr, wchar_t before = 0) const;

private:
    std::unordered_map<std::wstring, int> frequency_;
};

}  // namespace localpinyin
