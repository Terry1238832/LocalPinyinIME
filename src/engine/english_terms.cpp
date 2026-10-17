#include "english_terms.h"

#include "dictionary.h"
#include "../common/utf_utils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace localpinyin {

bool EnglishTerms::reads_as_pinyin(const std::wstring& letters) {
    if (letters.empty()) return false;
    std::vector<bool> reachable(letters.size() + 1, false);
    reachable[0] = true;
    for (size_t start = 0; start < letters.size(); ++start) {
        if (!reachable[start]) continue;
        for (size_t length = 1; length <= 6 && start + length <= letters.size(); ++length) {
            if (Dictionary::is_syllable(letters.substr(start, length))) reachable[start + length] = true;
        }
    }
    return reachable.back();
}

bool EnglishTerms::load(const std::wstring& path) {
    clear();
    std::ifstream file{std::filesystem::path(path), std::ios::binary};
    if (!file) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
        if (line.empty() || line[0] == '#') continue;
        const auto first = line.find('\t');
        const auto second = first == std::string::npos ? std::string::npos : line.find('\t', first + 1);
        if (second == std::string::npos) continue;
        const std::wstring key = utf8_to_wide(line.substr(0, first));
        const std::wstring text = utf8_to_wide(line.substr(first + 1, second - first - 1));
        int weight = 0;
        try { weight = std::stoi(line.substr(second + 1)); } catch (...) { continue; }
        if (key.size() < 2 || key.size() > 24 || text.empty() ||
            !std::all_of(key.begin(), key.end(), [](wchar_t ch) { return (ch >= L'a' && ch <= L'z') || (ch >= L'0' && ch <= L'9'); })) continue;
        Term term{text, std::clamp(weight, 1, 100), reads_as_pinyin(key)};
        terms_.insert_or_assign(key, std::move(term));
        max_key_length_ = std::max(max_key_length_, key.size());
    }
    return !terms_.empty();
}

void EnglishTerms::clear() noexcept {
    terms_.clear();
    max_key_length_ = 0;
}

const EnglishTerms::Term* EnglishTerms::find(const std::wstring& key) const {
    const auto found = terms_.find(key);
    return found == terms_.end() ? nullptr : &found->second;
}

void EnglishTerms::matches_at(const std::wstring& input, size_t offset,
                              std::vector<std::pair<size_t, const Term*>>& out) const {
    out.clear();
    for (size_t length = 3; length <= max_key_length_ && offset + length <= input.size(); ++length) {
        const auto found = terms_.find(input.substr(offset, length));
        if (found != terms_.end() && !found->second.pinyin_like) out.emplace_back(length, &found->second);
    }
}

bool EnglishTerms::occurs_in(const std::wstring& input) const {
    std::vector<std::pair<size_t, const Term*>> found;
    for (size_t offset = 0; offset + 3 <= input.size(); ++offset) {
        matches_at(input, offset, found);
        if (!found.empty()) return true;
    }
    return false;
}

std::vector<const EnglishTerms::Term*> EnglishTerms::completions(const std::wstring& prefix) const {
    std::vector<const Term*> result;
    if (prefix.size() < 2) return result;
    for (const auto& [key, term] : terms_) {
        if (key.size() > prefix.size() && key.size() >= 3 && !term.pinyin_like && key.starts_with(prefix)) result.push_back(&term);
    }
    std::sort(result.begin(), result.end(), [](const Term* left, const Term* right) {
        return left->weight != right->weight ? left->weight > right->weight : left->text < right->text;
    });
    if (result.size() > 3) result.resize(3);
    return result;
}

}  // namespace localpinyin
