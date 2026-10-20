#include "input_tools.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>

namespace localpinyin {
namespace {

constexpr wchar_t kUpperDigits[] = L"零壹贰叁肆伍陆柒捌玖";
constexpr wchar_t kLowerDigits[] = L"零一二三四五六七八九";
constexpr wchar_t kYearDigits[] = L"〇一二三四五六七八九";

// Each vowel unmarked, then in tones one to four.
constexpr const wchar_t* kToneVowels[] = {L"aāáǎà", L"eēéěè", L"iīíǐì", L"oōóǒò", L"uūúǔù", L"üǖǘǚǜ"};

const wchar_t* tone_row(wchar_t ch) {
    if (ch == L'v') ch = L'ü';
    for (const wchar_t* row : kToneVowels)
        if (row[0] == ch) return row;
    return nullptr;
}

bool digits_only(const std::wstring& text) {
    return !text.empty() && text.find_first_not_of(L"0123456789") == std::wstring::npos;
}

// A sum: + - * / (also × ÷), ^, % after a number, parentheses.
class Calculator {
public:
    explicit Calculator(const std::wstring& text) : text_(text) {}
    std::optional<double> evaluate() {
        auto value = sum();
        if (!value || at_ != text_.size() || !std::isfinite(*value)) return std::nullopt;
        return value;
    }
    [[nodiscard]] bool used_operator() const noexcept { return operators_ > 0; }

private:
    std::optional<double> sum() {
        auto left = product();
        while (left && at_ < text_.size() && (text_[at_] == L'+' || text_[at_] == L'-')) {
            const wchar_t op = text_[at_++];
            ++operators_;
            const auto right = product();
            if (!right) return std::nullopt;
            left = op == L'+' ? *left + *right : *left - *right;
        }
        return left;
    }
    std::optional<double> product() {
        auto left = power();
        while (left && at_ < text_.size() &&
               (text_[at_] == L'*' || text_[at_] == L'/' || text_[at_] == L'×' || text_[at_] == L'÷')) {
            const wchar_t op = text_[at_++];
            ++operators_;
            const auto right = power();
            if (!right) return std::nullopt;
            if ((op == L'/' || op == L'÷') && *right == 0.0) return std::nullopt;
            left = op == L'*' || op == L'×' ? *left * *right : *left / *right;
        }
        return left;
    }
    std::optional<double> power() {
        auto base = unary();
        if (base && at_ < text_.size() && text_[at_] == L'^') {
            ++at_;
            ++operators_;
            const auto exponent = power();
            if (!exponent) return std::nullopt;
            return std::pow(*base, *exponent);
        }
        return base;
    }
    std::optional<double> unary() {
        if (at_ < text_.size() && (text_[at_] == L'-' || text_[at_] == L'+')) {
            const bool negative = text_[at_++] == L'-';
            auto value = unary();
            if (!value) return std::nullopt;
            return negative ? -*value : *value;
        }
        auto value = primary();
        while (value && at_ < text_.size() && text_[at_] == L'%') {
            ++at_;
            ++operators_;
            *value /= 100.0;
        }
        return value;
    }
    std::optional<double> primary() {
        if (at_ < text_.size() && text_[at_] == L'(') {
            ++at_;
            auto value = sum();
            if (!value || at_ >= text_.size() || text_[at_] != L')') return std::nullopt;
            ++at_;
            return value;
        }
        const size_t start = at_;
        bool dot = false;
        while (at_ < text_.size() && ((text_[at_] >= L'0' && text_[at_] <= L'9') || (text_[at_] == L'.' && !dot))) {
            dot = dot || text_[at_] == L'.';
            ++at_;
        }
        if (at_ == start || (at_ - start == 1 && dot)) return std::nullopt;
        return std::wcstod(text_.substr(start, at_ - start).c_str(), nullptr);
    }

    const std::wstring& text_;
    size_t at_ = 0;
    int operators_ = 0;
};

std::wstring format_number(double value) {
    wchar_t buffer[64]{};
    if (std::fabs(value) < 1e15 && value == std::floor(value)) {
        std::swprintf(buffer, 64, L"%.0f", value);
    } else {
        std::swprintf(buffer, 64, L"%.12g", value);
    }
    std::wstring text = buffer;
    return text == L"-0" ? L"0" : text;
}

// Up to four digits with their units (千百十), upper or lower case.
std::wstring section(const std::wstring& digits, const wchar_t* numerals, const wchar_t* units, bool lower) {
    std::wstring result;
    bool zero = false;
    const size_t n = digits.size();
    for (size_t index = 0; index < n; ++index) {
        const int digit = digits[index] - L'0';
        const size_t place = n - 1 - index;
        if (digit == 0) {
            zero = !result.empty();
            continue;
        }
        if (zero) result += numerals[0];
        zero = false;
        // 一十二 reads 十二 at the start of a number.
        if (!(lower && digit == 1 && place == 1 && result.empty() && n == 2)) result += numerals[digit];
        if (place > 0) result += units[place - 1];
    }
    return result;
}

std::wstring integer_words(std::wstring digits, bool upper) {
    while (digits.size() > 1 && digits.front() == L'0') digits.erase(0, 1);
    if (digits.size() > 16) return {};
    const wchar_t* numerals = upper ? kUpperDigits : kLowerDigits;
    const wchar_t* units = upper ? L"拾佰仟" : L"十百千";
    if (digits == L"0") return std::wstring(1, numerals[0]);
    static const wchar_t* const kBig[] = {L"", L"万", L"亿", L"万亿"};
    std::vector<std::wstring> groups;
    for (size_t end = digits.size(); end > 0; end = end >= 4 ? end - 4 : 0)
        groups.insert(groups.begin(), digits.substr(end >= 4 ? end - 4 : 0, end >= 4 ? 4 : end));
    std::wstring result;
    bool pending_zero = false;
    for (size_t index = 0; index < groups.size(); ++index) {
        const auto& group = groups[index];
        const bool first = index == 0;
        const std::wstring words = section(group, numerals, units, !upper && first);
        if (words.empty()) {
            pending_zero = !result.empty();
            continue;
        }
        // A gap before a group: 1,0005 reads 一万零五.
        if (!result.empty() && (pending_zero || group.front() == L'0')) result += numerals[0];
        pending_zero = false;
        result += words + kBig[groups.size() - 1 - index];
    }
    return result;
}

bool split_number(const std::wstring& number, std::wstring* integer, std::wstring* fraction) {
    const auto dot = number.find(L'.');
    *integer = number.substr(0, dot);
    *fraction = dot == std::wstring::npos ? L"" : number.substr(dot + 1);
    return digits_only(*integer) && (fraction->empty() ? dot == std::wstring::npos : digits_only(*fraction));
}

std::wstring month_day_words(int month, int day) {
    return integer_words(std::to_wstring(month), false) + L"月" + integer_words(std::to_wstring(day), false) + L"日";
}

std::wstring year_words(const std::wstring& year) {
    std::wstring result;
    for (wchar_t ch : year) result += kYearDigits[ch - L'0'];
    return result + L"年";
}

std::wstring two(int value) {
    wchar_t buffer[8]{};
    std::swprintf(buffer, 8, L"%02d", value);
    return buffer;
}

bool valid_day(int year, int month, int day) {
    static const int kDays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12 || day < 1 || day > kDays[month - 1]) return false;
    const bool leap = year <= 0 || (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    return month != 2 || day <= 28 || leap;
}

// 12.19, 12-19, 12/19 and 2024.12.19 (any of . - / between).
std::vector<std::wstring> dates(const std::wstring& text) {
    std::vector<std::wstring> parts;
    std::wstring current;
    for (wchar_t ch : text) {
        if (ch == L'.' || ch == L'-' || ch == L'/') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    parts.push_back(current);
    for (const auto& part : parts) if (!digits_only(part) || part.size() > 4) return {};
    if (parts.size() == 2 && parts[0].size() <= 2 && parts[1].size() <= 2) {
        const int month = std::stoi(parts[0]), day = std::stoi(parts[1]);
        if (!valid_day(0, month, day)) return {};
        return {month_day_words(month, day), std::to_wstring(month) + L"月" + std::to_wstring(day) + L"日"};
    }
    if (parts.size() == 3 && parts[0].size() == 4 && parts[1].size() <= 2 && parts[2].size() <= 2) {
        const int year = std::stoi(parts[0]), month = std::stoi(parts[1]), day = std::stoi(parts[2]);
        if (!valid_day(year, month, day)) return {};
        return {year_words(parts[0]) + month_day_words(month, day),
                parts[0] + L"年" + std::to_wstring(month) + L"月" + std::to_wstring(day) + L"日",
                parts[0] + L"-" + two(month) + L"-" + two(day)};
    }
    return {};
}

}  // namespace

bool is_tool_text(const std::wstring& text) {
    bool digit = false;
    for (wchar_t ch : text) {
        if (ch >= L'0' && ch <= L'9') digit = true;
        else if (std::wstring(L".+-*/()^%×÷").find(ch) == std::wstring::npos) return false;
    }
    return digit;
}

std::wstring chinese_upper_number(const std::wstring& number) {
    std::wstring integer, fraction;
    if (!split_number(number, &integer, &fraction)) return {};
    std::wstring result = integer_words(integer, true);
    if (result.empty()) return {};
    if (!fraction.empty()) {
        result += L"点";
        for (wchar_t ch : fraction) result += kUpperDigits[ch - L'0'];
    }
    return result;
}

std::wstring chinese_lower_number(const std::wstring& number) {
    std::wstring integer, fraction;
    if (!split_number(number, &integer, &fraction)) return {};
    std::wstring result = integer_words(integer, false);
    if (result.empty()) return {};
    if (!fraction.empty()) {
        result += L"点";
        for (wchar_t ch : fraction) result += kLowerDigits[ch - L'0'];
    }
    return result;
}

std::wstring chinese_amount(const std::wstring& number) {
    std::wstring integer, fraction;
    if (!split_number(number, &integer, &fraction)) return {};
    const int jiao = fraction.size() > 0 ? fraction[0] - L'0' : 0;
    const int fen = fraction.size() > 1 ? fraction[1] - L'0' : 0;
    std::wstring whole = integer_words(integer, true);
    if (whole.empty()) return {};
    const bool no_yuan = whole == L"零";
    std::wstring result = no_yuan && (jiao || fen) ? L"" : whole + L"元";
    if (!jiao && !fen) return result + L"整";
    if (jiao) result += std::wstring(1, kUpperDigits[jiao]) + L"角";
    else if (!no_yuan) result += L"零";
    if (fen) result += std::wstring(1, kUpperDigits[fen]) + L"分";
    return result;
}

std::vector<std::wstring> tool_results(const std::wstring& text, const std::tm&) {
    std::vector<std::wstring> results;
    const auto add = [&results](const std::wstring& value) {
        if (!value.empty() && std::find(results.begin(), results.end(), value) == results.end()) results.push_back(value);
    };
    if (!is_tool_text(text)) return results;
    for (const auto& date : dates(text)) add(date);
    Calculator calculator(text);
    const auto value = calculator.evaluate();
    if (value && calculator.used_operator()) {
        const std::wstring answer = format_number(*value);
        add(answer);
        add(text + L"=" + answer);
        return results;
    }
    std::wstring integer, fraction;
    if (split_number(text, &integer, &fraction)) {
        add(chinese_upper_number(text));
        add(chinese_lower_number(text));
        if (fraction.size() <= 2) add(chinese_amount(text));
    }
    return results;
}

bool is_tone_text(const std::wstring& text) {
    if (text.empty() || text.size() > 32) return false;
    for (wchar_t ch : text)
        if (!(ch >= L'a' && ch <= L'z') && ch != L'ü') return false;
    return tone_vowel_count(text) > 0;
}

size_t tone_vowel_count(const std::wstring& text) {
    return static_cast<size_t>(std::count_if(text.begin(), text.end(), [](wchar_t ch) { return tone_row(ch) != nullptr; }));
}

std::wstring tone_text(const std::wstring& text, const std::vector<int>& tones) {
    std::wstring result;
    size_t vowel = 0;
    for (wchar_t ch : text) {
        const wchar_t* row = tone_row(ch);
        if (!row) { result += ch; continue; }
        const int tone = vowel < tones.size() ? tones[vowel] : 0;
        result += row[tone >= 1 && tone <= 4 ? tone : 0];
        ++vowel;
    }
    return result;
}

std::vector<std::wstring> time_shortcuts(const std::wstring& pinyin, const std::tm& now) {
    const int year = now.tm_year + 1900, month = now.tm_mon + 1, day = now.tm_mday;
    const std::wstring date = std::to_wstring(year) + L"-" + two(month) + L"-" + two(day);
    const std::wstring clock = two(now.tm_hour) + L":" + two(now.tm_min) + L":" + two(now.tm_sec);
    const std::wstring chinese_date = std::to_wstring(year) + L"年" + std::to_wstring(month) + L"月" + std::to_wstring(day) + L"日";
    static const wchar_t* const kWeekdays[] = {L"日", L"一", L"二", L"三", L"四", L"五", L"六"};
    const std::wstring weekday = kWeekdays[now.tm_wday % 7];
    if (pinyin == L"sj") return {date + L" " + clock, clock, chinese_date + L" " + two(now.tm_hour) + L":" + two(now.tm_min)};
    if (pinyin == L"rq") return {date, chinese_date, year_words(std::to_wstring(year)) + month_day_words(month, day)};
    if (pinyin == L"xq") return {L"星期" + weekday, L"周" + weekday};
    return {};
}

}  // namespace localpinyin
