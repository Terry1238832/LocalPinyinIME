#include "test_common.h"
#include "../src/common/json_lite.h"

#include <string>

int main() {
    using localpinyin::JsonValue;
    using localpinyin::parse_json;
    using localpinyin::write_json;

    // The panel's favorites file as the settings app writes it.
    const auto state = parse_json(R"({"favorites":{"symbol":["≈","√"],"emoji":[]},"recents":{"symbol":["$"]}})");
    REQUIRE_TRUE(state.has_value());
    const JsonValue* favorites = state->find(L"favorites");
    REQUIRE_TRUE(favorites && favorites->is_object());
    REQUIRE_EQ(favorites->find(L"symbol")->array.size(), size_t{2});
    REQUIRE_EQ(favorites->find(L"symbol")->array[1].string, std::wstring(L"√"));

    // Escapes, surrogate pairs, numbers and literals.
    const auto mixed = parse_json(" [ \"a\\\"b\\\\c\\n\\u00e9\\ud83d\\ude00\" , -12.5e1 , true , false , null , {} ] ");
    REQUIRE_TRUE(mixed.has_value() && mixed->is_array());
    REQUIRE_EQ(mixed->array[0].string, std::wstring(L"a\"b\\c\né\U0001F600"));
    REQUIRE_EQ(mixed->array[1].number, -125.0);
    REQUIRE_TRUE(mixed->array[2].boolean && !mixed->array[3].boolean);
    REQUIRE_TRUE(mixed->array[4].type == JsonValue::Type::Null && mixed->array[5].is_object());

    // A byte order mark is allowed; broken documents are not.
    REQUIRE_TRUE(parse_json("\xEF\xBB\xBF{\"a\":1}").has_value());
    for (const char* broken : {"", "{", "[1,]", "{\"a\" 1}", "\"open", "[1] 2", "{\"a\":\"\x01\"}", "nul"})
        REQUIRE_TRUE(!parse_json(broken).has_value());
    std::string deep(200, '[');
    deep += std::string(200, ']');
    REQUIRE_TRUE(!parse_json(deep).has_value());

    // Written back, it reads the same.
    JsonValue item = JsonValue::make_object();
    item.set(L"title", JsonValue::make_string(L"QQ 邮箱\t\"工作\""));
    item.set(L"count", JsonValue::make_number(3));
    item.set(L"secret", JsonValue::make_bool(true));
    item.set(L"title", JsonValue::make_string(L"改过的标题 😀"));
    const auto again = parse_json(write_json(item));
    REQUIRE_TRUE(again.has_value());
    REQUIRE_EQ(again->text(L"title"), std::wstring(L"改过的标题 😀"));
    REQUIRE_EQ(again->num(L"count"), 3.0);
    REQUIRE_TRUE(again->flag(L"secret"));
    REQUIRE_EQ(again->object.size(), size_t{3});
    REQUIRE_EQ(write_json(JsonValue::make_number(1792373931606.0)), std::string("1792373931606"));
    return 0;
}
