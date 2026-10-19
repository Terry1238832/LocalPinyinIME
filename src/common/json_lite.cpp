#include "json_lite.h"

#include "utf_utils.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace localpinyin {
namespace {

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    std::optional<JsonValue> document() {
        JsonValue value;
        if (!parse(value, 0)) return std::nullopt;
        skip();
        if (at_ != text_.size()) return std::nullopt;
        return value;
    }

private:
    static constexpr int kMaxDepth = 64;

    bool parse(JsonValue& value, int depth) {
        if (depth > kMaxDepth) return false;
        skip();
        if (at_ >= text_.size()) return false;
        const char ch = text_[at_];
        if (ch == '{') return object(value, depth);
        if (ch == '[') return array(value, depth);
        if (ch == '"') {
            value.type = JsonValue::Type::String;
            return string(value.string);
        }
        if (literal("true")) { value = JsonValue::make_bool(true); return true; }
        if (literal("false")) { value = JsonValue::make_bool(false); return true; }
        if (literal("null")) { value = JsonValue{}; return true; }
        return number(value);
    }

    bool object(JsonValue& value, int depth) {
        value = JsonValue::make_object();
        ++at_;
        skip();
        if (eat('}')) return true;
        while (true) {
            skip();
            std::wstring key;
            if (at_ >= text_.size() || text_[at_] != '"' || !string(key)) return false;
            skip();
            if (!eat(':')) return false;
            JsonValue member;
            if (!parse(member, depth + 1)) return false;
            value.object.emplace_back(std::move(key), std::move(member));
            skip();
            if (eat(',')) continue;
            return eat('}');
        }
    }

    bool array(JsonValue& value, int depth) {
        value = JsonValue::make_array();
        ++at_;
        skip();
        if (eat(']')) return true;
        while (true) {
            JsonValue item;
            if (!parse(item, depth + 1)) return false;
            value.array.push_back(std::move(item));
            skip();
            if (eat(',')) continue;
            return eat(']');
        }
    }

    bool number(JsonValue& value) {
        const size_t start = at_;
        if (at_ < text_.size() && (text_[at_] == '-' || text_[at_] == '+')) ++at_;
        while (at_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[at_])) || text_[at_] == '.' ||
                                      text_[at_] == 'e' || text_[at_] == 'E' || text_[at_] == '-' || text_[at_] == '+'))
            ++at_;
        if (start == at_) return false;
        const std::string digits(text_.substr(start, at_ - start));
        char* end = nullptr;
        const double parsed = std::strtod(digits.c_str(), &end);
        if (!end || *end != '\0' || !std::isfinite(parsed)) return false;
        value = JsonValue::make_number(parsed);
        return true;
    }

    static unsigned hex4(std::string_view text) {
        unsigned value = 0;
        for (char ch : text) {
            value <<= 4;
            if (ch >= '0' && ch <= '9') value |= static_cast<unsigned>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') value |= static_cast<unsigned>(ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') value |= static_cast<unsigned>(ch - 'A' + 10);
            else return 0x110000;
        }
        return value;
    }

    bool string(std::wstring& out) {
        ++at_;
        std::string utf8;
        while (at_ < text_.size()) {
            const char ch = text_[at_++];
            if (ch == '"') {
                out += utf8_to_wide(utf8);
                return true;
            }
            if (static_cast<unsigned char>(ch) < 0x20) return false;
            if (ch != '\\') { utf8 += ch; continue; }
            if (at_ >= text_.size()) return false;
            const char escape = text_[at_++];
            switch (escape) {
                case '"': utf8 += '"'; break;
                case '\\': utf8 += '\\'; break;
                case '/': utf8 += '/'; break;
                case 'b': utf8 += '\b'; break;
                case 'f': utf8 += '\f'; break;
                case 'n': utf8 += '\n'; break;
                case 'r': utf8 += '\r'; break;
                case 't': utf8 += '\t'; break;
                case 'u': {
                    if (at_ + 4 > text_.size()) return false;
                    unsigned code = hex4(text_.substr(at_, 4));
                    if (code > 0xFFFF) return false;
                    at_ += 4;
                    std::wstring wide(1, static_cast<wchar_t>(code));
                    if (code >= 0xD800 && code <= 0xDBFF && text_.substr(at_, 2) == "\\u" && at_ + 6 <= text_.size()) {
                        const unsigned low = hex4(text_.substr(at_ + 2, 4));
                        if (low >= 0xDC00 && low <= 0xDFFF) {
                            wide += static_cast<wchar_t>(low);
                            at_ += 6;
                        }
                    }
                    // Flush what came before so the pair converts as one.
                    out += utf8_to_wide(utf8);
                    utf8.clear();
                    out += wide;
                    break;
                }
                default: return false;
            }
        }
        return false;
    }

    bool literal(std::string_view word) {
        if (text_.substr(at_, word.size()) != word) return false;
        at_ += word.size();
        return true;
    }
    void skip() { while (at_ < text_.size() && (text_[at_] == ' ' || text_[at_] == '\t' || text_[at_] == '\r' || text_[at_] == '\n')) ++at_; }
    bool eat(char ch) { if (at_ < text_.size() && text_[at_] == ch) { ++at_; return true; } return false; }

    std::string_view text_;
    size_t at_ = 0;
};

void write(const JsonValue& value, std::string& out) {
    switch (value.type) {
        case JsonValue::Type::Null: out += "null"; break;
        case JsonValue::Type::Bool: out += value.boolean ? "true" : "false"; break;
        case JsonValue::Type::Number: {
            char buffer[32];
            if (value.number == std::floor(value.number) && std::fabs(value.number) < 9.0e15)
                std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(value.number));
            else
                std::snprintf(buffer, sizeof(buffer), "%.17g", value.number);
            out += buffer;
            break;
        }
        case JsonValue::Type::String: out += json_quote(value.string); break;
        case JsonValue::Type::Array:
            out += '[';
            for (size_t index = 0; index < value.array.size(); ++index) {
                if (index) out += ',';
                write(value.array[index], out);
            }
            out += ']';
            break;
        case JsonValue::Type::Object:
            out += '{';
            for (size_t index = 0; index < value.object.size(); ++index) {
                if (index) out += ',';
                out += json_quote(value.object[index].first);
                out += ':';
                write(value.object[index].second, out);
            }
            out += '}';
            break;
    }
}

}  // namespace

JsonValue JsonValue::make_string(std::wstring value) {
    JsonValue result;
    result.type = Type::String;
    result.string = std::move(value);
    return result;
}

JsonValue JsonValue::make_number(double value) {
    JsonValue result;
    result.type = Type::Number;
    result.number = value;
    return result;
}

JsonValue JsonValue::make_bool(bool value) {
    JsonValue result;
    result.type = Type::Bool;
    result.boolean = value;
    return result;
}

JsonValue JsonValue::make_array() {
    JsonValue result;
    result.type = Type::Array;
    return result;
}

JsonValue JsonValue::make_object() {
    JsonValue result;
    result.type = Type::Object;
    return result;
}

const JsonValue* JsonValue::find(std::wstring_view key) const {
    if (type != Type::Object) return nullptr;
    for (const auto& [name, member] : object)
        if (name == key) return &member;
    return nullptr;
}

std::wstring JsonValue::text(std::wstring_view key, std::wstring fallback) const {
    const auto* member = find(key);
    return member && member->type == Type::String ? member->string : fallback;
}

double JsonValue::num(std::wstring_view key, double fallback) const {
    const auto* member = find(key);
    return member && member->type == Type::Number ? member->number : fallback;
}

bool JsonValue::flag(std::wstring_view key, bool fallback) const {
    const auto* member = find(key);
    return member && member->type == Type::Bool ? member->boolean : fallback;
}

JsonValue& JsonValue::set(std::wstring_view key, JsonValue value) {
    if (type != Type::Object) *this = make_object();
    for (auto& [name, member] : object) {
        if (name == key) {
            member = std::move(value);
            return member;
        }
    }
    object.emplace_back(std::wstring(key), std::move(value));
    return object.back().second;
}

std::optional<JsonValue> parse_json(std::string_view utf8) {
    if (utf8.size() >= 3 && static_cast<unsigned char>(utf8[0]) == 0xEF && static_cast<unsigned char>(utf8[1]) == 0xBB &&
        static_cast<unsigned char>(utf8[2]) == 0xBF)
        utf8.remove_prefix(3);
    try {
        return Parser(utf8).document();
    } catch (...) {
        return std::nullopt;
    }
}

std::string write_json(const JsonValue& value) {
    std::string out;
    write(value, out);
    return out;
}

std::string json_quote(std::wstring_view value) {
    // Lone surrogates would not convert: they become U+FFFD.
    std::wstring clean;
    clean.reserve(value.size());
    for (size_t index = 0; index < value.size(); ++index) {
        const wchar_t ch = value[index];
        if (ch >= 0xD800 && ch <= 0xDBFF && index + 1 < value.size() && value[index + 1] >= 0xDC00 && value[index + 1] <= 0xDFFF) {
            clean += ch;
            clean += value[++index];
        } else if (ch >= 0xD800 && ch <= 0xDFFF) {
            clean += L'\xFFFD';
        } else {
            clean += ch;
        }
    }
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char ch : wide_to_utf8(clean)) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (ch < 0x20) { out += "\\u00"; out += hex[ch >> 4]; out += hex[ch & 15]; }
                else out += static_cast<char>(ch);
        }
    }
    out += '"';
    return out;
}

}  // namespace localpinyin
