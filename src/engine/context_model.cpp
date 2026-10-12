#include "context_model.h"

#include "dictionary.h"
#include "../common/utf_utils.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace localpinyin {
namespace {

bool is_han(wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; }

bool valid_sequence(const std::wstring& text) {
    return !text.empty() && text.size() <= 4 && std::all_of(text.begin(), text.end(), [](wchar_t ch) {
        return is_han(ch) || ch == L'^' || ch == L'$';
    });
}

int log_weight(int count, int unit, int maximum) {
    return std::min(maximum, static_cast<int>(std::log1p(static_cast<double>(count)) * unit));
}

}  // namespace

void ContextModel::reset(const Dictionary& dictionary) {
    counts_.clear();
    learned_counts_.clear();
    word_scores_.clear();
    // Lexicon words are weak evidence for their own internal transitions.
    // Sentence boundaries and cross-word context come from the authored corpus.
    for (const auto& [pinyin, candidates] : dictionary.entries_) {
        for (const auto& candidate : candidates) {
            const auto& word = candidate.text;
            const int frequency = candidate.decoding_frequency > 0 ? candidate.decoding_frequency : candidate.base_score;
            if (word.empty() || word.size() > 4 || frequency < 500 ||
                !std::all_of(word.begin(), word.end(), is_han)) continue;
            const int weight = std::clamp(frequency / 2000, 1, 10);
            for (size_t offset = 0; offset < word.size(); ++offset) {
                for (size_t length = 1; length <= 4 && offset + length <= word.size(); ++length) {
                    auto& count = counts_[word.substr(offset, length)];
                    count = std::max(count, weight);
                }
            }
        }
    }
    dictionary_counts_ = counts_;
    rebuild_statistics();
}

bool ContextModel::load(const std::wstring& path) {
    word_scores_.clear();
    std::error_code error;
    const auto size = std::filesystem::file_size(std::filesystem::path(path), error);
    if (error || size > 4 * 1024 * 1024) return false;
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) return false;
    bool loaded = false;
    size_t rows = 0;
    for (std::string line; rows++ < 200000 && std::getline(file, line);) {
        if (line.empty() || line.front() == '#' || line.size() > 128) continue;
        if (line.back() == '\r') line.pop_back();
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto sequence = utf8_to_wide(line.substr(0, tab));
        if (!valid_sequence(sequence)) continue;
        int count = 0;
        const char* begin = line.data() + tab + 1;
        const char* end = line.data() + line.size();
        const auto [last, code] = std::from_chars(begin, end, count);
        if (code != std::errc{} || last != end || count <= 0) continue;
        // Corpus observations are stronger than a lexicon's weak pseudocounts.
        counts_[sequence] = std::min(1000000, counts_[sequence] + std::min(count, 100000) * 10);
        loaded = true;
    }
    rebuild_statistics();
    return loaded;
}

void ContextModel::set_learning(const std::vector<std::pair<std::wstring, int>>& selections) {
    if (selections.empty() && learned_counts_.empty()) return;
    learned_counts_.clear();
    word_scores_.clear();
    for (const auto& [word, frequency] : selections) {
        const int weight = std::clamp(frequency, 0, 20) * 32;
        if (!weight || word.size() > 128) continue;
        for (size_t offset = 0; offset < word.size(); ++offset) {
            for (size_t length = 1; length <= 4 && offset + length <= word.size(); ++length) {
                const auto sequence = word.substr(offset, length);
                if (!std::all_of(sequence.begin(), sequence.end(), is_han)) continue;
                auto& count = learned_counts_[sequence];
                count = std::min(10000, count + weight);
            }
        }
    }
    rebuild_statistics();
}

int ContextModel::count(const std::wstring& sequence) const {
    const auto found = counts_.find(sequence);
    const auto learned = learned_counts_.find(sequence);
    return (found == counts_.end() ? 0 : found->second) +
           (learned == learned_counts_.end() ? 0 : learned->second);
}

void ContextModel::rebuild_statistics() {
    continuation_totals_.clear();
    transition_scores_.clear();
    word_scores_.clear();
    unigram_total_ = 0;
    const auto accumulate = [&](const auto& observations) {
        for (const auto& [sequence, frequency] : observations) {
            if (sequence.size() > 1) continuation_totals_[sequence.substr(0, sequence.size() - 1)] += frequency;
            else if (is_han(sequence.front())) unigram_total_ += frequency;
        }
    };
    accumulate(counts_);
    accumulate(learned_counts_);
}

int ContextModel::transition_score(const std::wstring& sequence) const {
    const int observations = count(sequence);
    if (!observations) return sequence.size() == 2 ? -180 : 0;
    const auto cached = transition_scores_.find(sequence);
    if (cached != transition_scores_.end()) return cached->second;
    const auto total = [&](const std::wstring& prefix) {
        const auto found = continuation_totals_.find(prefix);
        return found == continuation_totals_.end() ? 0.0 : found->second;
    };
    const double unigram = (count(sequence.substr(sequence.size() - 1)) + 1.0) / (unigram_total_ + 512.0);
    double backoff = unigram;
    if (sequence.size() > 2) {
        const auto suffix = sequence.substr(1);
        backoff = (count(suffix) + 20.0 * unigram) / (total(suffix.substr(0, suffix.size() - 1)) + 20.0);
    }
    const double probability = (observations + 50.0 * backoff) /
                               (total(sequence.substr(0, sequence.size() - 1)) + 50.0);
    // Reward predictive information over the lower-order model. A frequent
    // character cannot earn its raw corpus frequency at every ngram order;
    // rare higher-order observations have proportionally weaker influence.
    const double confidence = observations / (observations + (sequence.size() == 2 ? 20.0 : 50.0));
    const double information = std::clamp(std::log(probability / backoff), -1.0, 3.0);
    const int unit = sequence.size() == 2 ? 350 : sequence.size() == 3 ? 450 : 550;
    const int information_score = static_cast<int>(information * confidence * unit);
    const auto seeded = dictionary_counts_.find(sequence);
    const int dictionary_count = seeded == dictionary_counts_.end() ? 0 : seeded->second;
    const int raw_unit = sequence.size() == 2 ? 200 : sequence.size() == 3 ? 250 : 300;
    const int raw_maximum = sequence.size() == 2 ? 1400 : sequence.size() == 3 ? 1600 : 2200;
    const int seed_score = log_weight(dictionary_count, raw_unit, raw_maximum);
    const int raw_score = log_weight(observations, raw_unit, raw_maximum);
    const double corpus_count = observations - dictionary_count;
    // Keep weak dictionary evidence, then discount only the corpus increment.
    // Frequent observed combinations retain their useful raw association.
    const double raw_confidence = corpus_count / (corpus_count + 20.0);
    const int score = static_cast<int>((seed_score + raw_confidence * (raw_score - seed_score) + information_score) / 2.0);
    transition_scores_.emplace(sequence, score);
    return score;
}

int ContextModel::extend_score(const std::wstring& previous, const std::wstring& word) const {
    if (!word.empty() && std::all_of(word.begin(), word.end(), is_han)) {
        // The word's internal ngrams do not depend on the beam path. Cache
        // those once, then evaluate only the three transitions across its edge.
        auto cached = word_scores_.find(word);
        if (cached == word_scores_.end()) {
            int intrinsic = 0;
            for (size_t offset = 0; offset < word.size(); ++offset) {
                intrinsic += log_weight(count(word.substr(offset, 1)), 20, 100) - 1500;
                if (offset >= 1) intrinsic += transition_score(word.substr(offset - 1, 2));
                if (offset >= 2) intrinsic += transition_score(word.substr(offset - 2, 3));
                if (offset >= 3) intrinsic += transition_score(word.substr(offset - 3, 4));
            }
            if (word_scores_.size() >= 8192) word_scores_.clear();
            cached = word_scores_.emplace(word, intrinsic).first;
        }
        int score = cached->second;
        const auto history = previous.empty() ? L"^" : previous.substr(previous.size() > 3 ? previous.size() - 3 : 0);
        score += transition_score(history.substr(history.size() - 1) + word.front());
        if (history.size() >= 2) score += transition_score(history.substr(history.size() - 2) + word.front());
        if (word.size() >= 2) score += transition_score(history.substr(history.size() - 1) + word.substr(0, 2));
        if (history.size() >= 3) score += transition_score(history.substr(history.size() - 3) + word.front());
        if (history.size() >= 2 && word.size() >= 2) score += transition_score(history.substr(history.size() - 2) + word.substr(0, 2));
        if (word.size() >= 3) score += transition_score(history.substr(history.size() - 1) + word.substr(0, 3));
        return score;
    }
    std::wstring history = previous.empty() ? L"^" : previous.substr(previous.size() > 3 ? previous.size() - 3 : 0);
    int score = 0;
    for (wchar_t ch : word) {
        if (!is_han(ch)) { history.clear(); score -= 300; continue; }
        score += log_weight(count(std::wstring(1, ch)), 20, 100) - 1500;
        if (!history.empty()) {
            score += transition_score(history.substr(history.size() - 1) + ch);
        }
        if (history.size() >= 2) {
            score += transition_score(history.substr(history.size() - 2) + ch);
        }
        if (history.size() >= 3) score += transition_score(history.substr(history.size() - 3) + ch);
        history.push_back(ch);
        if (history.size() > 3) history.erase(0, history.size() - 3);
    }
    return score;
}

int ContextModel::finish_score(const std::wstring& text) const {
    if (text.empty()) return 0;
    int score = log_weight(count(text.substr(text.size() - 1) + L"$"), 150, 700);
    if (text.size() >= 2) score += log_weight(count(text.substr(text.size() - 2) + L"$"), 180, 900);
    if (text.size() >= 3) score += log_weight(count(text.substr(text.size() - 3) + L"$"), 200, 1100);
    return score;
}

}  // namespace localpinyin
