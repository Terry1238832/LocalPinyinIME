#include "online_translation.h"

#include "../common/utf_utils.h"

#include <windows.h>
#include <winhttp.h>

#include <cstdint>

namespace localpinyin {
namespace {

// The endpoint answers in under a second; past this the user is better off
// knowing it did not work.
constexpr int kResolveTimeoutMs = 4000;
constexpr int kConnectTimeoutMs = 4000;
constexpr int kSendTimeoutMs = 4000;
constexpr int kReceiveTimeoutMs = 6000;
constexpr size_t kMaxResponseBytes = 1 << 20;

class Handle {
public:
    explicit Handle(HINTERNET value = nullptr) noexcept : value_(value) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle() { if (value_) WinHttpCloseHandle(value_); }
    [[nodiscard]] HINTERNET get() const noexcept { return value_; }
    explicit operator bool() const noexcept { return value_ != nullptr; }
private:
    HINTERNET value_;
};

std::wstring connection_error(DWORD error) {
    switch (error) {
        case ERROR_WINHTTP_TIMEOUT:
            return L"翻译服务没有响应，请稍后再试。";
        case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        case ERROR_WINHTTP_CANNOT_CONNECT:
        case ERROR_WINHTTP_CONNECTION_ERROR:
            return L"连不上谷歌翻译，请检查网络。";
        default:
            return L"翻译失败（网络错误 " + std::to_wstring(error) + L"）。";
    }
}

// A JSON reader for the one shape the endpoint returns.
class Reader {
public:
    explicit Reader(const std::string& text) : text_(text) {}

    void skip_space() {
        while (at_ < text_.size() && (text_[at_] == ' ' || text_[at_] == '\n' || text_[at_] == '\r' || text_[at_] == '\t'))
            ++at_;
    }
    bool take(char ch) {
        skip_space();
        if (at_ < text_.size() && text_[at_] == ch) { ++at_; return true; }
        return false;
    }
    [[nodiscard]] bool peek(char ch) {
        skip_space();
        return at_ < text_.size() && text_[at_] == ch;
    }
    // A string as UTF-8, escapes decoded.
    bool string(std::string* out) {
        if (!take('"')) return false;
        while (at_ < text_.size()) {
            const char ch = text_[at_++];
            if (ch == '"') return true;
            if (ch != '\\') { out->push_back(ch); continue; }
            if (at_ >= text_.size()) return false;
            const char escaped = text_[at_++];
            switch (escaped) {
                case '"': out->push_back('"'); break;
                case '\\': out->push_back('\\'); break;
                case '/': out->push_back('/'); break;
                case 'b': out->push_back('\b'); break;
                case 'f': out->push_back('\f'); break;
                case 'n': out->push_back('\n'); break;
                case 'r': out->push_back('\r'); break;
                case 't': out->push_back('\t'); break;
                case 'u': {
                    std::uint32_t code = 0;
                    if (!hex4(&code)) return false;
                    if (code >= 0xD800 && code <= 0xDBFF) {
                        std::uint32_t low = 0;
                        if (at_ + 1 < text_.size() && text_[at_] == '\\' && text_[at_ + 1] == 'u') {
                            at_ += 2;
                            if (!hex4(&low) || low < 0xDC00 || low > 0xDFFF) return false;
                            code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                        } else {
                            return false;
                        }
                    }
                    append_utf8(out, code);
                    break;
                }
                default:
                    return false;
            }
        }
        return false;
    }
    // Any value, read past.
    bool skip_value() {
        skip_space();
        if (at_ >= text_.size()) return false;
        const char ch = text_[at_];
        if (ch == '"') { std::string ignored; return string(&ignored); }
        if (ch == '[' || ch == '{') {
            const char close = ch == '[' ? ']' : '}';
            ++at_;
            if (take(close)) return true;
            do {
                if (close == '}') {
                    std::string key;
                    if (!string(&key) || !take(':')) return false;
                }
                if (!skip_value()) return false;
            } while (take(','));
            return take(close);
        }
        // A number, true, false or null.
        const size_t start = at_;
        while (at_ < text_.size() && text_[at_] != ',' && text_[at_] != ']' && text_[at_] != '}' &&
               text_[at_] != ' ' && text_[at_] != '\n' && text_[at_] != '\r' && text_[at_] != '\t')
            ++at_;
        return at_ > start;
    }

private:
    bool hex4(std::uint32_t* code) {
        if (at_ + 4 > text_.size()) return false;
        *code = 0;
        for (int i = 0; i < 4; ++i) {
            const char ch = text_[at_++];
            *code <<= 4;
            if (ch >= '0' && ch <= '9') *code |= static_cast<std::uint32_t>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') *code |= static_cast<std::uint32_t>(ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') *code |= static_cast<std::uint32_t>(ch - 'A' + 10);
            else return false;
        }
        return true;
    }
    static void append_utf8(std::string* out, std::uint32_t code) {
        if (code < 0x80) {
            out->push_back(static_cast<char>(code));
        } else if (code < 0x800) {
            out->push_back(static_cast<char>(0xC0 | (code >> 6)));
            out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else if (code < 0x10000) {
            out->push_back(static_cast<char>(0xE0 | (code >> 12)));
            out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            out->push_back(static_cast<char>(0xF0 | (code >> 18)));
            out->push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
            out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }

    const std::string& text_;
    size_t at_ = 0;
};

}  // namespace

std::string percent_encode_utf8(const std::wstring& text) {
    static const char* const kHex = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char ch : wide_to_utf8(text)) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            result.push_back(static_cast<char>(ch));
        } else {
            result.push_back('%');
            result.push_back(kHex[ch >> 4]);
            result.push_back(kHex[ch & 0x0F]);
        }
    }
    return result;
}

// [[["Hello World","你好世界",null,null,10],["...","..."]],null,"zh-CN",...]
std::optional<std::wstring> parse_google_translation(const std::string& body) {
    Reader reader(body);
    if (!reader.take('[') || !reader.take('[')) return std::nullopt;
    std::string translated;
    if (!reader.take(']')) {
        do {
            if (!reader.peek('[')) {
                if (!reader.skip_value()) return std::nullopt;
                continue;
            }
            reader.take('[');
            if (reader.peek('"')) {
                if (!reader.string(&translated)) return std::nullopt;
            } else if (!reader.skip_value()) {
                return std::nullopt;
            }
            while (reader.take(',')) {
                if (!reader.skip_value()) return std::nullopt;
            }
            if (!reader.take(']')) return std::nullopt;
        } while (reader.take(','));
        if (!reader.take(']')) return std::nullopt;
    }
    if (translated.empty()) return std::nullopt;
    return utf8_to_wide(translated);
}

TranslationResult translate_with_google(const std::wstring& chinese) {
    TranslationResult result;
    Handle session(WinHttpOpen(L"LocalPinyinIME", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) {
        result.error = connection_error(GetLastError());
        return result;
    }
    WinHttpSetTimeouts(session.get(), kResolveTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs, kReceiveTimeoutMs);
    Handle connection(WinHttpConnect(session.get(), L"translate.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    const std::wstring path = utf8_to_wide("/translate_a/single?client=gtx&sl=zh-CN&tl=en&dt=t&q=" +
                                           percent_encode_utf8(chinese));
    Handle request(connection ? WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                                   WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
                              : nullptr);
    if (!request || !WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.get(), nullptr)) {
        result.error = connection_error(GetLastError());
        return result;
    }
    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &status, &size, WINHTTP_NO_HEADER_INDEX);
    if (status != 200) {
        result.error = status == 429 || status == 403 ? L"谷歌翻译暂时拒绝了请求，请稍后再试。"
                                                      : L"翻译失败（HTTP " + std::to_wstring(status) + L"）。";
        return result;
    }
    std::string body;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            result.error = connection_error(GetLastError());
            return result;
        }
        if (available == 0) break;
        const size_t offset = body.size();
        body.resize(offset + available);
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), body.data() + offset, available, &read)) {
            result.error = connection_error(GetLastError());
            return result;
        }
        body.resize(offset + read);
        if (body.size() > kMaxResponseBytes) break;
    }
    const auto translated = parse_google_translation(body);
    if (!translated) {
        result.error = L"翻译服务返回了无法识别的内容。";
        return result;
    }
    result.ok = true;
    result.text = *translated;
    return result;
}

}  // namespace localpinyin
