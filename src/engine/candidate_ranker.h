#pragma once

#include "candidate.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace localpinyin {

class CandidateRanker {
public:
    void set_frequency(const std::wstring& pinyin, const std::wstring& word, int frequency);
    void increment_frequency(const std::wstring& pinyin, const std::wstring& word);
    [[nodiscard]] std::vector<Candidate> rank(const std::wstring& pinyin, std::vector<Candidate> candidates) const;

private:
    std::unordered_map<std::wstring, int> frequency_;
};

}  // namespace localpinyin
