#include "panel_data.h"

#include "../common/json_lite.h"
#include "../common/utf_utils.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace localpinyin {
namespace {

std::string read_all(const std::wstring& path) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::vector<std::wstring> split(const std::wstring& text, wchar_t separator) {
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (true) {
        const size_t end = text.find(separator, start);
        parts.push_back(text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start));
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    return parts;
}

constexpr const wchar_t* kKindNames[] = {L"symbol", L"kaomoji", L"emoji"};

std::vector<std::wstring> strings(const JsonValue* value, size_t limit) {
    std::vector<std::wstring> list;
    if (!value || !value->is_array()) return list;
    for (const auto& entry : value->array) {
        if (entry.is_string() && !entry.string.empty() && entry.string.size() <= 64) list.push_back(entry.string);
        if (list.size() >= limit) break;
    }
    return list;
}

}  // namespace

bool PanelCatalog::load(const std::wstring& tsv_path) {
    groups_.clear();
    const std::wstring text = utf8_to_wide(read_all(tsv_path));
    std::wistringstream lines(text);
    std::wstring line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        const auto columns = split(line, L'\t');
        if (columns.size() < 5 || columns[3].empty()) continue;
        if (groups_.empty() || groups_.back().id != columns[0]) groups_.push_back(PanelGroup{columns[0], columns[1], {}});
        PanelItem item{columns[3], columns[4], {}};
        if (columns.size() > 5 && !columns[5].empty()) item.keywords = split(columns[5], L'|');
        groups_.back().items.push_back(std::move(item));
    }
    return !groups_.empty();
}

const PanelItem* PanelCatalog::find(const std::wstring& text) const {
    for (const auto& group : groups_)
        for (const auto& item : group.items)
            if (item.text == text) return &item;
    return nullptr;
}

PanelUserState PanelUserState::load(const std::wstring& path) {
    PanelUserState state;
    const auto value = parse_json(read_all(path));
    if (!value || !value->is_object()) return state;
    const auto* favorites = value->find(L"favorites");
    const auto* recents = value->find(L"recents");
    for (size_t kind = 0; kind < 3; ++kind) {
        state.favorites[kind] = strings(favorites ? favorites->find(kKindNames[kind]) : nullptr, 500);
        state.recents[kind] = strings(recents ? recents->find(kKindNames[kind]) : nullptr, kRecentLimit);
    }
    return state;
}

bool PanelUserState::save(const std::wstring& path) const {
    JsonValue root = JsonValue::make_object();
    JsonValue favorite_lists = JsonValue::make_object();
    JsonValue recent_lists = JsonValue::make_object();
    for (size_t kind = 0; kind < 3; ++kind) {
        JsonValue favorite = JsonValue::make_array(), recent = JsonValue::make_array();
        for (const auto& text : favorites[kind]) favorite.array.push_back(JsonValue::make_string(text));
        for (const auto& text : recents[kind]) recent.array.push_back(JsonValue::make_string(text));
        favorite_lists.set(kKindNames[kind], std::move(favorite));
        recent_lists.set(kKindNames[kind], std::move(recent));
    }
    root.set(L"favorites", std::move(favorite_lists));
    root.set(L"recents", std::move(recent_lists));
    std::error_code error;
    const std::filesystem::path target(path);
    std::filesystem::create_directories(target.parent_path(), error);
    const std::wstring temporary = path + L".tmp";
    {
        std::ofstream file(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
        if (!file) return false;
        const std::string text = write_json(root) + "\n";
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file) return false;
    }
    return MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
}

void PanelUserState::remember(PanelKind kind, const std::vector<std::wstring>& picks) {
    auto& list = recents[static_cast<size_t>(kind)];
    for (auto pick = picks.rbegin(); pick != picks.rend(); ++pick) {
        list.erase(std::remove(list.begin(), list.end(), *pick), list.end());
        list.insert(list.begin(), *pick);
    }
    if (list.size() > kRecentLimit) list.resize(kRecentLimit);
}

std::vector<Sticker> load_stickers(const std::wstring& index_path) {
    std::vector<Sticker> stickers;
    const auto value = parse_json(read_all(index_path));
    if (!value || !value->is_array()) return stickers;
    for (const auto& entry : value->array) {
        Sticker sticker{entry.text(L"id"), entry.text(L"ext"), entry.text(L"name")};
        const bool id_ok = sticker.id.size() == 36 &&
            std::all_of(sticker.id.begin(), sticker.id.end(), [](wchar_t ch) { return (ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'f') || ch == L'-'; });
        const auto& ext = sticker.ext;
        const bool ext_ok = ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".gif" || ext == L".webp" || ext == L".bmp";
        if (id_ok && ext_ok) stickers.push_back(std::move(sticker));
    }
    return stickers;
}

bool contains_folded(const std::wstring& haystack, const std::wstring& needle) {
    if (needle.empty()) return true;
    const auto fold = [](wchar_t ch) { return ch >= L'A' && ch <= L'Z' ? static_cast<wchar_t>(ch - L'A' + L'a') : ch; };
    return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                       [&](wchar_t left, wchar_t right) { return fold(left) == fold(right); }) != haystack.end();
}

bool panel_item_matches(const PanelItem& item, const std::wstring& needle) {
    if (item.text == needle || contains_folded(item.name, needle)) return true;
    return std::any_of(item.keywords.begin(), item.keywords.end(), [&](const std::wstring& word) { return contains_folded(word, needle); });
}

}  // namespace localpinyin
