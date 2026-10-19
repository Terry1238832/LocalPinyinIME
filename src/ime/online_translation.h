#pragma once

// Tab to English: text the user just typed is translated online and replaces
// the original. It is off unless the user turns it on, and only the text being
// translated is sent, only when Tab is pressed; everything else the input
// method does stays on this computer.

#include <functional>
#include <optional>
#include <string>

namespace localpinyin {

struct TranslationResult {
    bool ok = false;
    std::wstring text;
    // Why it failed, as shown to the user.
    std::wstring error;
};

// Translates Chinese to English. Runs on a background thread.
using Translator = std::function<TranslationResult(const std::wstring& chinese)>;

// Google Translate's public web endpoint (no key), with the system proxy.
TranslationResult translate_with_google(const std::wstring& chinese);

// The query string value: UTF-8, percent-encoded.
[[nodiscard]] std::string percent_encode_utf8(const std::wstring& text);
// The translation in a response of the endpoint above: the first element of
// each sentence, joined. Empty when the body is not such a response.
[[nodiscard]] std::optional<std::wstring> parse_google_translation(const std::string& body);

}  // namespace localpinyin
