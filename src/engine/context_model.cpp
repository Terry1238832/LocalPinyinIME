#include "context_model.h"

#include "dictionary.h"
#include "../common/utf_utils.h"

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>

namespace localpinyin {
namespace {

constexpr std::uint16_t kBos = 2;
constexpr std::uint16_t kEos = 3;
constexpr size_t kMaxOrder = 4;
constexpr std::uint32_t kBinaryVersion = 2;
constexpr size_t kBinaryHeaderBytes = 40;
constexpr double kCountDiscount = 0.75;
// Like the packaged model, a count table ignores single observations of
// longer sequences: one sighting is not evidence of a habitual phrase.
constexpr int kMinimumSequenceCount = 2;
// Unigrams are smoothed over a realistic Han inventory, so a small table
// stays close to uniform instead of favouring every character it mentions.
constexpr double kHanVocabulary = 6000;
constexpr int kNonHanPenalty = 2000;

bool is_han(wchar_t ch) noexcept { return ch >= 0x3400 && ch <= 0x9FFF; }

// Up to three history tokens, oldest first. A clause boundary (start of
// text, punctuation, Latin text) becomes the sentence-start token.
size_t history_tokens(std::wstring_view history, std::uint16_t* tokens) noexcept {
    std::uint16_t reversed[kMaxOrder - 1]{};
    size_t count = 0;
    bool bounded = true;
    for (size_t index = history.size(); index > 0 && count < kMaxOrder - 1; --index) {
        const wchar_t ch = history[index - 1];
        if (!is_han(ch)) { bounded = true; break; }
        reversed[count++] = static_cast<std::uint16_t>(ch);
        bounded = index - 1 == 0;
    }
    if (count < kMaxOrder - 1 && bounded) reversed[count++] = kBos;
    for (size_t index = 0; index < count; ++index) tokens[index] = reversed[count - 1 - index];
    return count;
}

std::uint64_t pack(const std::uint16_t* tokens, size_t count, std::uint16_t next) noexcept {
    std::uint64_t key = 0;
    for (size_t index = 0; index < count; ++index) key = (key << 16) | tokens[index];
    return (key << 16) | next;
}

// Unigram and bigram keys are exact. Longer n-grams use a 32-bit fingerprint
// of the packed key, matching tools/build_context_lm.py.
std::uint32_t compact_key(std::uint64_t packed, size_t order) noexcept {
    return order <= 2 ? static_cast<std::uint32_t>(packed)
                      : static_cast<std::uint32_t>((packed * 0x9E3779B97F4A7C15ULL) >> 32);
}

std::uint16_t token_of(wchar_t ch) noexcept {
    if (ch == L'^') return kBos;
    if (ch == L'$') return kEos;
    return is_han(ch) ? static_cast<std::uint16_t>(ch) : 0;
}

}  // namespace

struct ContextModel::Storage {
    // Either a read-only file mapping or owned arrays estimated from counts.
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = nullptr;
    const void* view = nullptr;
    std::array<std::vector<std::uint32_t>, kMaxOrder> keys;
    std::array<std::vector<std::int16_t>, kMaxOrder> logprob;
    std::array<std::vector<std::int16_t>, kMaxOrder> backoff;

    Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    ~Storage() {
        if (view) UnmapViewOfFile(view);
        if (mapping) CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    }
};

void ContextModel::reset(const Dictionary&) {
    tables_ = {};
    storage_.reset();
    unknown_logprob_ = -14000;
}

bool ContextModel::load(const std::wstring& path) {
    const auto extension = std::filesystem::path(path).extension().wstring();
    if (extension == L".lm" || extension == L".LM") return load_binary(path);
    return load_counts(path);
}

bool ContextModel::load_binary(const std::wstring& path) {
    auto storage = std::make_shared<Storage>();
    storage->file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (storage->file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(storage->file, &size) || size.QuadPart < static_cast<LONGLONG>(kBinaryHeaderBytes) ||
        size.QuadPart > 256LL * 1024 * 1024) return false;
    storage->mapping = CreateFileMappingW(storage->file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!storage->mapping) return false;
    storage->view = MapViewOfFile(storage->mapping, FILE_MAP_READ, 0, 0, 0);
    if (!storage->view) return false;
    const auto* bytes = static_cast<const unsigned char*>(storage->view);
    std::uint32_t header[9]{};
    std::memcpy(header, bytes + 4, sizeof(header));
    if (std::memcmp(bytes, "LPLM", 4) != 0 || header[0] != kBinaryVersion || header[1] != kMaxOrder) return false;
    std::array<Table, kMaxOrder> tables{};
    size_t offset = kBinaryHeaderBytes;
    for (size_t order = 0; order < kMaxOrder; ++order) {
        const size_t count = header[4 + order];
        const size_t bytes_needed = count * 8;
        if (offset + bytes_needed > static_cast<size_t>(size.QuadPart)) return false;
        tables[order].keys = reinterpret_cast<const std::uint32_t*>(bytes + offset);
        tables[order].logprob = reinterpret_cast<const std::int16_t*>(bytes + offset + count * 4);
        tables[order].backoff = reinterpret_cast<const std::int16_t*>(bytes + offset + count * 6);
        tables[order].size = count;
        offset += bytes_needed;
    }
    tables_ = tables;
    unknown_logprob_ = static_cast<int>(static_cast<std::int32_t>(header[3]));
    storage_ = std::move(storage);
    return true;
}

bool ContextModel::load_counts(const std::wstring& path) {
    std::error_code error;
    const auto file_size = std::filesystem::file_size(std::filesystem::path(path), error);
    if (error || file_size > 16 * 1024 * 1024) return false;
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) return false;
    std::array<std::map<std::uint64_t, double>, kMaxOrder> counts;
    bool loaded = false;
    for (std::string line; std::getline(file, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#' || line.size() > 128) continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto sequence = utf8_to_wide(line.substr(0, tab));
        if (sequence.empty() || sequence.size() > kMaxOrder) continue;
        std::uint64_t key = 0;
        bool valid = true;
        for (wchar_t ch : sequence) {
            const auto token = token_of(ch);
            valid = valid && token != 0;
            key = (key << 16) | token;
        }
        int count = 0;
        const char* begin = line.data() + tab + 1;
        const char* end = line.data() + line.size();
        const auto [last, code] = std::from_chars(begin, end, count);
        if (!valid || code != std::errc{} || last != end || count <= 0 || sequence == L"^" || sequence == L"$") continue;
        if (sequence.size() >= 2 && count < kMinimumSequenceCount) { loaded = true; continue; }
        counts[sequence.size() - 1][key] += std::min(count, 1000000);
        loaded = true;
    }
    if (!loaded) return false;

    // Interpolated absolute discounting, written in backoff form.
    std::array<std::map<std::uint64_t, std::pair<double, double>>, kMaxOrder> model;
    double unigram_total = 0;
    for (const auto& [key, count] : counts[0]) if (key != kBos) unigram_total += count;
    const double vocabulary = std::max(kHanVocabulary, static_cast<double>(counts[0].size()) + 1);
    const double unknown = unigram_total > 0 ? std::log(0.5 / (unigram_total + 0.5 * vocabulary)) : -14.0;
    for (const auto& [key, count] : counts[0])
        model[0][key] = {key == kBos ? -30.0 : std::log((count + 0.5) / (unigram_total + 0.5 * vocabulary)), 0.0};
    const auto lower_logprob = [&](size_t order, std::uint64_t key) {
        // Backoff probability of an order-(order + 1) key using lower tables.
        double accumulated = 0;
        for (size_t current = order + 1; current-- > 0;) {
            const std::uint64_t mask = current == 3 ? ~0ULL : ((1ULL << (16 * (current + 1))) - 1);
            const auto found = model[current].find(key & mask);
            if (found != model[current].end()) return accumulated + found->second.first;
            if (current == 0) break;
            const auto history = model[current - 1].find((key & mask) >> 16);
            if (history != model[current - 1].end()) accumulated += history->second.second;
        }
        return accumulated + unknown;
    };
    for (size_t order = 1; order < kMaxOrder; ++order) {
        std::map<std::uint64_t, std::pair<double, double>> histories;  // total, distinct
        for (const auto& [key, count] : counts[order]) {
            auto& history = histories[key >> 16];
            history.first += count;
            history.second += 1;
        }
        for (const auto& [history, stats] : histories) {
            const double gamma = kCountDiscount * stats.second / stats.first;
            auto found = model[order - 1].find(history);
            if (found == model[order - 1].end())
                found = model[order - 1].emplace(history, std::pair{lower_logprob(order - 1, history), 0.0}).first;
            found->second.second = std::log(gamma);
        }
        for (const auto& [key, count] : counts[order]) {
            const auto& stats = histories[key >> 16];
            const double gamma = kCountDiscount * stats.second / stats.first;
            const std::uint64_t lower_key = key & ((1ULL << (16 * order)) - 1);
            const double lower = std::exp(lower_logprob(order - 1, lower_key));
            model[order][key] = {std::log((count - kCountDiscount) / stats.first + gamma * lower), 0.0};
        }
    }
    auto storage = std::make_shared<Storage>();
    std::array<Table, kMaxOrder> tables{};
    for (size_t order = 0; order < kMaxOrder; ++order) {
        std::vector<std::pair<std::uint32_t, std::pair<double, double>>> entries;
        for (const auto& [key, values] : model[order]) entries.emplace_back(compact_key(key, order + 1), values);
        std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
            return left.first != right.first ? left.first < right.first : left.second.first > right.second.first;
        });
        for (const auto& [key, values] : entries) {
            if (!storage->keys[order].empty() && storage->keys[order].back() == key) continue;
            storage->keys[order].push_back(key);
            storage->logprob[order].push_back(static_cast<std::int16_t>(std::clamp(std::lround(values.first * 1000), -32000L, 0L)));
            storage->backoff[order].push_back(static_cast<std::int16_t>(std::clamp(std::lround(values.second * 1000), -32000L, 32000L)));
        }
        tables[order] = Table{storage->keys[order].data(), storage->logprob[order].data(),
                              storage->backoff[order].data(), storage->keys[order].size()};
    }
    tables_ = tables;
    unknown_logprob_ = static_cast<int>(std::lround(unknown * 1000));
    storage_ = std::move(storage);
    return true;
}

void ContextModel::set_learning(const std::vector<std::pair<std::wstring, int>>& selections) {
    learned_.clear();
    for (const auto& [word, frequency] : selections) {
        if (frequency <= 0 || word.size() > 128) continue;
        for (size_t end = 1; end < word.size(); ++end) {
            for (size_t order = 2; order <= kMaxOrder && order <= end + 1; ++order) {
                std::uint64_t key = 0;
                bool han = true;
                for (size_t index = end + 1 - order; index <= end; ++index) {
                    han = han && is_han(word[index]);
                    key = (key << 16) | static_cast<std::uint16_t>(word[index]);
                }
                if (!han) continue;
                auto& count = learned_[key];
                count = std::min(20, count + frequency);
            }
        }
    }
}

size_t ContextModel::find(const Table& table, std::uint64_t packed, size_t order) noexcept {
    const std::uint32_t key = compact_key(packed, order);
    const auto* end = table.keys + table.size;
    const auto* found = std::lower_bound(table.keys, end, key);
    return found != end && *found == key ? static_cast<size_t>(found - table.keys) : table.size;
}

int ContextModel::log_probability(std::wstring_view history, wchar_t next) const {
    const std::uint16_t next_token = token_of(next);
    if (!next_token || next_token == kBos) return unknown_logprob_;
    std::uint16_t tokens[kMaxOrder - 1]{};
    const size_t count = history_tokens(history, tokens);
    int accumulated = 0;
    for (size_t used = count + 1; used-- > 0;) {
        const auto* context = tokens + (count - used);
        const Table& table = tables_[used];
        const size_t index = find(table, pack(context, used, next_token), used + 1);
        if (index < table.size) return accumulated + table.logprob[index];
        if (used == 0) break;
        const Table& lower = tables_[used - 1];
        std::uint64_t history_key = 0;
        for (size_t position = 0; position < used; ++position) history_key = (history_key << 16) | context[position];
        const size_t history_index = find(lower, history_key, used);
        if (history_index < lower.size) accumulated += lower.backoff[history_index];
    }
    return accumulated + unknown_logprob_;
}

int ContextModel::learned_bonus(std::wstring_view history, wchar_t next) const {
    if (learned_.empty() || !is_han(next)) return 0;
    for (size_t order = std::min(kMaxOrder, history.size() + 1); order >= 2; --order) {
        std::uint64_t key = 0;
        bool han = true;
        for (size_t index = history.size() + 1 - order; index < history.size(); ++index) {
            han = han && is_han(history[index]);
            key = (key << 16) | static_cast<std::uint16_t>(history[index]);
        }
        if (!han) continue;
        const auto found = learned_.find((key << 16) | static_cast<std::uint16_t>(next));
        if (found != learned_.end())
            return static_cast<int>(order - 1) * (3500 + 500 * std::min(found->second, 8));
    }
    return 0;
}

int ContextModel::extend_score(const std::wstring& previous, const std::wstring& word) const {
    // Callers pass the whole clause when it is shorter than three characters,
    // so a short history always begins at a clause boundary.
    std::wstring history = previous.substr(previous.size() > 3 ? previous.size() - 3 : 0);
    int score = 0;
    for (wchar_t ch : word) {
        if (!is_han(ch)) {
            score -= kNonHanPenalty;
            history = L"^";
            continue;
        }
        int transition = log_probability(history, ch);
        if (!history.empty() && is_han(history.back()))
            transition = std::max(transition, log_probability(history, L'$') + log_probability(L"", ch) + boundary_penalty_);
        score += transition + learned_bonus(history, ch);
        history.push_back(ch);
        if (history.size() > 3) history.erase(0, history.size() - 3);
    }
    return score;
}

int ContextModel::finish_score(const std::wstring& text) const {
    if (text.empty()) return 0;
    return log_probability(std::wstring_view(text).substr(text.size() > 3 ? text.size() - 3 : 0), L'$');
}

size_t ContextModel::entry_count() const noexcept {
    size_t total = 0;
    for (const auto& table : tables_) total += table.size;
    return total;
}

}  // namespace localpinyin
