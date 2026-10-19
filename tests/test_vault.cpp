#include "test_common.h"
#include "../src/vault/vault.h"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

int main() {
    using localpinyin::Vault;
    using localpinyin::VaultField;
    using localpinyin::VaultItem;

    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    const auto folder = std::filesystem::path(temp) / (L"lp-vault-" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const Vault vault((folder / L"vault.json").wstring());

    REQUIRE_TRUE(!vault.exists());
    REQUIRE_TRUE(!vault.has_password());
    std::vector<VaultItem> items;
    REQUIRE_TRUE(!vault.open(L"", &items));

    // With a password: only it opens the vault.
    REQUIRE_TRUE(vault.create(L"correct horse"));
    REQUIRE_TRUE(!vault.create(L"again"));
    REQUIRE_TRUE(vault.exists() && vault.has_password());
    REQUIRE_TRUE(vault.open(L"correct horse", &items));
    REQUIRE_TRUE(items.empty());
    REQUIRE_TRUE(!vault.open(L"wrong", &items));
    REQUIRE_TRUE(!vault.open(L"", &items));

    VaultItem mail{Vault::new_id(), L"password", L"QQ 邮箱",
                   {VaultField{L"邮箱", L"me@qq.com", false}, VaultField{L"密码", L"p@ss 密码😀", true}}, 100, 200};
    VaultItem home{Vault::new_id(), L"address", L"家",
                   {VaultField{L"收件人", L"张三", false}, VaultField{L"地址", L"北京市海淀区\n某路 1 号", false}}, 50, 60};
    REQUIRE_EQ(mail.id.size(), size_t{16});
    REQUIRE_TRUE(mail.id != home.id);
    REQUIRE_TRUE(vault.save(L"correct horse", {mail, home}));
    REQUIRE_TRUE(!vault.save(L"wrong", {mail}));

    // Nothing readable on disk.
    std::ifstream file(folder / L"vault.json", std::ios::binary);
    const std::string disk((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    REQUIRE_TRUE(disk.find("me@qq.com") == std::string::npos);
    REQUIRE_TRUE(disk.find("\"lock\":\"password\"") != std::string::npos);

    REQUIRE_TRUE(vault.open(L"correct horse", &items));
    REQUIRE_EQ(items.size(), size_t{2});
    REQUIRE_EQ(items[0].title, std::wstring(L"QQ 邮箱"));
    REQUIRE_EQ(items[0].fields[1].value, std::wstring(L"p@ss 密码😀"));
    REQUIRE_TRUE(items[0].fields[1].secret);
    REQUIRE_EQ(items[1].kind, std::wstring(L"address"));
    REQUIRE_EQ(items[1].fields[1].value, std::wstring(L"北京市海淀区\n某路 1 号"));
    REQUIRE_EQ(items[1].updated, 60LL);

    // A new password opens it, the old one no longer does; the items stay.
    REQUIRE_TRUE(!vault.change_password(L"wrong", L"new one"));
    REQUIRE_TRUE(vault.change_password(L"correct horse", L"new one"));
    REQUIRE_TRUE(!vault.open(L"correct horse", &items));
    REQUIRE_TRUE(vault.open(L"new one", &items));
    REQUIRE_EQ(items.size(), size_t{2});

    // Without a password: Windows keeps the key for this account.
    REQUIRE_TRUE(vault.change_password(L"new one", L""));
    REQUIRE_TRUE(!vault.has_password());
    REQUIRE_TRUE(vault.open(L"", &items));
    REQUIRE_EQ(items[0].fields[0].value, std::wstring(L"me@qq.com"));
    REQUIRE_TRUE(vault.save(L"", {home}));
    REQUIRE_TRUE(vault.open(L"anything", &items));
    REQUIRE_EQ(items.size(), size_t{1});
    // And back to a password.
    REQUIRE_TRUE(vault.change_password(L"", L"third"));
    REQUIRE_TRUE(vault.has_password());
    REQUIRE_TRUE(vault.open(L"third", &items));

    // A damaged file opens with no password.
    std::ofstream damaged(folder / L"vault.json", std::ios::binary | std::ios::trunc);
    damaged << "{\"format\":\"localpinyin-vault\",\"version\":1,\"lock\":\"password\",\"key\":\"AAAA\",\"salt\":\"AAAAAAAAAAAAAAAAAAAAAA==\",\"iterations\":1000,\"items\":\"AAAA\"}";
    damaged.close();
    REQUIRE_TRUE(!vault.open(L"third", &items));

    REQUIRE_TRUE(vault.destroy());
    REQUIRE_TRUE(!vault.exists());
    REQUIRE_TRUE(vault.create(L""));
    REQUIRE_TRUE(vault.open(L"", &items) && items.empty());

    // Items for the settings bridge.
    VaultItem parsed;
    REQUIRE_TRUE(localpinyin::parse_vault_item(R"({"id":"x","kind":"weird","title":"卡","fields":[{"name":"卡号","value":"6222","secret":true}]})", &parsed));
    REQUIRE_EQ(parsed.kind, std::wstring(L"other"));
    REQUIRE_TRUE(parsed.fields[0].secret);
    // Written as UTF-8 text, not \u escapes.
    REQUIRE_TRUE(localpinyin::vault_items_json({parsed}).find("\"卡号\"") != std::string::npos);
    std::filesystem::remove_all(folder);
    return 0;
}
