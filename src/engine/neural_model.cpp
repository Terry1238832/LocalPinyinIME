#include "neural_model.h"

#include <windows.h>
#include <immintrin.h>
#include <intrin.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace localpinyin {
namespace {

constexpr std::uint32_t kVersion = 1;
constexpr size_t kHeaderBytes = 24;
constexpr int kUnknown = 1;
constexpr int kClauseStart = 2;

float half_to_float(std::uint16_t half) noexcept {
    const std::uint32_t sign = static_cast<std::uint32_t>(half & 0x8000) << 16;
    std::uint32_t exponent = (half >> 10) & 0x1F;
    std::uint32_t mantissa = half & 0x3FF;
    std::uint32_t bits;
    if (exponent == 0) {
        if (mantissa == 0) {
            bits = sign;
        } else {
            // Subnormal: normalise it.
            exponent = 127 - 15 + 1;
            while (!(mantissa & 0x400)) { mantissa <<= 1; --exponent; }
            bits = sign | (exponent << 23) | ((mantissa & 0x3FF) << 13);
        }
    } else if (exponent == 31) {
        bits = sign | 0x7F800000 | (mantissa << 13);
    } else {
        bits = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);
    }
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool has_vector_math() noexcept {
    int info[4]{};
    __cpuid(info, 1);
    const bool fma = (info[2] & (1 << 12)) != 0, avx = (info[2] & (1 << 28)) != 0, f16c = (info[2] & (1 << 29)) != 0;
    const bool os_saves_ymm = (info[2] & (1 << 27)) != 0 && (_xgetbv(0) & 0x6) == 0x6;
    return fma && avx && f16c && os_saves_ymm;
}

// Dot product of `count` half floats with floats.
float dot_vector(const std::uint16_t* row, const float* x, size_t count) noexcept {
    __m256 sum = _mm256_setzero_ps();
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        const __m256 w = _mm256_cvtph_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(row + i)));
        sum = _mm256_fmadd_ps(w, _mm256_loadu_ps(x + i), sum);
    }
    alignas(32) float lanes[8];
    _mm256_store_ps(lanes, sum);
    float total = lanes[0] + lanes[1] + lanes[2] + lanes[3] + lanes[4] + lanes[5] + lanes[6] + lanes[7];
    for (; i < count; ++i) total += half_to_float(row[i]) * x[i];
    return total;
}

float dot_scalar(const std::uint16_t* row, const float* x, size_t count) noexcept {
    float total = 0;
    for (size_t i = 0; i < count; ++i) total += half_to_float(row[i]) * x[i];
    return total;
}

float sigmoid(float x) noexcept { return 1.0f / (1.0f + std::exp(-x)); }

}  // namespace

struct NeuralModel::Storage {
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = nullptr;
    const void* view = nullptr;

    Storage() = default;
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    ~Storage() {
        if (view) UnmapViewOfFile(view);
        if (mapping) CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    }
};

void NeuralModel::reset() noexcept {
    vocab_ = embed_ = hidden_ = 0;
    embedding_ = input_weights_ = hidden_weights_ = input_bias_ = hidden_bias_ = nullptr;
    projection_ = projection_bias_ = output_bias_ = nullptr;
    tokens_.clear();
    cache_.clear();
    storage_.reset();
}

bool NeuralModel::load(const std::wstring& path) {
    reset();
    auto storage = std::make_shared<Storage>();
    storage->file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (storage->file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(storage->file, &size) || size.QuadPart < static_cast<LONGLONG>(kHeaderBytes) ||
        size.QuadPart > 128LL * 1024 * 1024) return false;
    storage->mapping = CreateFileMappingW(storage->file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!storage->mapping) return false;
    storage->view = MapViewOfFile(storage->mapping, FILE_MAP_READ, 0, 0, 0);
    if (!storage->view) return false;
    const auto* bytes = static_cast<const unsigned char*>(storage->view);
    std::uint32_t header[5]{};
    std::memcpy(header, bytes + 4, sizeof(header));
    if (std::memcmp(bytes, "LPNN", 4) != 0 || header[0] != kVersion) return false;
    const size_t vocab = header[1], embed = header[2], hidden = header[3];
    if (vocab < 8 || vocab > 65536 || embed == 0 || embed > 4096 || hidden == 0 || hidden > 4096) return false;
    const size_t halves = vocab * embed + 3 * hidden * embed + 3 * hidden * hidden + 6 * hidden + embed * hidden + embed + vocab;
    if (kHeaderBytes + vocab * 2 + halves * 2 != static_cast<size_t>(size.QuadPart)) return false;
    const auto* characters = reinterpret_cast<const std::uint16_t*>(bytes + kHeaderBytes);
    const auto* next = characters + vocab;
    auto take = [&](size_t count) { const auto* start = next; next += count; return start; };
    embedding_ = take(vocab * embed);
    input_weights_ = take(3 * hidden * embed);
    hidden_weights_ = take(3 * hidden * hidden);
    input_bias_ = take(3 * hidden);
    hidden_bias_ = take(3 * hidden);
    projection_ = take(embed * hidden);
    projection_bias_ = take(embed);
    output_bias_ = take(vocab);
    for (size_t index = 4; index < vocab; ++index)
        if (characters[index]) tokens_.emplace(static_cast<wchar_t>(characters[index]), static_cast<int>(index));
    vocab_ = vocab;
    embed_ = embed;
    hidden_ = hidden;
    vector_math_ = has_vector_math();
    storage_ = std::move(storage);
    return true;
}

int NeuralModel::token(wchar_t ch) const noexcept {
    const auto found = tokens_.find(ch);
    return found == tokens_.end() ? kUnknown : found->second;
}

void NeuralModel::step(int token, const State& before, State& after) const {
    const auto dot = vector_math_ ? dot_vector : dot_scalar;
    std::vector<float> x(embed_);
    const auto* row = embedding_ + static_cast<size_t>(token) * embed_;
    for (size_t i = 0; i < embed_; ++i) x[i] = half_to_float(row[i]);
    const size_t h = hidden_;
    std::vector<float> gi(3 * h), gh(3 * h);
    for (size_t r = 0; r < 3 * h; ++r) {
        gi[r] = dot(input_weights_ + r * embed_, x.data(), embed_) + half_to_float(input_bias_[r]);
        gh[r] = dot(hidden_weights_ + r * h, before.data(), h) + half_to_float(hidden_bias_[r]);
    }
    after.resize(h);
    for (size_t i = 0; i < h; ++i) {
        const float reset = sigmoid(gi[i] + gh[i]);
        const float update = sigmoid(gi[h + i] + gh[h + i]);
        const float candidate = std::tanh(gi[2 * h + i] + reset * gh[2 * h + i]);
        after[i] = (1.0f - update) * candidate + update * before[i];
    }
}

double NeuralModel::log_probability(const State& state, int target) const {
    const auto dot = vector_math_ ? dot_vector : dot_scalar;
    std::vector<float> projected(embed_);
    for (size_t e = 0; e < embed_; ++e)
        projected[e] = dot(projection_ + e * hidden_, state.data(), hidden_) + half_to_float(projection_bias_[e]);
    std::vector<float> logits(vocab_);
    float highest = -1e30f;
    for (size_t v = 0; v < vocab_; ++v) {
        logits[v] = dot(embedding_ + v * embed_, projected.data(), embed_) + half_to_float(output_bias_[v]);
        highest = std::max(highest, logits[v]);
    }
    double total = 0;
    for (float logit : logits) total += std::exp(static_cast<double>(logit - highest));
    return static_cast<double>(logits[static_cast<size_t>(target)] - highest) - std::log(total);
}

NeuralModel::Cached NeuralModel::advance(const std::wstring& key, const Cached& from, wchar_t ch, bool scored) const {
    const auto found = cache_.find(key);
    if (found != cache_.end()) return found->second;
    if (cache_.size() >= kCacheEntries) cache_.clear();
    Cached next;
    const int id = token(ch);
    next.sum = scored ? from.sum + log_probability(from.state, id) : 0.0;
    step(id, from.state, next.state);
    cache_.emplace(key, next);
    return next;
}

std::vector<double> NeuralModel::score(std::wstring_view context, const std::vector<std::wstring>& texts) const {
    std::vector<double> result(texts.size(), 0.0);
    if (empty()) return result;
    // The clause-start state, then the context: cached under keys that
    // begin with U+0001 so they never collide with a text's beginnings.
    std::wstring key(1, L'\x01');
    Cached start;
    if (const auto found = cache_.find(key); found != cache_.end()) {
        start = found->second;
    } else {
        const State zero(hidden_, 0.0f);
        step(kClauseStart, zero, start.state);
        cache_.emplace(key, start);
    }
    // The context conditions the texts; only the texts are scored.
    for (wchar_t ch : context) {
        key.push_back(ch);
        start = advance(key, start, ch, false);
    }
    key.push_back(L'\x02');
    for (size_t index = 0; index < texts.size(); ++index) {
        std::wstring text_key = key;
        Cached at = start;
        for (wchar_t ch : texts[index]) {
            text_key.push_back(ch);
            at = advance(text_key, at, ch, true);
        }
        result[index] = at.sum;
    }
    return result;
}

}  // namespace localpinyin
