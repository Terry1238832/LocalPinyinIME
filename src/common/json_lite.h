#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace localpinyin {

// A small JSON reader and writer for the files the settings app and the input
// method share (panel favorites, the sticker list, the record vault's
// contents). Strings are UTF-16 in memory and UTF-8 in text.
struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool boolean = false;
    double number = 0;
    std::wstring string;
    std::vector<JsonValue> array;
    std::vector<std::pair<std::wstring, JsonValue>> object;

    static JsonValue make_string(std::wstring value);
    static JsonValue make_number(double value);
    static JsonValue make_bool(bool value);
    static JsonValue make_array();
    static JsonValue make_object();

    [[nodiscard]] bool is_string() const noexcept { return type == Type::String; }
    [[nodiscard]] bool is_array() const noexcept { return type == Type::Array; }
    [[nodiscard]] bool is_object() const noexcept { return type == Type::Object; }
    [[nodiscard]] const JsonValue* find(std::wstring_view key) const;
    [[nodiscard]] std::wstring text(std::wstring_view key, std::wstring fallback = {}) const;
    [[nodiscard]] double num(std::wstring_view key, double fallback = 0) const;
    [[nodiscard]] bool flag(std::wstring_view key, bool fallback = false) const;
    // Sets or replaces a member of an object.
    JsonValue& set(std::wstring_view key, JsonValue value);
};

std::optional<JsonValue> parse_json(std::string_view utf8);
std::string write_json(const JsonValue& value);
// A UTF-8 JSON string literal, quotes included.
std::string json_quote(std::wstring_view value);

}  // namespace localpinyin
