#include "test_common.h"
#include "../src/common/logging.h"

#include <string>

int main() {
    SYSTEMTIME timestamp{};
    timestamp.wYear = 2026;
    timestamp.wMonth = 7;
    timestamp.wDay = 4;
    timestamp.wHour = 9;
    timestamp.wMinute = 57;
    timestamp.wSecond = 54;

    {
        const std::wstring line = localpinyin::format_status_log_line(timestamp, L"", L"");
        REQUIRE_EQ(line, std::wstring(L"2026-07-04 09:57:54 [] \r\n"));
    }

    const std::wstring long_path(700, L'x');
    const std::wstring diagnostic =
        L"stage=RegisterCategory operation=RegisterCategory hr=0x80070490 "
        L"category_name=GUID_TFCAT_TIPCAP_UIELEMENTENABLED "
        L"category_guid={49D2F9CF-1F5E-11D7-A6D3-00065B84435C} "
        L"item_guid={7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A} "
        L"target_dll=C:\\Program Files\\LocalPinyinIME\\releases\\0.3.13-dev\\x64\\LocalPinyinIME.dll "
        L"rollback_attempted=TRUE rollback_succeeded=TRUE "
        L"unicode_path=C:\\\u7528\u6237\\\u672C\u5730\u62FC\u97F3\\" +
        long_path;

    const std::wstring line =
        localpinyin::format_status_log_line(timestamp, L"registration", diagnostic);
    REQUIRE_TRUE(line.starts_with(L"2026-07-04 09:57:54 [registration] "));
    REQUIRE_TRUE(line.ends_with(L"\r\n"));
    REQUIRE_TRUE(line.find(L"GUID_TFCAT_TIPCAP_UIELEMENTENABLED") != std::wstring::npos);
    REQUIRE_TRUE(line.find(L"{49D2F9CF-1F5E-11D7-A6D3-00065B84435C}") != std::wstring::npos);
    REQUIRE_TRUE(line.find(L"0x80070490") != std::wstring::npos);
    REQUIRE_TRUE(line.find(L"\u7528\u6237") != std::wstring::npos);
    REQUIRE_TRUE(line.find(long_path) != std::wstring::npos);
    REQUIRE_TRUE(line.size() > 512);

    return 0;
}
