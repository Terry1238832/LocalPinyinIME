#include "continuation_model.h"
#include "dictionary.h"
#include "../common/utf_utils.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>

namespace localpinyin {
namespace {
bool han(wchar_t ch) { return ch >= 0x3400 && ch <= 0x9fff; }
bool han_text(const std::wstring& text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), han);
}
}

std::wstring ContinuationModel::context_suffix(const std::wstring& text) {
    size_t start = text.size();
    while (start > 0 && text.size() - start < 32 && han(text[start - 1])) --start;
    return text.substr(start);
}

void ContinuationModel::add(Index& index, const std::wstring& prefix, Entry entry) {
    auto& entries = index[prefix];
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const auto& old) { return old.text == entry.text; });
    if (found == entries.end()) entries.push_back(std::move(entry));
    else if (entry.weight > found->weight || (entry.personal && !found->personal)) *found = std::move(entry);
}

void ContinuationModel::compact(Index& index) {
    for (auto& [prefix, entries] : index) {
        std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
            return left.weight != right.weight ? left.weight > right.weight : left.text < right.text;
        });
        if (entries.size() > 32) entries.resize(32);
    }
}

void ContinuationModel::add_phrase(Index& index, const std::wstring& text, int weight, bool personal) {
    if (text.size() < 3 || text.size() > 16 || !han_text(text)) return;
    for (size_t length = 2; length < text.size() && length <= 8; ++length) {
        add(index, text.substr(0, length), Entry{text.substr(length), std::clamp(weight, 0, 10000), personal});
    }
}

void ContinuationModel::reset(const Dictionary& dictionary) {
    dictionary_.clear();
    rules_.clear();
    learned_.clear();
    rule_count_ = 0;
    for (const auto& [key, entries] : dictionary.entries_) {
        for (const auto& entry : entries) {
            if (entry.base_score >= 5000 || entry.is_user_lexicon_entry)
                add_phrase(dictionary_, entry.text, entry.base_score, entry.is_user_lexicon_entry);
        }
    }
    compact(dictionary_);
}

bool ContinuationModel::load(const std::wstring& path) {
    rules_.clear();
    rule_count_ = 0;
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        if (line.size() > 256 || rule_count_ >= 10000) continue;
        const auto first = line.find('\t');
        const auto second = line.find('\t', first == std::string::npos ? 0 : first + 1);
        if (first == std::string::npos || second == std::string::npos || line.find('\t', second + 1) != std::string::npos) continue;
        const auto prefix = utf8_to_wide(line.substr(0, first));
        const auto next = utf8_to_wide(line.substr(first + 1, second - first - 1));
        int weight = 0;
        const auto parsed = std::from_chars(line.data() + second + 1, line.data() + line.size(), weight);
        if (parsed.ec != std::errc{} || parsed.ptr != line.data() + line.size() || weight < 100 || weight > 10000 ||
            prefix.size() < 2 || prefix.size() > 8 || next.size() > 16 || !han_text(prefix) || !han_text(next)) continue;
        const auto before = rules_[prefix].size();
        add(rules_, prefix, Entry{next, weight, false});
        if (rules_[prefix].size() > before) ++rule_count_;
    }
    compact(rules_);
    return rule_count_ > 0;
}

void ContinuationModel::set_learning(const std::vector<std::pair<std::wstring, int>>& selections) {
    learned_.clear();
    for (const auto& [text, count] : selections) {
        if (count > 0) add_phrase(learned_, text, static_cast<int>(std::min<long long>(10000, 2000LL * count)), true);
    }
    compact(learned_);
}

std::vector<Candidate> ContinuationModel::predict(const std::wstring& preceding_text, size_t limit) const {
    const auto context = context_suffix(preceding_text);
    if (context.size() < 2 || !limit) return {};
    std::unordered_map<std::wstring, Candidate> found;
    auto append = [&](const Index& index, const std::wstring& prefix, int source_bonus) {
        const auto bucket = index.find(prefix);
        if (bucket == index.end()) return;
        for (const auto& entry : bucket->second) {
            const int score = static_cast<int>(prefix.size()) * 25000 + source_bonus + entry.weight;
            auto old = found.find(entry.text);
            if (old != found.end() && old->second.base_score >= score) continue;
            Candidate candidate{entry.text, score};
            candidate.is_user_lexicon_entry = entry.personal;
            found[entry.text] = std::move(candidate);
        }
    };
    for (size_t length = 2; length <= std::min<size_t>(8, context.size()); ++length) {
        const auto prefix = context.substr(context.size() - length);
        append(dictionary_, prefix, 0);
        append(rules_, prefix, 14000);
        append(learned_, prefix, 18000);
    }
    std::vector<Candidate> result;
    result.reserve(found.size());
    for (auto& [text, candidate] : found) result.push_back(std::move(candidate));
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.base_score != right.base_score ? left.base_score > right.base_score : left.text < right.text;
    });
    if (result.size() > std::min(limit, kMaxCandidateCount)) result.resize(std::min(limit, kMaxCandidateCount));
    return result;
}
}  // namespace localpinyin
