#include "../common/utf_utils.h"
#include "../common/win32_utils.h"
#include "../engine/pinyin_engine.h"
#include "../engine/user_lexicon.h"
#include "../settings/dictionary_catalog.h"
#include "../settings/settings_store.h"
#include "localpinyin_version.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace localpinyin;

// One process receives one command through argv and writes exactly one UTF-8
// JSON object. No shell, private-value diagnostics, or interactive prompts.
std::string json_string(const std::wstring& value) {
    const std::string utf8 = wide_to_utf8(value);
    if (!value.empty() && utf8.empty()) throw std::runtime_error("Invalid Unicode text");
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (unsigned char ch : utf8) {
        switch (ch) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (ch < 0x20) {
                    result += "\\u00";
                    result += hex[ch >> 4];
                    result += hex[ch & 15];
                } else result += static_cast<char>(ch);
        }
    }
    return result + '"';
}

struct BridgeError { std::wstring message; };

void require(bool condition, const wchar_t* message) {
    if (!condition) throw BridgeError{message};
}

void require_count(const std::vector<std::wstring>& args, size_t count) {
    require(args.size() == count, L"请求参数数量不正确。");
}

int integer(const std::wstring& text, int minimum, int maximum) {
    int value = 0;
    require(parse_user_lexicon_frequency(text, &value) && value >= minimum && value <= maximum,
            L"数值超出允许范围。");
    return value;
}

class BridgeFileLock {
public:
    explicit BridgeFileLock(const std::wstring& path) {
        unsigned long long hash = 1469598103934665603ULL;
        for (wchar_t ch : path) {
            if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
            hash = (hash ^ static_cast<unsigned long long>(ch)) * 1099511628211ULL;
        }
        const auto name = L"Local\\LocalPinyinIME.SettingsBridge." + std::to_wstring(hash);
        handle_ = CreateMutexW(nullptr, FALSE, name.c_str());
        if (handle_) {
            const DWORD result = WaitForSingleObject(handle_, 5000);
            acquired_ = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
        }
    }
    ~BridgeFileLock() {
        if (acquired_) ReleaseMutex(handle_);
        if (handle_) CloseHandle(handle_);
    }
    bool acquired() const { return acquired_; }
    BridgeFileLock(const BridgeFileLock&) = delete;
    BridgeFileLock& operator=(const BridgeFileLock&) = delete;
private:
    HANDLE handle_ = nullptr;
    bool acquired_ = false;
};

std::filesystem::path resource_directory() {
    return std::filesystem::path(module_path(nullptr)).parent_path();
}

void notify_options_changed() {
    const UINT message = RegisterWindowMessageW(L"LocalPinyinIME.CandidateOptionsChanged");
    if (message) PostMessageW(HWND_BROADCAST, message, 0, 0);
}

std::string settings_json(const SettingsStore& store) {
    const auto candidate = store.load_candidate_options();
    const auto input = store.load_input_mode_options();
    return "{\"theme\":" + json_string(candidate_theme_mode_to_string(candidate.theme_mode)) +
           ",\"textSize\":" + std::to_string(candidate_text_size_level(candidate.text_size)) +
           ",\"shiftToggleEnabled\":" + (input.shift_toggle_enabled ? "true" : "false") + '}';
}

UserLexiconLoadResult read_user_lexicon(const std::wstring& path) {
    require(!path.empty(), L"无法找到个人词库位置。");
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    require(error || size <= 8 * 1024 * 1024, L"个人词库文件过大，无法在设置中打开。");
    auto loaded = load_user_lexicon_file(path, false);
    require(!loaded.failed, L"无法读取个人词库，请检查文件权限。");
    return loaded;
}

std::string entries_json(const std::vector<UserLexiconEntry>& entries) {
    std::string result = "{\"entries\":[";
    for (size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries[index];
        if (index) result += ',';
        result += "{\"pinyin\":" + json_string(entry.pinyin) + ",\"word\":" + json_string(entry.word) +
                  ",\"frequency\":" + std::to_string(entry.frequency) + '}';
    }
    return result + "]}";
}

UserLexiconEntry validated_entry(const std::wstring& pinyin, const std::wstring& word, int frequency) {
    require(pinyin.size() <= 256 && word.size() <= 128, L"拼音或词语太长，最多允许 256 个拼音字母和 128 个字符。");
    require(word.empty() || !wide_to_utf8(word).empty(), L"词语包含无效的 Unicode 字符。");
    UserLexiconEntry entry;
    const auto validation = validate_user_lexicon_entry(pinyin, word, frequency, &entry);
    if (!validation.valid) throw BridgeError{validation.message};
    return entry;
}

DictionaryCatalog read_catalog() {
    DictionaryCatalog catalog;
    require(catalog.load((resource_directory() / L"dictionary").wstring()), L"无法读取内置词库，请重新安装本地拼音。");
    return catalog;
}

std::string run(const std::vector<std::wstring>& args) {
    require(!args.empty(), L"缺少请求命令。");
    const auto& command = args[0];
    SettingsStore store;
    const auto user_path = default_user_lexicon_path();

    if (command == L"state") {
        require_count(args, 1);
        const auto catalog = read_catalog();
        const auto& stats = catalog.stats();
        const auto user = read_user_lexicon(user_path);
        UserLearning learning;
        require(learning.open(store.learning_data_path()), L"无法读取学习记录，请检查文件权限。");
        const auto runtime = store.load_runtime_input_mode_status();
        return "{\"version\":" + json_string(LOCALPINYINIME_VERSION_STRING) +
               ",\"settings\":" + settings_json(store) + ",\"dictionary\":{\"total\":" +
               std::to_string(stats.total_unique_entries) + ",\"core\":" + std::to_string(stats.core_entries) +
               ",\"local\":" + std::to_string(stats.local_core_entries) + ",\"standard\":" +
               std::to_string(stats.standard_entries) + "},\"learning\":{\"count\":" +
               std::to_string(learning.selected_texts().size()) + "},\"runtime\":{\"active\":" +
               (runtime.active ? "true" : "false") + ",\"mode\":" +
               json_string(runtime.mode == InputMode::Chinese ? L"Chinese" : L"English") +
               ",\"available\":" + (runtime.available ? "true" : "false") +
               "},\"dataDirectory\":" + json_string(store.data_dir()) + ",\"userLexiconCount\":" +
               std::to_string(user.entries.size()) + '}';
    }
    if (command == L"save-settings") {
        require_count(args, 4);
        require(args[1] == L"system" || args[1] == L"light" || args[1] == L"dark", L"主题必须是 system、light 或 dark。");
        const int level = integer(args[2], 1, 7);
        require(args[3] == L"true" || args[3] == L"false", L"Shift 切换选项必须是 true 或 false。");
        BridgeFileLock lock(store.settings_path());
        require(lock.acquired(), L"设置正在保存，请稍后重试。");
        auto candidate = store.load_candidate_options();
        auto input = store.load_input_mode_options();
        candidate.theme_mode = parse_candidate_theme_mode(args[1]);
        candidate.text_size = candidate_text_size_from_level(level);
        input.shift_toggle_enabled = args[3] == L"true";
        require(store.save_options(candidate, input), L"无法保存设置，请检查文件权限。");
        notify_options_changed();
        return "{\"settings\":" + settings_json(store) + '}';
    }
    if (command == L"lookup") {
        require_count(args, 2);
        require(args[1].size() <= 256, L"预览拼音最多允许 256 个字符。");
        require(args[1].find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'") == std::wstring::npos,
                L"预览只接受拼音字母、数字和分隔用的单引号。");
        // Unlike normal TSF activation, opening a preview must not create a
        // missing user lexicon or write learning, logs, or runtime status.
        // Surface an unreadable personal file rather than silently previewing
        // only system entries after the engine's optional-layer fallback.
        (void)read_user_lexicon(user_path);
        PinyinEngine engine(DictionaryLoadMode::Empty);
        require(engine.load_dictionary_resource_directory(resource_directory().wstring(), user_path,
                    UserLexiconCreateMode::DoNotCreate), L"无法加载输入法词库。");
        const auto candidates = engine.lookup(args[1]);
        std::string result = "{\"candidates\":[";
        for (size_t index = 0; index < candidates.size(); ++index) {
            if (index) result += ',';
            result += json_string(candidates[index].text);
        }
        return result + "]}";
    }
    if (command == L"user-list") {
        require_count(args, 1);
        return entries_json(read_user_lexicon(user_path).entries);
    }
    if (command == L"user-upsert" || command == L"user-delete") {
        if (command == L"user-upsert") require(args.size() == 4 || args.size() == 6, L"请求参数数量不正确。");
        else require_count(args, 3);
        const auto entry = validated_entry(args[1], args[2], command == L"user-upsert" ? integer(args[3], 0, 1000000) : 0);
        BridgeFileLock lock(user_path);
        require(lock.acquired(), L"个人词库正在保存，请稍后重试。");
        auto loaded = read_user_lexicon(user_path);
        require(loaded.stats.invalid_rows == 0, L"个人词库包含无法识别的词条，请先修复文件后再保存，以免丢失数据。");
        auto& entries = loaded.entries;
        if (command == L"user-upsert") {
            if (args.size() == 6) {
                const auto previous = validated_entry(args[4], args[5], 0);
                require(remove_user_lexicon_entry(entries, previous.pinyin, previous.word), L"原词条已经更改或删除，请刷新个人词库。");
            }
            require(upsert_user_lexicon_entry(entries, entry, nullptr), L"无法添加个人词条。");
        } else {
            require(remove_user_lexicon_entry(entries, entry.pinyin, entry.word), L"词条已经删除，请刷新个人词库。");
        }
        require(save_user_lexicon_file_atomic(user_path, entries).saved, L"无法保存个人词库，请检查文件权限。");
        return entries_json(entries);
    }
    if (command == L"dictionary-search") {
        require(args.size() == 2 || args.size() == 3, L"请求参数数量不正确。");
        require(args[1].size() <= 128, L"搜索内容最多允许 128 个字符。");
        const size_t limit = args.size() == 3 ? static_cast<size_t>(integer(args[2], 1, 500)) : 100;
        auto query = args[1];
        for (auto& ch : query) if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
        const auto catalog = read_catalog();
        std::string result = "{\"entries\":[";
        size_t total = 0;
        size_t included = 0;
        for (const auto& entry : catalog.entries()) {
            if (!query.empty() && entry.pinyin.find(query) == std::wstring::npos && entry.word.find(query) == std::wstring::npos) continue;
            ++total;
            if (included == limit) continue;
            if (included++) result += ',';
            const wchar_t* source = entry.source == DictionaryCatalogSource::Core ? L"core" :
                                    entry.source == DictionaryCatalogSource::LocalCore ? L"local" : L"standard";
            result += "{\"pinyin\":" + json_string(entry.pinyin) + ",\"word\":" + json_string(entry.word) +
                      ",\"frequency\":" + std::to_string(entry.weight) + ",\"source\":" + json_string(source) + '}';
        }
        return result + "],\"total\":" + std::to_string(total) + '}';
    }
    if (command == L"clear-learning") {
        require_count(args, 1);
        BridgeFileLock lock(store.learning_data_path());
        require(lock.acquired(), L"学习记录正在更新，请稍后重试。");
        require(store.clear_learning_data(), L"无法清除学习记录，请检查文件权限。");
        notify_options_changed();
        return "{\"cleared\":true}";
    }
    throw BridgeError{L"未知请求命令。"};
}
}  // namespace

int wmain(int argc, wchar_t** argv) {
    // This process is a private settings API, not a diagnostics session.
    SetEnvironmentVariableW(L"LOCALPINYINIME_DIAGNOSTICS", nullptr);
    try {
        std::vector<std::wstring> args;
        for (int index = 1; index < argc; ++index) args.emplace_back(argv[index]);
        const auto result = run(args);
        std::cout << result << '\n';
        return std::cout ? 0 : 3;
    } catch (const BridgeError& error) {
        std::cout << "{\"error\":" << json_string(error.message) << "}\n";
    } catch (...) {
        std::cout << "{\"error\":" << json_string(L"无法完成请求，请重试或重新安装本地拼音。") << "}\n";
    }
    return 2;
}
