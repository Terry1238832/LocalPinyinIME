#include "../common/utf_utils.h"
#include "../common/win32_utils.h"
#include "../clipboard/clipboard_store.h"
#include "../clipboard/picture_clipboard.h"
#include "../common/json_lite.h"
#include "../engine/custom_phrases.h"
#include "../engine/input_tools.h"
#include "../engine/pinyin_engine.h"
#include "../engine/user_lexicon.h"
#include "../settings/dictionary_catalog.h"
#include "../settings/settings_store.h"
#include "../vault/vault.h"
#include "localpinyin_version.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <fcntl.h>
#include <io.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
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

// ["z-zh","n-l"]: the enabled fuzzy pairs.
// "a,b" -> ["a","b"]
std::string ids_json(const std::wstring& ids) {
    std::string result = "[";
    size_t start = 0;
    while (!ids.empty() && start <= ids.size()) {
        const size_t comma = ids.find(L',', start);
        if (result.size() > 1) result += ',';
        result += json_string(ids.substr(start, comma == std::wstring::npos ? std::wstring::npos : comma - start));
        if (comma == std::wstring::npos) break;
        start = comma + 1;
    }
    return result + ']';
}

std::string settings_json(const SettingsStore& store) {
    const auto candidate = store.load_candidate_options();
    const auto input = store.load_input_mode_options();
    return "{\"theme\":" + json_string(candidate_theme_mode_to_string(candidate.theme_mode)) +
           ",\"textSize\":" + std::to_string(candidate_text_size_level(candidate.text_size)) +
           ",\"candidateStyle\":" + json_string(candidate_style_to_string(candidate.style)) +
           ",\"shiftToggleEnabled\":" + (input.shift_toggle_enabled ? "true" : "false") +
           ",\"predictionEnabled\":" + (input.prediction_enabled ? "true" : "false") +
           ",\"typoCorrection\":" + (input.engine.typo_correction ? "true" : "false") +
           ",\"mixedEnglish\":" + (input.engine.mixed_english ? "true" : "false") +
           ",\"fuzzyPinyin\":" + ids_json(fuzzy_pinyin_to_string(input.engine.fuzzy)) +
           ",\"doublePinyin\":" + json_string(double_pinyin_to_string(input.engine.double_pinyin)) +
           ",\"usageDomains\":" + ids_json(usage_domains_to_string(input.engine.domains)) +
           ",\"digitPeriod\":" + (input.digit_period ? "true" : "false") +
           ",\"digitPunctuation\":" + (input.digit_punctuation ? "true" : "false") +
           ",\"letterPunctuation\":" + (input.letter_punctuation ? "true" : "false") +
           ",\"codePunctuation\":" + (input.code_punctuation ? "true" : "false") +
           ",\"tabTranslate\":" + (input.tab_translate ? "true" : "false") +
           ",\"panelDoubleTab\":" + (input.panel_double_tab ? "true" : "false") +
           ",\"panelResident\":" + (input.panel_resident ? "true" : "false") +
           ",\"emojiCandidates\":" + (input.engine.emoji_candidates ? "true" : "false") +
           ",\"clipboardHistory\":" + json_string(
               input.clipboard_history == InputModeOptions::ClipboardHistory::On ? L"on"
               : input.clipboard_history == InputModeOptions::ClipboardHistory::Off ? L"off" : L"ask") +
           ",\"clipboardRetentionDays\":" + std::to_string(input.clipboard_retention_days) + '}';
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

// The tool panel hands text to the input method in the application it was
// opened from (WM_COPYDATA to its window, which inserts at the caret).
constexpr ULONG_PTR kPanelInsertMessage = 0x4C50494E;  // 'LPIN'

HWND window_argument(const std::wstring& text) {
    require(!text.empty() && text.size() <= 20 && text.find_first_not_of(L"0123456789") == std::wstring::npos,
            L"窗口参数无效。");
    return reinterpret_cast<HWND>(static_cast<std::uintptr_t>(std::stoull(text)));
}

// Back to the application's window, where the text or picture goes.
void return_to(HWND host) {
    if (!host || !IsWindow(host)) return;
    if (IsIconic(host)) ShowWindow(host, SW_RESTORE);
    SetForegroundWindow(host);
    for (int attempt = 0; attempt < 20 && GetForegroundWindow() != host; ++attempt) Sleep(10);
}

// The pinyin a phrase is typed with: the dictionary's reading of the whole
// word, else each character's most common reading.
std::wstring phrase_pinyin(const DictionaryCatalog& catalog, const std::wstring& phrase) {
    std::wstring letters;
    for (wchar_t ch : phrase) {
        if (ch >= L'A' && ch <= L'Z') letters += static_cast<wchar_t>(ch - L'A' + L'a');
        else if (ch >= L'a' && ch <= L'z') letters += ch;
        else { letters.clear(); break; }
    }
    if (!letters.empty()) return letters;
    const DictionaryCatalogEntry* best = nullptr;
    for (const auto& entry : catalog.entries())
        if (entry.word == phrase && (!best || entry.weight > best->weight)) best = &entry;
    if (best) return best->pinyin;
    std::wstring pinyin;
    for (wchar_t ch : phrase) {
        const DictionaryCatalogEntry* reading = nullptr;
        for (const auto& entry : catalog.entries())
            if (entry.word.size() == 1 && entry.word[0] == ch && (!reading || entry.weight > reading->weight)) reading = &entry;
        if (!reading) return {};
        pinyin += reading->pinyin;
    }
    return pinyin;
}

std::string phrases_json(const std::wstring& path) {
    std::string result = "{\"phrases\":[";
    bool first = true;
    for (const auto& phrase : CustomPhrases::read(path)) {
        if (!first) result += ',';
        first = false;
        result += "{\"pinyin\":" + json_string(phrase.pinyin) + ",\"text\":" + json_string(phrase.text) +
                  ",\"label\":" + json_string(phrase.label) + ",\"created\":" + std::to_string(phrase.created) + '}';
    }
    return result + "]}";
}

// The picture on the clipboard, for a new sticker: a copied picture file
// (Explorer; chat programs, where GIFs stay whole), one an HTML copy points
// to, else the copied picture itself saved to `out` (GIF or PNG as copied, a
// bitmap as PNG). Returns the file; the caller checks it is a picture.
std::wstring copied_picture(const std::wstring& out) {
    using Microsoft::WRL::ComPtr;
    bool opened = false;
    for (int attempt = 0; attempt < 10 && !opened; ++attempt) {
        opened = OpenClipboard(nullptr) != FALSE;
        if (!opened) Sleep(25);
    }
    require(opened, L"剪贴板正被占用，请稍后重试。");
    const auto bytes_of = [](UINT format) {
        std::vector<std::uint8_t> bytes;
        if (!IsClipboardFormatAvailable(format)) return bytes;
        if (HANDLE data = GetClipboardData(format)) {
            if (const auto* value = static_cast<const std::uint8_t*>(GlobalLock(data))) {
                bytes.assign(value, value + GlobalSize(data));
                GlobalUnlock(data);
            }
        }
        return bytes;
    };
    std::wstring file;
    if (IsClipboardFormatAvailable(CF_HDROP)) {
        if (HANDLE data = GetClipboardData(CF_HDROP)) {
            wchar_t path[4096]{};
            if (DragQueryFileW(static_cast<HDROP>(data), 0, path, ARRAYSIZE(path))) file = path;
        }
    }
    if (file.empty()) {
        // <img src="file:///C:/…">, with %XX escapes.
        const auto html = bytes_of(RegisterClipboardFormatW(L"HTML Format"));
        const std::string text(html.begin(), html.end());
        const size_t start = text.find("src=\"file:///");
        const size_t end = start == std::string::npos ? std::string::npos : text.find('"', start + 14);
        if (end != std::string::npos) {
            std::string path;
            for (size_t at = start + 13; at < end; ++at) {
                if (text[at] == '%' && at + 2 < end) {
                    path += static_cast<char>(std::stoi(text.substr(at + 1, 2), nullptr, 16));
                    at += 2;
                } else {
                    path += text[at] == '/' ? '\\' : text[at];
                }
            }
            if (std::filesystem::is_regular_file(std::filesystem::path(utf8_to_wide(path)))) file = utf8_to_wide(path);
        }
    }
    std::vector<std::uint8_t> picture;
    HBITMAP bitmap = nullptr;
    if (file.empty()) {
        picture = bytes_of(RegisterClipboardFormatW(L"GIF"));
        if (picture.empty()) picture = bytes_of(RegisterClipboardFormatW(L"PNG"));
        if (picture.empty() && (IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_DIB)))
            if (auto copied = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP)))
                bitmap = static_cast<HBITMAP>(CopyImage(copied, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    }
    CloseClipboard();
    if (!file.empty()) return file;
    if (!picture.empty()) {
        std::ofstream target(std::filesystem::path(out), std::ios::binary);
        target.write(reinterpret_cast<const char*>(picture.data()), static_cast<std::streamsize>(picture.size()));
        require(target.good(), L"无法保存图片，请稍后重试。");
        return out;
    }
    require(bitmap != nullptr, L"剪贴板里没有图片。先在聊天里右键图片选“复制”，再回来按 Ctrl+V。");
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICBitmap> source;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    UINT width = 0, height = 0;
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    const bool saved =
        SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
        SUCCEEDED(factory->CreateBitmapFromHBITMAP(bitmap, nullptr, WICBitmapIgnoreAlpha, &source)) &&
        SUCCEEDED(factory->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(out.c_str(), GENERIC_WRITE)) &&
        SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
        SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) && SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)) &&
        SUCCEEDED(frame->Initialize(nullptr)) && SUCCEEDED(source->GetSize(&width, &height)) && SUCCEEDED(frame->SetSize(width, height)) &&
        SUCCEEDED(frame->SetPixelFormat(&format)) && SUCCEEDED(frame->WriteSource(source.Get(), nullptr)) &&
        SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit());
    DeleteObject(bitmap);
    require(saved, L"无法读取剪贴板里的图片。");
    return out;
}

void put_text_on_clipboard(const std::wstring& text) {
    require(OpenClipboard(nullptr) != FALSE, L"剪贴板正被占用，请稍后重试。");
    EmptyClipboard();
    const size_t size = (text.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size)) {
        std::memcpy(GlobalLock(memory), text.c_str(), size);
        GlobalUnlock(memory);
        if (!SetClipboardData(CF_UNICODETEXT, memory)) GlobalFree(memory);
    }
    CloseClipboard();
}

// A lock another process takes for the same file: its name's prefix and the
// file path, lowercased and hashed the way the input method does.
class NamedFileLock {
public:
    NamedFileLock(const wchar_t* prefix, const std::wstring& path) {
        unsigned long long hash = 1469598103934665603ULL;
        for (wchar_t ch : path) {
            if (ch >= L'A' && ch <= L'Z') ch = ch - L'A' + L'a';
            hash = (hash ^ static_cast<unsigned long long>(ch)) * 1099511628211ULL;
        }
        handle_ = CreateMutexW(nullptr, FALSE, (std::wstring(prefix) + std::to_wstring(hash)).c_str());
        if (handle_) {
            const DWORD result = WaitForSingleObject(handle_, 5000);
            acquired_ = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
        }
    }
    ~NamedFileLock() {
        if (acquired_) ReleaseMutex(handle_);
        if (handle_) CloseHandle(handle_);
    }
    NamedFileLock(const NamedFileLock&) = delete;
    NamedFileLock& operator=(const NamedFileLock&) = delete;
    bool acquired() const { return acquired_; }
private:
    HANDLE handle_ = nullptr;
    bool acquired_ = false;
};

bool replace_file(const std::wstring& source, const std::wstring& target) {
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(target).parent_path(), error);
    return MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != FALSE;
}

// The clipboard recorder: LocalPinyinClipboard.exe beside this bridge,
// started at sign-in through the user's Run key while history is on.
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"LocalPinyinClipboard";
constexpr wchar_t kAgentWindowClass[] = L"LocalPinyinIME.ClipboardAgent";

std::wstring clipboard_agent_path() { return (resource_directory() / L"LocalPinyinClipboard.exe").wstring(); }

void start_clipboard_agent() {
    const auto path = clipboard_agent_path();
    require(std::filesystem::is_regular_file(std::filesystem::path(path)), L"找不到剪贴板记录程序，请重新安装本地拼音。");
    wchar_t skip[8]{};
    if (GetEnvironmentVariableW(L"LOCALPINYINIME_NO_AUTOSTART", skip, 8) == 0 || std::wstring(skip) != L"1") {
        const std::wstring command = L"\"" + path + L"\"";
        RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, REG_SZ, command.c_str(),
                        static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    }
    if (FindWindowExW(HWND_MESSAGE, nullptr, kAgentWindowClass, nullptr)) return;
    std::wstring command = L"\"" + path + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    // Out of the settings app's job, so it keeps running when the app leaves.
    if (!CreateProcessW(path.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_BREAKAWAY_FROM_JOB | DETACHED_PROCESS,
                        nullptr, nullptr, &startup, &process) &&
        !CreateProcessW(path.c_str(), command.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, nullptr, &startup, &process))
        require(false, L"无法启动剪贴板记录程序。");
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

void stop_clipboard_agent() {
    RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue);
    HWND window = nullptr;
    while ((window = FindWindowExW(HWND_MESSAGE, window, kAgentWindowClass, nullptr)) != nullptr)
        PostMessageW(window, WM_CLOSE, 0, 0);
}

std::string clipboard_summary_json(const ClipboardStore& store) {
    size_t items = 0, images = 0;
    unsigned long long bytes = 0;
    for (const auto& item : store.read()) {
        ++items;
        if (item.image) ++images;
    }
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(store.directory()), error))
        if (entry.is_regular_file(error)) bytes += entry.file_size(error);
    return "{\"items\":" + std::to_string(items) + ",\"images\":" + std::to_string(images) + ",\"bytes\":" + std::to_string(bytes) + '}';
}

// A request with secrets (passwords, records) comes on stdin as JSON, so it
// never shows in the process list.
JsonValue read_request() {
    _setmode(_fileno(stdin), _O_BINARY);
    std::string text((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
    auto value = parse_json(text);
    if (!text.empty()) SecureZeroMemory(text.data(), text.size());
    require(value && value->is_object(), L"请求内容无效。");
    return std::move(*value);
}

std::string vault_status_json(const Vault& vault) {
    return std::string("{\"exists\":") + (vault.exists() ? "true" : "false") + ",\"password\":" + (vault.has_password() ? "true" : "false") + '}';
}

std::string vault_open_json(const Vault& vault, const std::wstring& password) {
    std::vector<VaultItem> items;
    require(vault.open(password, &items), vault.exists() ? L"密码不对。" : L"还没有创建记录。");
    std::stable_sort(items.begin(), items.end(), [](const VaultItem& left, const VaultItem& right) { return left.updated > right.updated; });
    return "{\"items\":" + vault_items_json(items) + '}';
}

// A record from the settings app, checked before it is kept.
VaultItem vault_item(const JsonValue& value) {
    require(value.is_object(), L"记录内容无效。");
    VaultItem item;
    item.id = value.text(L"id");
    item.kind = value.text(L"kind", L"other");
    if (item.kind != L"password" && item.kind != L"address") item.kind = L"other";
    item.title = value.text(L"title");
    require(!item.title.empty() && item.title.size() <= 100, L"标题应为 1 到 100 个字。");
    if (const auto* fields = value.find(L"fields"); fields && fields->is_array()) {
        require(fields->array.size() <= 32, L"一条记录最多 32 项。");
        for (const auto& entry : fields->array) {
            VaultField field{entry.text(L"name"), entry.text(L"value"), entry.flag(L"secret")};
            require(!field.name.empty() && field.name.size() <= 32, L"每一项都要有名字，最多 32 个字。");
            require(field.value.size() <= 4000, L"一项内容最多 4000 个字。");
            item.fields.push_back(std::move(field));
        }
    }
    return item;
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
        // Counted rather than loaded: the settings window waits for this.
        const auto stats = DictionaryCatalog::count((resource_directory() / L"dictionary").wstring());
        require(stats.total_unique_entries > 0, L"无法读取内置词库，请重新安装本地拼音。");
        const auto user = read_user_lexicon(user_path);
        UserLearning learning;
        require(learning.open(store.learning_data_path()), L"无法读取学习记录，请检查文件权限。");
        const auto runtime = store.load_runtime_input_mode_status();
        return "{\"version\":" + json_string(LOCALPINYINIME_VERSION_STRING) +
               ",\"settings\":" + settings_json(store) + ",\"dictionary\":{\"total\":" +
               std::to_string(stats.total_unique_entries) + ",\"core\":" + std::to_string(stats.core_entries) +
               ",\"local\":" + std::to_string(stats.local_core_entries) + ",\"standard\":" +
               std::to_string(stats.standard_entries) + "},\"learning\":{\"count\":" +
               std::to_string(learning.selected_texts().size()) + ",\"capacity\":" +
               std::to_string(UserLearning::kCapacity) + "},\"runtime\":{\"active\":" +
               (runtime.active ? "true" : "false") + ",\"mode\":" +
               json_string(runtime.mode == InputMode::Chinese ? L"Chinese" : L"English") +
               ",\"available\":" + (runtime.available ? "true" : "false") +
               "},\"dataDirectory\":" + json_string(store.data_dir()) + ",\"userLexiconCount\":" +
               std::to_string(user.entries.size()) + '}';
    }
    if (command == L"save-settings") {
        // theme size shift [prediction [style]] [key=value...]: the named
        // input options follow the positional ones.
        size_t positional = args.size();
        for (size_t index = 4; index < args.size(); ++index) {
            if (args[index].find(L'=') != std::wstring::npos) { positional = index; break; }
        }
        require(positional >= 4 && positional <= 6, L"请求参数数量不正确。");
        require(args[1] == L"system" || args[1] == L"light" || args[1] == L"dark", L"主题必须是 system、light 或 dark。");
        const int level = integer(args[2], 1, 7);
        require(args[3] == L"true" || args[3] == L"false", L"Shift 切换选项必须是 true 或 false。");
        if (positional >= 5) require(args[4] == L"true" || args[4] == L"false", L"前文联想选项必须是 true 或 false。");
        // The style is optional: a caller that omits it keeps the saved one.
        // A style id must be one the native candidate window knows.
        if (positional == 6) require(candidate_style_to_string(parse_candidate_style(args[5])) == args[5],
                                      L"候选框风格无效。");
        std::vector<std::pair<std::wstring, std::wstring>> named;
        for (size_t index = positional; index < args.size(); ++index) {
            const size_t equals = args[index].find(L'=');
            require(equals != std::wstring::npos, L"请求参数格式不正确。");
            const auto key = args[index].substr(0, equals), value = args[index].substr(equals + 1);
            if (key == L"typoCorrection" || key == L"mixedEnglish" || key == L"digitPeriod" || key == L"digitPunctuation" ||
                key == L"letterPunctuation" || key == L"codePunctuation" || key == L"tabTranslate" ||
                key == L"panelDoubleTab" || key == L"panelResident" || key == L"emojiCandidates")
                require(value == L"true" || value == L"false", L"开关选项必须是 true 或 false。");
            else if (key == L"clipboardHistory")
                require(value == L"ask" || value == L"on" || value == L"off", L"剪贴板记录选项无效。");
            else if (key == L"clipboardRetentionDays")
                require(value == L"0" || value == L"1" || value == L"7" || value == L"30" || value == L"90" || value == L"365",
                        L"剪贴板保存时间无效。");
            else if (key == L"doublePinyin")
                require(value == L"none" || value == L"xiaohe" || value == L"ziranma", L"双拼方案必须是 none、xiaohe 或 ziranma。");
            else if (key == L"fuzzyPinyin")
                require(value.empty() || fuzzy_pinyin_to_string(parse_fuzzy_pinyin(value)).size() == value.size(),
                        L"模糊音选项无效。");
            else if (key == L"usageDomains")
                require(value.empty() || usage_domains_to_string(parse_usage_domains(value)) == value,
                        L"常用场景选项无效。");
            else require(false, L"未知的设置项。");
            named.emplace_back(key, value);
        }
        BridgeFileLock lock(store.settings_path());
        require(lock.acquired(), L"设置正在保存，请稍后重试。");
        auto candidate = store.load_candidate_options();
        auto input = store.load_input_mode_options();
        candidate.theme_mode = parse_candidate_theme_mode(args[1]);
        candidate.text_size = candidate_text_size_from_level(level);
        input.shift_toggle_enabled = args[3] == L"true";
        if (positional >= 5) input.prediction_enabled = args[4] == L"true";
        if (positional == 6) candidate.style = parse_candidate_style(args[5]);
        for (const auto& [key, value] : named) {
            if (key == L"typoCorrection") input.engine.typo_correction = value == L"true";
            else if (key == L"mixedEnglish") input.engine.mixed_english = value == L"true";
            else if (key == L"doublePinyin") input.engine.double_pinyin = parse_double_pinyin(value);
            else if (key == L"fuzzyPinyin") input.engine.fuzzy = parse_fuzzy_pinyin(value);
            else if (key == L"usageDomains") input.engine.domains = parse_usage_domains(value);
            else if (key == L"digitPeriod") input.digit_period = value == L"true";
            else if (key == L"digitPunctuation") input.digit_punctuation = value == L"true";
            else if (key == L"letterPunctuation") input.letter_punctuation = value == L"true";
            else if (key == L"codePunctuation") input.code_punctuation = value == L"true";
            else if (key == L"tabTranslate") input.tab_translate = value == L"true";
            else if (key == L"panelDoubleTab") input.panel_double_tab = value == L"true";
            else if (key == L"panelResident") input.panel_resident = value == L"true";
            else if (key == L"emojiCandidates") input.engine.emoji_candidates = value == L"true";
            else if (key == L"clipboardHistory")
                input.clipboard_history = value == L"on" ? InputModeOptions::ClipboardHistory::On
                                        : value == L"off" ? InputModeOptions::ClipboardHistory::Off
                                                          : InputModeOptions::ClipboardHistory::Ask;
            else if (key == L"clipboardRetentionDays") input.clipboard_retention_days = std::stoul(value);
        }
        require(store.save_options(candidate, input), L"无法保存设置，请检查文件权限。");
        notify_options_changed();
        return "{\"settings\":" + settings_json(store) + '}';
    }
    if (command == L"lookup" || command == L"predict") {
        if (command == L"lookup") {
            require(args.size() == 2 || args.size() == 3, L"请求参数数量不正确。");
            require(args[1].size() <= 256, L"预览拼音最多允许 256 个字符。");
            require(args[1].find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'") == std::wstring::npos,
                    L"预览只接受拼音字母、数字和分隔用的单引号。");
        } else require_count(args, 2);
        const auto preceding = command == L"predict" ? args[1] : (args.size() == 3 ? args[2] : L"");
        require(preceding.size() <= 32 && preceding.find_first_of(L"\r\n\t") == std::wstring::npos &&
                (preceding.empty() || !wide_to_utf8(preceding).empty()), L"前文最多32个字符，不能含换行或无效字符。");
        // Unlike normal TSF activation, opening a preview must not create a
        // missing user lexicon or write learning, logs, or runtime status.
        // Surface an unreadable personal file rather than silently previewing
        // only system entries after the engine's optional-layer fallback.
        (void)read_user_lexicon(user_path);
        PinyinEngine engine(DictionaryLoadMode::Empty);
        require(engine.load_dictionary_resource_directory(resource_directory().wstring(), user_path,
                    UserLexiconCreateMode::DoNotCreate), L"无法加载输入法词库。");
        // The preview types exactly as the IME does under the saved options.
        engine.set_input_options(store.load_input_mode_options().engine);
        const auto candidates = command == L"predict" ? engine.predict(preceding) : engine.lookup(args[1], preceding);
        std::string result = "{\"candidates\":[";
        for (size_t index = 0; index < candidates.size(); ++index) {
            if (index) result += ',';
            result += json_string(candidates[index].text);
        }
        // Lookups also return the pinyin as the candidate window shows it
        // (segmented, e.g. sui'bian), so previews match the real popup.
        if (command == L"lookup") return result + "],\"pinyin\":" + json_string(engine.display_input(args[1])) + '}';
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
    if (command == L"domain-words") {
        // The words of every usage-scene pack, for the settings page to list.
        require_count(args, 1);
        std::string result = "{\"domains\":{";
        for (unsigned bit = 1; bit & kDomainAll; bit <<= 1) {
            const std::wstring id = usage_domain_id(static_cast<UsageDomain>(bit));
            const auto pack = DomainLexicon::read_pack((resource_directory() / L"dictionary" / L"domains" / (id + L".tsv")).wstring());
            result += (bit == 1 ? "" : ",") + json_string(id) + ":[";
            for (size_t index = 0; index < pack.size(); ++index)
                result += (index ? "," : "") + json_string(pack[index].second);
            result += ']';
        }
        return result + "}}";
    }
    if (command == L"tools") {
        // The typing tools for the panel's 工具 page: a sum, a number, a date,
        // or sj/rq/xq.
        require_count(args, 2);
        require(args[1].size() <= 128, L"内容过长。");
        std::tm now{};
        const std::time_t clock = std::time(nullptr);
        localtime_s(&now, &clock);
        std::wstring input = args[1];
        if (!input.empty() && (input[0] == L'v' || input[0] == L'V') && is_tool_text(input.substr(1))) input.erase(0, 1);
        auto results = is_tool_text(input) ? tool_results(input, now) : time_shortcuts(input, now);
        std::string result = "{\"results\":[";
        for (size_t index = 0; index < results.size(); ++index) result += (index ? "," : "") + json_string(results[index]);
        return result + "]}";
    }
    if (command == L"phrases-list") {
        require_count(args, 1);
        return phrases_json(store.data_dir() + L"\\custom_phrases.tsv");
    }
    if (command == L"phrase-pinyin") {
        require_count(args, 2);
        require(!args[1].empty() && args[1].size() <= 32, L"短语应为 1 到 32 个字。");
        const auto pinyin = phrase_pinyin(read_catalog(), args[1]);
        require(!pinyin.empty(), L"读不出这个短语的拼音，请换一个写法。");
        return "{\"pinyin\":" + json_string(pinyin) + '}';
    }
    if (command == L"phrases-add") {
        // phrases-add <text> <label> [pinyin]: typing the label offers the text.
        require(args.size() == 3 || args.size() == 4, L"请求参数数量不正确。");
        CustomPhrase phrase{args.size() == 4 ? args[3] : L"", args[1], args[2], 0};
        if (phrase.pinyin.empty()) phrase.pinyin = phrase_pinyin(read_catalog(), phrase.label);
        require(!phrase.pinyin.empty(), L"读不出这个短语的拼音，请换一个写法。");
        require(CustomPhrases::valid(phrase), L"短语或内容无效：短语最多 32 个字，内容最多 64 个字符，不能有换行。");
        require(store.ensure_data_dir(), L"无法创建数据目录。");
        require(CustomPhrases::add(store.data_dir() + L"\\custom_phrases.tsv", phrase), L"无法保存短语，请稍后重试。");
        return phrases_json(store.data_dir() + L"\\custom_phrases.tsv");
    }
    if (command == L"phrases-delete") {
        require_count(args, 3);
        require(CustomPhrases::remove(store.data_dir() + L"\\custom_phrases.tsv", args[1], args[2]), L"无法删除短语，请稍后重试。");
        return phrases_json(store.data_dir() + L"\\custom_phrases.tsv");
    }
    if (command == L"vault") {
        // vault <status|create|open|save|delete|password|destroy>, secrets on stdin.
        require_count(args, 2);
        require(store.ensure_data_dir(), L"无法创建数据目录。");
        const Vault vault(store.data_dir() + L"\\vault.json");
        const std::wstring& operation = args[1];
        if (operation == L"status") return vault_status_json(vault);
        if (operation == L"destroy") {
            require(vault.destroy(), L"无法删除记录文件，请稍后重试。");
            return vault_status_json(vault);
        }
        JsonValue request = read_request();
        std::wstring password = request.text(L"password");
        require(password.size() <= 128, L"密码最多 128 个字符。");
        std::string result;
        if (operation == L"create") {
            require(!vault.exists(), L"记录已经创建过了。");
            require(vault.create(password), L"无法创建记录文件。");
            result = vault_status_json(vault);
        } else if (operation == L"open") {
            result = vault_open_json(vault, password);
        } else if (operation == L"save" || operation == L"delete") {
            std::vector<VaultItem> items;
            require(vault.open(password, &items), L"密码不对。");
            const long long now = static_cast<long long>(std::time(nullptr));
            if (operation == L"save") {
                const auto* value = request.find(L"item");
                require(value != nullptr, L"记录内容无效。");
                VaultItem item = vault_item(*value);
                const auto same = std::find_if(items.begin(), items.end(), [&](const VaultItem& entry) { return !item.id.empty() && entry.id == item.id; });
                item.updated = now;
                if (same != items.end()) {
                    item.created = same->created;
                    *same = std::move(item);
                } else {
                    require(items.size() < Vault::kMaxItems, L"记录已满。");
                    item.id = Vault::new_id();
                    item.created = now;
                    items.push_back(std::move(item));
                }
            } else {
                const std::wstring id = request.text(L"id");
                items.erase(std::remove_if(items.begin(), items.end(), [&](const VaultItem& entry) { return entry.id == id; }), items.end());
            }
            require(vault.save(password, items), L"无法保存记录，请稍后重试。");
            result = vault_open_json(vault, password);
        } else if (operation == L"password") {
            const std::wstring next = request.text(L"newPassword");
            require(next.size() <= 128, L"密码最多 128 个字符。");
            require(vault.change_password(password, next), vault.has_password() ? L"原来的密码不对。" : L"无法修改密码。");
            result = vault_status_json(vault);
        } else {
            require(false, L"未知的记录操作。");
        }
        SecureZeroMemory(password.data(), password.size() * sizeof(wchar_t));
        return result;
    }
    if (command == L"copy-secret") {
        // A record copied for pasting: kept out of clipboard histories.
        require_count(args, 1);
        JsonValue request = read_request();
        std::wstring text = request.text(L"text");
        require(!text.empty() && text.size() <= 4000, L"内容无效。");
        require(OpenClipboard(nullptr) != FALSE, L"剪贴板正被占用，请稍后重试。");
        EmptyClipboard();
        const auto put = [](UINT format, const void* data, size_t size) {
            HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size ? size : 1);
            if (!memory) return;
            if (size) {
                std::memcpy(GlobalLock(memory), data, size);
                GlobalUnlock(memory);
            }
            if (!SetClipboardData(format, memory)) GlobalFree(memory);
        };
        put(CF_UNICODETEXT, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        const DWORD no = 0;
        put(RegisterClipboardFormatW(L"ExcludeClipboardContentFromMonitorProcessing"), nullptr, 0);
        put(RegisterClipboardFormatW(L"CanIncludeInClipboardHistory"), &no, sizeof(no));
        put(RegisterClipboardFormatW(L"CanUploadToCloudClipboard"), &no, sizeof(no));
        CloseClipboard();
        SecureZeroMemory(text.data(), text.size() * sizeof(wchar_t));
        return "{\"sequence\":" + std::to_string(GetClipboardSequenceNumber()) + '}';
    }
    if (command == L"clear-secret") {
        // clear-secret <sequence>: empties the clipboard if it still holds that copy.
        require_count(args, 2);
        const bool same = std::to_wstring(GetClipboardSequenceNumber()) == args[1];
        if (same && OpenClipboard(nullptr)) {
            EmptyClipboard();
            CloseClipboard();
        }
        return std::string("{\"cleared\":") + (same ? "true" : "false") + '}';
    }
    if (command == L"emoji-learn") {
        // emoji-learn <emoji> <name>...: the panel confirmed it; typing a
        // name later offers it third.
        require(args.size() >= 3 && args.size() <= 6, L"请求参数数量不正确。");
        require(!args[1].empty() && args[1].size() <= 32, L"表情无效。");
        require(store.ensure_data_dir(), L"无法创建数据目录。");
        const auto catalog = read_catalog();
        for (size_t index = 2; index < args.size(); ++index) {
            const auto pinyin = phrase_pinyin(catalog, args[index]);
            if (pinyin.empty()) continue;
            CustomPhrases::remember(store.data_dir() + L"\\emoji_candidates.tsv", CustomPhrase{pinyin, args[1], args[index], 0}, 3000);
        }
        return "{\"learned\":true}";
    }
    if (command == L"emoji-list") {
        require_count(args, 1);
        return phrases_json(store.data_dir() + L"\\emoji_candidates.tsv");
    }
    if (command == L"emoji-delete") {
        require_count(args, 3);
        require(CustomPhrases::remove(store.data_dir() + L"\\emoji_candidates.tsv", args[1], args[2]), L"无法删除，请稍后重试。");
        return phrases_json(store.data_dir() + L"\\emoji_candidates.tsv");
    }
    if (command == L"clipboard-agent") {
        require_count(args, 2);
        require(args[1] == L"start" || args[1] == L"stop", L"参数无效。");
        if (args[1] == L"start") start_clipboard_agent(); else stop_clipboard_agent();
        return std::string("{\"running\":") + (args[1] == L"start" ? "true" : "false") + '}';
    }
    if (command == L"clipboard-summary" || command == L"clipboard-clear") {
        require_count(args, 1);
        const ClipboardStore clipboard(store.data_dir() + L"\\clipboard");
        if (command == L"clipboard-clear") require(clipboard.clear(), L"剪贴板记录正被使用，请稍后重试。");
        return clipboard_summary_json(clipboard);
    }
    if (command == L"clipboard-delete") {
        require_count(args, 2);
        const std::string id = wide_to_utf8(args[1]);
        require(ClipboardStore::valid_id(id), L"剪贴板记录不存在。");
        require(ClipboardStore(store.data_dir() + L"\\clipboard").remove(id), L"剪贴板记录正被使用，请稍后重试。");
        return "{\"deleted\":true}";
    }
    if (command == L"clipboard-paste") {
        // clipboard-paste <application window> <id>: back on the clipboard
        // (and to the top of the history), pasted with Ctrl+V.
        require_count(args, 3);
        const std::string id = wide_to_utf8(args[2]);
        require(ClipboardStore::valid_id(id), L"剪贴板记录不存在。");
        const ClipboardStore clipboard(store.data_dir() + L"\\clipboard");
        bool image = false, found = false;
        for (const auto& item : clipboard.read()) if (item.id == id) { found = true; image = item.image; break; }
        require(found, L"剪贴板记录不存在。");
        if (image) {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            require(localpinyin::put_picture_on_clipboard(clipboard.picture_path(id), false), L"剪贴板正被占用，请稍后重试。");
        } else {
            std::wstring text;
            require(clipboard.full_text(id, &text), L"剪贴板记录不存在。");
            put_text_on_clipboard(text);
        }
        return_to(window_argument(args[1]));
        Sleep(60);
        localpinyin::press_paste();
        return "{\"pasted\":true}";
    }
    if (command == L"replace-data") {
        // replace-data <part> <file>: an imported file into place, under the
        // lock its writers take; the file is moved, not copied.
        require_count(args, 3);
        const auto& kind = args[1];
        const auto& source = args[2];
        require(std::filesystem::is_regular_file(std::filesystem::path(source)), L"导入的文件不存在。");
        require(store.ensure_data_dir(), L"无法创建数据目录。");
        bool placed = false;
        if (kind == L"settings") {
            BridgeFileLock lock(store.settings_path());
            require(lock.acquired(), L"设置正在保存，请稍后重试。");
            placed = replace_file(source, store.settings_path());
            if (placed) notify_options_changed();
        } else if (kind == L"lexicon") {
            const auto path = default_user_lexicon_path();
            BridgeFileLock lock(path);
            require(lock.acquired(), L"个人词库正在保存，请稍后重试。");
            placed = replace_file(source, path);
        } else if (kind == L"learning") {
            const auto path = store.data_dir() + L"\\user_learning.tsv";
            NamedFileLock lock(L"Local\\LocalPinyinIME.Learning.", path);
            require(lock.acquired(), L"选词记录正被使用，请稍后重试。");
            placed = replace_file(source, path);
        } else if (kind == L"phrases" || kind == L"emoji") {
            const auto path = store.data_dir() + (kind == L"phrases" ? L"\\custom_phrases.tsv" : L"\\emoji_candidates.tsv");
            NamedFileLock lock(L"Local\\LocalPinyinIME.Phrases.", path);
            require(lock.acquired(), L"输入习惯正被使用，请稍后重试。");
            placed = replace_file(source, path);
        } else if (kind == L"clipboard") {
            placed = ClipboardStore(store.data_dir() + L"\\clipboard").replace_index(source);
        } else if (kind == L"vault") {
            placed = replace_file(source, store.data_dir() + L"\\vault.json");
        } else {
            require(false, L"未知的数据类型。");
        }
        require(placed, L"无法写入导入的数据，请检查文件权限。");
        return "{\"replaced\":true}";
    }
    if (command == L"panel-insert") {
        // panel-insert <input method window> <application window> <token> <text>
        require_count(args, 5);
        require(!args[4].empty() && args[4].size() <= 4096, L"内容过长。");
        const HWND ime = window_argument(args[1]);
        require(IsWindow(ime) != FALSE, L"原来的输入框已经关闭。");
        return_to(window_argument(args[2]));
        const std::wstring payload = args[3] + L"\n" + args[4];
        COPYDATASTRUCT data{kPanelInsertMessage, static_cast<DWORD>((payload.size() + 1) * sizeof(wchar_t)),
                            const_cast<wchar_t*>(payload.c_str())};
        DWORD_PTR accepted = 0;
        require(SendMessageTimeoutW(ime, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data), SMTO_ABORTIFHUNG, 3000, &accepted) &&
                accepted == 1, L"输入法没有接收，请回到输入框后重试。");
        return "{\"inserted\":true}";
    }
    if (command == L"clipboard-picture") {
        // clipboard-picture <file>: the copied picture, as a file (maybe <file>).
        require_count(args, 2);
        require(std::filesystem::path(args[1]).is_absolute(), L"参数无效。");
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        return "{\"file\":" + json_string(copied_picture(args[1])) + '}';
    }
    if (command == L"panel-paste") {
        // panel-paste <application window> <picture>: pasted with Ctrl+V.
        require_count(args, 3);
        require(std::filesystem::is_regular_file(std::filesystem::path(args[2])), L"图片不存在。");
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        require(localpinyin::put_picture_on_clipboard(args[2]), L"剪贴板正被占用，请稍后重试。");
        return_to(window_argument(args[1]));
        Sleep(60);
        localpinyin::press_paste();
        return "{\"pasted\":true}";
    }
    if (command == L"share-stickers") {
        // The stickers folder, readable by store apps (the small panel there).
        require_count(args, 1);
        const std::wstring folder = store.data_dir() + L"\\stickers";
        require(store.ensure_data_dir() && ensure_directory(folder), L"无法创建表情包文件夹。");
        require(share_with_store_apps(folder), L"无法设置表情包文件夹的权限。");
        return "{\"shared\":true}";
    }
    if (command == L"learning-list") {
        // learning-list [query]: learned choices, the latest first; the query
        // matches the start of the pinyin or any part of the word.
        require(args.size() == 1 || args.size() == 2, L"请求参数数量不正确。");
        const std::wstring query = args.size() == 2 ? args[1] : L"";
        UserLearning learning;
        learning.open(store.learning_data_path());
        const auto entries = learning.entries();
        std::string result = "{\"total\":" + std::to_string(entries.size()) + ",\"entries\":[";
        size_t shown = 0;
        for (const auto& entry : entries) {
            if (!query.empty() && entry.pinyin.rfind(query, 0) != 0 && entry.word.find(query) == std::wstring::npos) continue;
            if (shown++ >= 300) break;
            if (shown > 1) result += ',';
            result += "{\"pinyin\":" + json_string(entry.pinyin) + ",\"word\":" + json_string(entry.word) +
                      ",\"count\":" + std::to_string(entry.count) + ",\"time\":" + std::to_string(entry.last_selected) + '}';
        }
        return result + "]}";
    }
    if (command == L"learning-forget") {
        // learning-forget <pinyin> <word>
        require_count(args, 3);
        UserLearning learning;
        require(learning.open(store.learning_data_path()), L"学习记录正在更新，请稍后重试。");
        const size_t forgotten = learning.forget(args[2], args[1]);
        notify_options_changed();
        return "{\"forgotten\":" + std::to_string(forgotten) + '}';
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
