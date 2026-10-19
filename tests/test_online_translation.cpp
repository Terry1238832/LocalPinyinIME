#include "test_common.h"
#include "../src/ime/online_translation.h"

#include <string>

int main() {
    using localpinyin::parse_google_translation;
    using localpinyin::percent_encode_utf8;

    // The query value: UTF-8 bytes, everything but unreserved characters encoded.
    REQUIRE_EQ(percent_encode_utf8(L"你好"), std::string("%E4%BD%A0%E5%A5%BD"));
    REQUIRE_EQ(percent_encode_utf8(L"a b&c=d~"), std::string("a%20b%26c%3Dd~"));
    REQUIRE_EQ(percent_encode_utf8(L"3.14，好"), std::string("3.14%EF%BC%8C%E5%A5%BD"));
    REQUIRE_EQ(percent_encode_utf8(L"😀"), std::string("%F0%9F%98%80"));

    // One sentence, as the endpoint returns it.
    auto one = parse_google_translation(
        R"([[["Hello World","你好世界",null,null,10]],null,"zh-CN",null,null,null,null,[]])");
    REQUIRE_TRUE(one.has_value());
    REQUIRE_EQ(*one, std::wstring(L"Hello World"));

    // Several sentences are joined; escapes and \u sequences are decoded.
    auto several = parse_google_translation(
        "[[[\"I want to eat. \",\"\xE6\x88\x91\xE6\x83\xB3\xE5\x90\x83\xE3\x80\x82\",null,null,3],"
        "[\"\\\"Rice\\\" \\u00e9\\ud83d\\ude00\\n\",\"x\",null,null,3,null,null,[[]],[[\"k\",\"v\"]]]],"
        "null,\"zh-CN\",null,null,null,1,[],[[\"zh-CN\"],null,[1],[\"zh-CN\"]]]");
    REQUIRE_TRUE(several.has_value());
    REQUIRE_EQ(*several, std::wstring(L"I want to eat. \"Rice\" é\U0001F600\n"));

    // Objects and numbers inside a sentence are read past.
    auto nested = parse_google_translation(R"( [ [ [ "OK" , "好" , {"a":[1,2,{"b":null}]} , -1.5e3 , true ] ] ] )");
    REQUIRE_TRUE(nested.has_value());
    REQUIRE_EQ(*nested, std::wstring(L"OK"));

    // Anything else is not a translation.
    REQUIRE_TRUE(!parse_google_translation("").has_value());
    REQUIRE_TRUE(!parse_google_translation("<html>blocked</html>").has_value());
    REQUIRE_TRUE(!parse_google_translation("[null,null,\"zh-CN\"]").has_value());
    REQUIRE_TRUE(!parse_google_translation("[[]]").has_value());
    REQUIRE_TRUE(!parse_google_translation("[[[\"unterminated").has_value());
    REQUIRE_TRUE(!parse_google_translation("[[[\"bad \\ud83d escape\"]]]").has_value());
    std::wcout << L"PASS: online translation encoding and response parsing\n";
    return 0;
}
