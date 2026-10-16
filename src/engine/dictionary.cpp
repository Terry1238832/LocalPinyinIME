#include "dictionary.h"

#include "../common/logging.h"
#include "../common/utf_utils.h"
#include "../common/win32_utils.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace localpinyin {
namespace {

// Legal Mandarin syllables, shared by all dictionary layers. Derive initials
// against the character count so xian (先) is not incorrectly split as xi'an.
const std::unordered_set<std::wstring>& syllable_set() {
    static const std::unordered_set<std::wstring> syllables = [] {
        std::wistringstream stream(
            L"a ai an ang ao ba bai ban bang bao bei ben beng bi bian biao bie bin bing bo bu "
            L"ca cai can cang cao ce cen ceng cha chai chan chang chao che chen cheng chi chong chou chu chua chuai chuan chuang chui chun chuo ci cong cou cu cuan cui cun cuo "
            L"da dai dan dang dao de dei den deng di dia dian diao die ding diu dong dou du duan dui dun duo "
            L"e ei en eng er fa fan fang fei fen feng fo fou fu "
            L"ga gai gan gang gao ge gei gen geng gong gou gu gua guai guan guang gui gun guo "
            L"ha hai han hang hao he hei hen heng hong hou hu hua huai huan huang hui hun huo "
            L"ji jia jian jiang jiao jie jin jing jiong jiu ju juan jue jun "
            L"ka kai kan kang kao ke ken keng kong kou ku kua kuai kuan kuang kui kun kuo "
            L"la lai lan lang lao le lei leng li lia lian liang liao lie lin ling liu lo long lou lu luan lun luo lv lve "
            L"ma mai man mang mao me mei men meng mi mian miao mie min ming miu mo mou mu "
            L"na nai nan nang nao ne nei nen neng ni nian niang niao nie nin ning niu nong nou nu nuan nuo nv nve "
            L"o ou pa pai pan pang pao pei pen peng pi pian piao pie pin ping po pou pu "
            L"qi qia qian qiang qiao qie qin qing qiong qiu qu quan que qun "
            L"ran rang rao re ren reng ri rong rou ru ruan rui run ruo "
            L"sa sai san sang sao se sen seng sha shai shan shang shao she shei shen sheng shi shou shu shua shuai shuan shuang shui shun shuo si song sou su suan sui sun suo "
            L"ta tai tan tang tao te teng ti tian tiao tie ting tong tou tu tuan tui tun tuo "
            L"wa wai wan wang wei wen weng wo wu xi xia xian xiang xiao xie xin xing xiong xiu xu xuan xue xun "
            L"ya yan yang yao ye yi yin ying yo yong you yu yuan yue yun "
            L"za zai zan zang zao ze zei zen zeng zha zhai zhan zhang zhao zhe zhei zhen zheng zhi zhong zhou zhu zhua zhuai zhuan zhuang zhui zhun zhuo zi zong zou zu zuan zui zun zuo");
        std::unordered_set<std::wstring> result;
        for (std::wstring syllable; stream >> syllable;) result.insert(syllable);
        return result;
    }();
    return syllables;
}

std::wstring initials_for(const std::wstring& pinyin, size_t character_count,
                         const std::vector<size_t>& boundaries = {}, std::vector<size_t>* ends = nullptr) {
    const auto& syllables = syllable_set();
    if (character_count < 1 || character_count > 32 || pinyin.size() > 128) return {};
    // One bit per attempted character count avoids an allocation for every
    // pinyin position during the many small prediction-index queries.
    std::array<unsigned long long, 129> failed{};
    std::wstring result;
    const auto solve = [&](auto&& self, size_t offset, size_t count) -> bool {
        if (offset == pinyin.size()) return count == character_count;
        if (count == character_count || (failed[offset] & (1ULL << count))) return false;
        for (size_t length = std::min<size_t>(6, pinyin.size() - offset); length > 0; --length) {
            if (std::any_of(boundaries.begin(), boundaries.end(), [&](size_t boundary) {
                return boundary > offset && boundary < offset + length;
            })) continue;
            if (!syllables.contains(pinyin.substr(offset, length))) continue;
            result.push_back(pinyin[offset]);
            if (ends) ends->push_back(offset + length);
            if (self(self, offset + length, count + 1)) return true;
            result.pop_back();
            if (ends) ends->pop_back();
        }
        failed[offset] |= 1ULL << count;
        return false;
    };
    return solve(solve, 0, 0) ? result : L"";
}

std::wstring trim_ascii(std::wstring value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) {
        return L"";
    }
    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::wstring normalize_pinyin_key(const std::wstring& value) {
    std::wstring normalized;
    normalized.reserve(value.size());
    for (wchar_t ch : value) {
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

std::wstring directory_name(std::wstring path) {
    const auto slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return path.substr(0, slash);
}

void dictionary_module_anchor() {}

std::wstring module_directory() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&dictionary_module_anchor),
                            &module)) {
        return L".";
    }

    std::array<wchar_t, MAX_PATH> path{};
    const DWORD size = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (size == 0 || size >= path.size()) {
        return L".";
    }
    return directory_name(path.data());
}

std::wstring join_path(const std::wstring& directory, const std::wstring& leaf) {
    if (directory.empty()) {
        return leaf;
    }
    const wchar_t last = directory.back();
    if (last == L'\\' || last == L'/') {
        return directory + leaf;
    }
    return directory + L"\\" + leaf;
}

std::vector<std::wstring> split_tsv_line(const std::wstring& line) {
    std::vector<std::wstring> fields;
    size_t begin = 0;
    while (begin <= line.size()) {
        const size_t tab = line.find(L'\t', begin);
        if (tab == std::wstring::npos) {
            fields.push_back(line.substr(begin));
            break;
        }
        fields.push_back(line.substr(begin, tab - begin));
        begin = tab + 1;
    }
    return fields;
}

bool parse_int(const std::wstring& value, int& result) {
    const std::string narrow = wide_to_utf8(trim_ascii(value));
    const char* begin = narrow.data();
    const char* end = begin + narrow.size();
    const auto [ptr, ec] = std::from_chars(begin, end, result);
    return ec == std::errc{} && ptr == end;
}

std::wstring bool_text(bool value) {
    return value ? L"true" : L"false";
}

void merge_stats(DictionaryStats& target, const DictionaryStats& source) {
    target.source_rows += source.source_rows;
    target.comment_rows += source.comment_rows;
    target.blank_rows += source.blank_rows;
    target.duplicate_rows += source.duplicate_rows;
    target.invalid_rows += source.invalid_rows;
}

DictionaryStats to_dictionary_stats(const UserLexiconStats& source) {
    DictionaryStats stats;
    stats.source_rows = source.source_rows;
    stats.comment_rows = source.comment_rows;
    stats.blank_rows = source.blank_rows;
    stats.duplicate_rows = source.duplicate_rows;
    stats.invalid_rows = source.invalid_rows;
    stats.valid_entries = source.valid_entries;
    return stats;
}

std::wstring make_layer_log_message(const DictionaryLayerStats& layer) {
    std::wstringstream stream;
    stream << L"dictionary_layer=" << layer.layer_name
           << L" loaded=" << bool_text(layer.loaded)
           << L" missing=" << bool_text(layer.missing)
           << L" created=" << bool_text(layer.created)
           << L" valid_entries=" << layer.stats.valid_entries
           << L" invalid_rows=" << layer.stats.invalid_rows
           << L" duplicate_rows=" << layer.stats.duplicate_rows
           << L" comment_rows=" << layer.stats.comment_rows
           << L" blank_rows=" << layer.stats.blank_rows;
    return stream.str();
}

}  // namespace

Dictionary::Dictionary() {
    load_from_default_resource();
}

Dictionary::Dictionary(DictionaryLoadMode mode) {
    if (mode == DictionaryLoadMode::LoadDefaultResource) {
        load_from_default_resource();
    }
}

void Dictionary::clear() {
    entries_.clear();
    max_key_length_ = 0;
    prediction_index_dirty_ = true;
    prediction_partitions_.clear();
    initials_index_.clear();
    prediction_readings_.clear();
    entry_count_ = 0;
    stats_ = {};
    layer_stats_.clear();
    layer_log_messages_.clear();
    using_fallback_ = false;
    loaded_resource_path_.clear();
    user_lexicon_path_.clear();
    user_lexicon_overlays_.clear();
    has_user_lexicon_write_time_ = false;
}

bool Dictionary::add_entry(const std::wstring& pinyin,
                           const std::wstring& word,
                           int base_frequency,
                           EntryAddMode mode) {
    const std::wstring key = normalize_pinyin_key(pinyin);
    if (key.empty() || word.empty() || base_frequency < 0) {
        return false;
    }
    max_key_length_ = std::max(max_key_length_, key.size());
    prediction_index_dirty_ = true;

    auto& candidates = entries_[key];
    const auto it = std::find_if(candidates.begin(), candidates.end(), [&](const Candidate& candidate) {
        return candidate.text == word;
    });
    const bool added = it == candidates.end();
    if (!added) {
        if (mode == EntryAddMode::ReferenceFrequency) {
            it->decoding_frequency = base_frequency;
        } else if (mode == EntryAddMode::ReplaceExisting) {
            it->base_score = base_frequency;
        } else {
            it->base_score = std::max(it->base_score, base_frequency);
        }
    } else {
        candidates.push_back(Candidate{word, base_frequency, 0});
        if (mode == EntryAddMode::ReferenceFrequency) candidates.back().decoding_frequency = base_frequency;
        ++entry_count_;
    }

    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.base_score > right.base_score;
    });
    return added;
}

void Dictionary::add_entry(const std::wstring& pinyin, const std::vector<std::wstring>& words) {
    int score = static_cast<int>(words.size()) * 100;
    for (const auto& word : words) {
        add_entry(pinyin, word, score);
        score -= 10;
    }
}

void Dictionary::load_minimal_fallback() {
    add_entry(L"nihao", L"\u4F60\u597D", 100);
    add_entry(L"wo", L"\u6211", 100);
    using_fallback_ = true;
}

std::wstring Dictionary::default_resource_path() {
    return join_path(join_path(module_directory(), L"dictionary"), L"core_zh_pinyin.tsv");
}

std::wstring Dictionary::default_local_core_resource_path() {
    return join_path(join_path(module_directory(), L"dictionary"), L"local_core_zh_pinyin.tsv");
}

std::wstring Dictionary::default_user_lexicon_path() {
    return ::localpinyin::default_user_lexicon_path();
}

bool Dictionary::load_from_default_resource() {
    const std::wstring core_path = default_resource_path();
    if (load_from_layered_files(core_path,
                                default_local_core_resource_path(),
                                default_user_lexicon_path(),
                                UserLexiconCreateMode::CreateIfMissing,
                                join_path(join_path(module_directory(), L"dictionary"), L"standard_zh_pinyin.tsv"))) {
        return true;
    }

    const HRESULT hr = std::filesystem::exists(std::filesystem::path(core_path))
                           ? E_FAIL
                           : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    log_status(L"dictionary", L"load failed hr=" + hresult_hex(hr));
    log_status(L"dictionary", L"loaded entries=0");
    load_minimal_fallback();
    log_status(L"dictionary", L"fallback dictionary active");
    return false;
}

bool Dictionary::load_from_resource_directory(const std::wstring& directory) {
    return load_from_resource_directory(directory, default_user_lexicon_path());
}

bool Dictionary::load_from_resource_directory(const std::wstring& directory,
                                              const std::wstring& user_lexicon_path) {
    const std::wstring dictionary_dir = join_path(directory, L"dictionary");
    return load_from_layered_files(join_path(dictionary_dir, L"core_zh_pinyin.tsv"),
                                   join_path(dictionary_dir, L"local_core_zh_pinyin.tsv"),
                                   user_lexicon_path,
                                   UserLexiconCreateMode::CreateIfMissing,
                                   join_path(dictionary_dir, L"standard_zh_pinyin.tsv"));
}

bool Dictionary::load_from_layered_files(const std::wstring& core_path,
                                         const std::wstring& local_core_path,
                                         const std::wstring& user_lexicon_path,
                                         UserLexiconCreateMode create_mode,
                                         const std::wstring& standard_path) {
    clear();

    const auto core = load_layer(L"core", core_path, EntryAddMode::KeepExisting, true, false);
    merge_stats(stats_, core.stats);
    layer_stats_.push_back(core);

    const auto local_core = load_layer(L"local_core", local_core_path, EntryAddMode::ReplaceExisting, false, false);
    merge_stats(stats_, local_core.stats);
    layer_stats_.push_back(local_core);

    if (!standard_path.empty()) {
        const auto standard = load_layer(L"standard", standard_path, EntryAddMode::ReferenceFrequency, false, false);
        merge_stats(stats_, standard.stats);
        layer_stats_.push_back(standard);
    }

    const auto user = load_layer(L"local_user",
                                 user_lexicon_path,
                                 EntryAddMode::ReplaceExisting,
                                 false,
                                 create_mode == UserLexiconCreateMode::CreateIfMissing);
    merge_stats(stats_, user.stats);
    layer_stats_.push_back(user);

    stats_.valid_entries = entry_count_;
    if (entry_count_ > 0) {
        loaded_resource_path_ = core.loaded ? core_path : L"";
        return true;
    }
    return false;
}

DictionaryLayerStats Dictionary::load_layer(const std::wstring& layer_name,
                                            const std::wstring& path,
                                            EntryAddMode mode,
                                            bool /*required*/,
                                            bool create_if_missing) {
    DictionaryLayerStats layer;
    layer.layer_name = layer_name;
    if (layer_name == L"local_user") {
        load_user_lexicon_layer(path, layer, create_if_missing);
        layer_log_messages_.push_back(make_layer_log_message(layer));
        log_status(L"dictionary", layer_log_messages_.back());
        return layer;
    }

    if (path.empty()) {
        layer.missing = true;
        layer_log_messages_.push_back(make_layer_log_message(layer));
        log_status(L"dictionary", layer_log_messages_.back());
        return layer;
    }

    std::error_code exists_error;
    const bool exists = std::filesystem::exists(std::filesystem::path(path), exists_error);
    if (!exists || exists_error) {
        layer.missing = true;
        layer_log_messages_.push_back(make_layer_log_message(layer));
        log_status(L"dictionary", layer_log_messages_.back());
        return layer;
    }

    layer.loaded = load_tsv_file_into(path, layer.stats, mode);
    layer_log_messages_.push_back(make_layer_log_message(layer));
    log_status(L"dictionary", layer_log_messages_.back());
    return layer;
}

bool Dictionary::load_user_lexicon_layer(const std::wstring& path,
                                         DictionaryLayerStats& layer,
                                         bool create_if_missing) {
    user_lexicon_path_ = path;
    const UserLexiconLoadResult loaded = load_user_lexicon_file(path, create_if_missing);
    layer.created = loaded.created;
    layer.missing = loaded.missing;
    layer.stats = to_dictionary_stats(loaded.stats);

    if (loaded.failed) {
        layer.loaded = false;
        remember_user_lexicon_write_time();
        return false;
    }

    for (const auto& entry : loaded.entries) {
        add_user_lexicon_entry(entry, layer.stats);
    }
    layer.stats.valid_entries = loaded.entries.size();
    layer.loaded = loaded.loaded;
    remember_user_lexicon_write_time();
    return true;
}

bool Dictionary::add_user_lexicon_entry(const UserLexiconEntry& entry, DictionaryStats& layer_stats) {
    const std::wstring key = normalize_pinyin_key(entry.pinyin);
    if (key.empty() || entry.word.empty() || entry.frequency < 0) {
        return false;
    }
    max_key_length_ = std::max(max_key_length_, key.size());
    prediction_index_dirty_ = true;

    auto& candidates = entries_[key];
    const auto it = std::find_if(candidates.begin(), candidates.end(), [&](const Candidate& candidate) {
        return candidate.text == entry.word;
    });

    UserLexiconOverlay overlay;
    overlay.pinyin = key;
    overlay.word = entry.word;
    overlay.had_previous = it != candidates.end();
    if (overlay.had_previous) {
        overlay.previous_score = it->base_score;
        overlay.previous_decoding_frequency = it->decoding_frequency;
        overlay.previous_user_lexicon_entry = it->is_user_lexicon_entry;
        it->base_score = entry.frequency;
        it->decoding_frequency = entry.frequency;
        it->is_user_lexicon_entry = true;
        ++layer_stats.duplicate_rows;
    } else {
        candidates.push_back(Candidate{entry.word, entry.frequency, 0});
        candidates.back().decoding_frequency = entry.frequency;
        candidates.back().is_user_lexicon_entry = true;
        ++entry_count_;
    }
    user_lexicon_overlays_.push_back(overlay);

    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        return left.base_score > right.base_score;
    });
    return true;
}

void Dictionary::revert_user_lexicon_entries() {
    prediction_index_dirty_ = true;
    for (auto it = user_lexicon_overlays_.rbegin(); it != user_lexicon_overlays_.rend(); ++it) {
        auto candidates_it = entries_.find(it->pinyin);
        if (candidates_it == entries_.end()) {
            continue;
        }

        auto& candidates = candidates_it->second;
        const auto candidate_it = std::find_if(candidates.begin(), candidates.end(), [&](const Candidate& candidate) {
            return candidate.text == it->word;
        });
        if (candidate_it == candidates.end()) {
            continue;
        }

        if (it->had_previous) {
            candidate_it->base_score = it->previous_score;
            candidate_it->decoding_frequency = it->previous_decoding_frequency;
            candidate_it->is_user_lexicon_entry = it->previous_user_lexicon_entry;
        } else {
            candidates.erase(candidate_it);
            if (entry_count_ > 0) {
                --entry_count_;
            }
        }

        if (candidates.empty()) {
            entries_.erase(candidates_it);
        } else {
            std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
                return left.base_score > right.base_score;
            });
        }
    }
    user_lexicon_overlays_.clear();
}

void Dictionary::remember_user_lexicon_write_time() {
    has_user_lexicon_write_time_ = false;
    if (user_lexicon_path_.empty()) {
        return;
    }

    std::error_code ec;
    const auto time = std::filesystem::last_write_time(std::filesystem::path(user_lexicon_path_), ec);
    if (!ec) {
        user_lexicon_write_time_ = time;
        has_user_lexicon_write_time_ = true;
    }
}

bool Dictionary::user_lexicon_file_changed() const {
    if (user_lexicon_path_.empty()) {
        return false;
    }

    std::error_code ec;
    const auto time = std::filesystem::last_write_time(std::filesystem::path(user_lexicon_path_), ec);
    if (ec) {
        return has_user_lexicon_write_time_;
    }
    return !has_user_lexicon_write_time_ || time != user_lexicon_write_time_;
}

void Dictionary::recompute_stats_from_layers() {
    stats_ = {};
    for (const auto& layer : layer_stats_) {
        merge_stats(stats_, layer.stats);
    }
    stats_.valid_entries = entry_count_;
}

UserLexiconRefreshResult Dictionary::refresh_user_lexicon_if_changed(bool composition_active) {
    UserLexiconRefreshResult result;
    result.checked = true;
    if (!user_lexicon_file_changed()) {
        return result;
    }

    result.changed = true;
    if (composition_active) {
        result.skipped_active_composition = true;
        log_status(L"dictionary", L"user_lexicon_refresh skipped active_composition=true");
        return result;
    }

    const UserLexiconLoadResult loaded = load_user_lexicon_file(user_lexicon_path_, true);
    result.stats = loaded.stats;
    if (loaded.failed) {
        result.failed = true;
        log_status(L"dictionary", L"user_lexicon_refresh failed");
        return result;
    }

    DictionaryLayerStats layer;
    layer.layer_name = L"local_user";
    layer.created = loaded.created;
    layer.missing = loaded.missing;
    layer.loaded = loaded.loaded;
    layer.stats = to_dictionary_stats(loaded.stats);

    revert_user_lexicon_entries();
    for (const auto& entry : loaded.entries) {
        add_user_lexicon_entry(entry, layer.stats);
    }
    layer.stats.valid_entries = loaded.entries.size();

    bool replaced = false;
    for (auto& existing : layer_stats_) {
        if (existing.layer_name == L"local_user") {
            existing = layer;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        layer_stats_.push_back(layer);
    }

    recompute_stats_from_layers();
    remember_user_lexicon_write_time();
    result.reloaded = true;
    log_status(L"dictionary", make_layer_log_message(layer));
    return result;
}

bool Dictionary::load_tsv_file_into(const std::wstring& path,
                                    DictionaryStats& layer_stats,
                                    EntryAddMode mode) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        return false;
    }

    bool loaded_any = false;
    std::string line;
    while (std::getline(file, line)) {
        ++layer_stats.source_rows;

        const std::wstring decoded = utf8_to_wide(line);
        if (decoded.empty() && !line.empty()) {
            ++layer_stats.invalid_rows;
            continue;
        }

        const std::wstring wide = trim_ascii(decoded);
        if (wide.empty()) {
            ++layer_stats.blank_rows;
            continue;
        }
        if (wide.front() == L'#') {
            ++layer_stats.comment_rows;
            continue;
        }

        const auto fields = split_tsv_line(wide);
        if (fields.size() != 3) {
            ++layer_stats.invalid_rows;
            continue;
        }

        int frequency = 0;
        const std::wstring pinyin = trim_ascii(fields[0]);
        const std::wstring word = trim_ascii(fields[1]);
        if (normalize_pinyin_key(pinyin).empty() || word.empty() || !parse_int(fields[2], frequency) || frequency < 0) {
            ++layer_stats.invalid_rows;
            continue;
        }
        ++layer_stats.valid_entries;
        loaded_any = true;
        if (!add_entry(pinyin, word, frequency, mode)) {
            ++layer_stats.duplicate_rows;
        }
    }

    return loaded_any;
}

bool Dictionary::load_from_tsv_file(const std::wstring& path) {
    clear();
    DictionaryLayerStats layer;
    layer.layer_name = L"single";
    layer.loaded = load_tsv_file_into(path, layer.stats, EntryAddMode::KeepExisting);
    layer_stats_.push_back(layer);
    stats_ = layer.stats;
    stats_.valid_entries = entry_count_;
    if (layer.loaded) {
        loaded_resource_path_ = path;
    }
    return layer.loaded;
}

bool Dictionary::load_from_json_file(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::wstring content = utf8_to_wide(buffer.str());

    std::wstring key;
    std::vector<std::wstring> values;
    bool in_string = false;
    bool escape = false;
    std::wstring token;
    bool reading_key = true;

    for (wchar_t ch : content) {
        if (escape) {
            token.push_back(ch);
            escape = false;
            continue;
        }
        if (ch == L'\\' && in_string) {
            escape = true;
            continue;
        }
        if (ch == L'"') {
            if (in_string) {
                if (reading_key) {
                    key = token;
                    values.clear();
                    reading_key = false;
                } else {
                    values.push_back(token);
                }
                token.clear();
                in_string = false;
            } else {
                in_string = true;
            }
            continue;
        }
        if (in_string) {
            token.push_back(ch);
            continue;
        }
        if (ch == L']' && !key.empty()) {
            add_entry(trim_ascii(key), values);
            key.clear();
            values.clear();
            reading_key = true;
        }
    }

    return true;
}

std::vector<Candidate> Dictionary::lookup(const std::wstring& pinyin) const {
    const auto it = entries_.find(normalize_pinyin_key(pinyin));
    if (it == entries_.end()) {
        return {};
    }
    return it->second;
}

std::vector<std::wstring> Dictionary::matching_pinyins_at(const std::wstring& input, size_t offset) const {
    std::vector<std::wstring> matches;
    if (offset >= input.size()) return matches;
    // At most input length hash probes, rather than a complete dictionary scan
    // for every character of every keystroke.
    for (size_t length = std::min(max_key_length_, input.size() - offset); length > 0; --length) {
        std::wstring key = input.substr(offset, length);
        if (entries_.contains(key)) matches.push_back(std::move(key));
    }
    return matches;
}

const Dictionary::PredictionPartition& Dictionary::rebuild_prediction_index(wchar_t first_letter) const {
    if (prediction_index_dirty_) {
        prediction_partitions_.clear();
        initials_index_.clear();
        prediction_readings_.clear();
        prediction_index_dirty_ = false;
    }
    const auto cached = prediction_partitions_.find(first_letter);
    if (cached != prediction_partitions_.end()) return cached->second;

    // Ordinary typing touches one initial at a time. Completion and initials
    // share that initial's sorted keys, without sorting every other initial on
    // the first keystroke. Empty partitions are cached as well.
    PredictionPartition partition;
    for (const auto& [key, candidates] : entries_) {
        if (key.empty() || key.front() != first_letter || candidates.empty()) continue;
        partition.sorted_keys.push_back(key);
        for (size_t length = 1; length <= std::min<size_t>(2, key.size()); ++length) {
            partition.short_prefix_index[key.substr(0, length)].push_back(key);
        }
    }
    std::sort(partition.sorted_keys.begin(), partition.sorted_keys.end());
    for (auto& [prefix, keys] : partition.short_prefix_index) {
        const size_t retained = std::min<size_t>(64, keys.size());
        std::partial_sort(keys.begin(), keys.begin() + retained, keys.end(), [&](const auto& left, const auto& right) {
            const int left_score = entries_.at(left).front().base_score;
            const int right_score = entries_.at(right).front().base_score;
            if (left_score != right_score) return left_score > right_score;
            return left.size() != right.size() ? left.size() < right.size() : left < right;
        });
        keys.resize(retained);
    }
    return prediction_partitions_.emplace(first_letter, std::move(partition)).first->second;
}

const Dictionary::PredictionReading& Dictionary::prediction_reading(const std::wstring& key, size_t character_count) const {
    auto& readings = prediction_readings_[key];
    const auto found = std::find_if(readings.begin(), readings.end(), [&](const auto& reading) {
        return reading.character_count == character_count;
    });
    if (found != readings.end()) return *found;
    std::vector<size_t> ends;
    auto initials = initials_for(key, character_count, {}, &ends);
    const size_t final_begin = ends.size() <= 1 ? 0 : ends[ends.size() - 2];
    readings.push_back({character_count, final_begin, std::move(initials)});
    return readings.back();
}

std::vector<Candidate> Dictionary::lookup_abbreviation(const std::wstring& initials) const {
    if (initials.size() < 2 || initials.size() > 16) return {};
    const auto& partition = rebuild_prediction_index(initials.front());
    auto it = initials_index_.find(initials);
    if (it == initials_index_.end()) {
        // Query only the relevant first-letter range and character count.
        // Full dictionary syllabification is unnecessary for an ordinary key
        // or for the first two-letter abbreviation the user enters.
        std::vector<std::wstring> keys;
        for (const auto& key : partition.sorted_keys) {
            for (const auto& candidate : entries_.at(key)) {
                if (candidate.text.size() != initials.size() ||
                    !std::all_of(candidate.text.begin(), candidate.text.end(), [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; })) continue;
                const auto& reading = prediction_reading(key, candidate.text.size());
                if (reading.initials != initials || reading.initials == key) continue;
                keys.push_back(key);
                break;
            }
        }
        std::sort(keys.begin(), keys.end(), [&](const auto& left, const auto& right) {
            const int left_score = entries_.at(left).front().base_score;
            const int right_score = entries_.at(right).front().base_score;
            return left_score != right_score ? left_score > right_score : left < right;
        });
        if (keys.size() > 32) keys.resize(32);
        if (initials_index_.size() >= 256) initials_index_.clear();
        it = initials_index_.emplace(initials, std::move(keys)).first;
    }
    std::vector<Candidate> result;
    for (const auto& key : it->second) {
        const auto& candidates = entries_.at(key);
        size_t added = 0;
        for (const auto& candidate : candidates) {
            if (candidate.text.size() != initials.size()) continue;
            result.push_back(candidate);
            if (++added >= 3) break;
        }
    }
    return result;
}

std::vector<Candidate> Dictionary::lookup_completion(const std::wstring& prefix, bool final_syllable_only) const {
    if (prefix.empty() || prefix.size() > 32) return {};
    const auto& partition = rebuild_prediction_index(prefix.front());
    std::vector<Candidate> result;
    const auto add_key = [&](const std::wstring& key) {
        if (key.size() <= prefix.size() || key.size() - prefix.size() > 6) return false;
        const auto& candidates = entries_.at(key);
        size_t added = 0;
        for (const auto& candidate : candidates) {
            if (final_syllable_only) {
                if (candidate.text.empty() || candidate.text.size() > 16 ||
                    !std::all_of(candidate.text.begin(), candidate.text.end(), [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; })) continue;
                const auto& reading = prediction_reading(key, candidate.text.size());
                if (reading.initials.empty() || prefix.size() <= reading.final_syllable_begin) continue;
            }
            result.push_back(candidate);
            if (++added >= 3) break;
        }
        return added != 0;
    };
    if (prefix.size() <= 2) {
        const auto found = partition.short_prefix_index.find(prefix);
        if (found != partition.short_prefix_index.end()) {
            size_t matched = 0;
            for (const auto& key : found->second) {
                if (add_key(key) && ++matched >= 32) break;
            }
        }
        return result;
    }
    size_t matched_keys = 0;
    size_t scanned_keys = 0;
    for (auto it = std::lower_bound(partition.sorted_keys.begin(), partition.sorted_keys.end(), prefix);
         it != partition.sorted_keys.end() && it->starts_with(prefix) && matched_keys < 32 && scanned_keys < 128; ++it, ++scanned_keys) {
        if (add_key(*it)) ++matched_keys;
    }
    return result;
}

bool Dictionary::reading_matches_boundaries(const std::wstring& pinyin, const std::wstring& word,
                                             const std::vector<size_t>& boundaries) const {
    if (boundaries.empty() || !std::all_of(word.begin(), word.end(), [](wchar_t ch) { return ch >= 0x3400 && ch <= 0x9FFF; })) return true;
    return !initials_for(pinyin, word.size(), boundaries).empty();
}

bool Dictionary::is_syllable(const std::wstring& pinyin) {
    return syllable_set().contains(pinyin);
}

bool Dictionary::is_syllable_prefix(const std::wstring& pinyin) {
    if (pinyin.empty()) return false;
    const auto& syllables = syllable_set();
    return std::any_of(syllables.begin(), syllables.end(), [&](const std::wstring& syllable) {
        return syllable.compare(0, pinyin.size(), pinyin) == 0;
    });
}

size_t Dictionary::entry_count() const noexcept {
    return entry_count_;
}

}  // namespace localpinyin
