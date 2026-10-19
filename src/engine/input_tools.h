#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace localpinyin {

// Typing tools, also on the tool panel's 工具 page.
//
// After v: a sum (v55+15 → 70), a number (v123 → 壹佰贰拾叁, 一百二十三,
// 壹佰贰拾叁元整) or a date (v12.19 → 十二月十九日). Results come best first;
// none when the text is not one of these (v5+ while still typing).
[[nodiscard]] bool is_tool_text(const std::wstring& text);
[[nodiscard]] std::vector<std::wstring> tool_results(const std::wstring& text, const std::tm& now);

// sj, rq and xq: the time, the date and the weekday now; none otherwise.
[[nodiscard]] std::vector<std::wstring> time_shortcuts(const std::wstring& pinyin, const std::tm& now);

// Digits ("123", "1.5") in words: 壹佰贰拾叁 (upper case, for amounts on
// cheques), 一百二十三, and 壹佰贰拾叁元整 (yuan, jiao, fen). Empty when
// the number is out of range (more than 16 integer digits).
[[nodiscard]] std::wstring chinese_upper_number(const std::wstring& number);
[[nodiscard]] std::wstring chinese_lower_number(const std::wstring& number);
[[nodiscard]] std::wstring chinese_amount(const std::wstring& number);

}  // namespace localpinyin
