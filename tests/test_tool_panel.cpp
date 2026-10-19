#include "test_common.h"
#include "../src/ime/tool_panel.h"

#include <windows.h>

#include <filesystem>
#include <string>

namespace {

using localpinyin::PanelAction;
using localpinyin::PanelHit;
using localpinyin::PanelKey;
using localpinyin::PanelPage;
using localpinyin::ToolPanel;

PanelKey vk(UINT code, bool shift = false, bool ctrl = false) { return PanelKey{code, 0, shift, ctrl}; }
PanelKey ch(wchar_t character) {
    const UINT code = character >= L'a' && character <= L'z' ? character - L'a' + L'A' : character;
    return PanelKey{code, character, false, false};
}
void type(ToolPanel& panel, const std::wstring& text) {
    for (wchar_t character : text) {
        PanelKey key = ch(character);
        if (character == L'+') key.vk = VK_OEM_PLUS, key.shift = true;
        if (character == L'.') key.vk = VK_OEM_PERIOD;
        panel.key(key);
    }
}
bool has_cell(const localpinyin::ToolPanelView& view, const std::wstring& text) {
    for (const auto& cell : view.cells) if (cell.text == text) return true;
    return false;
}

}  // namespace

int main() {
    wchar_t module[MAX_PATH]{};
    GetModuleFileNameW(nullptr, module, MAX_PATH);
    const auto resources = std::filesystem::path(module).parent_path() / L"dictionary";
    wchar_t temp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, temp);
    const auto data = std::filesystem::path(temp) / (L"lp-panel-" + std::to_wstring(GetCurrentProcessId()));
    std::filesystem::remove_all(data);
    std::filesystem::create_directories(data);

    long long now = 1'800'000'000;
    ToolPanel::Services services;
    services.data_dir = data.wstring();
    services.resource_dir = resources.wstring();
    services.words = [](const std::wstring& pinyin) -> std::vector<std::wstring> {
        if (pinyin == L"meiyuan") return {L"美元", L"美圆"};
        if (pinyin == L"kaixin") return {L"开心"};
        if (pinyin == L"huiyi") return {L"会议"};
        return {};
    };
    services.clipboard_on = [] { return true; };
    services.now = [&] { return now; };
    ToolPanel panel(services);

    // 符号: 常用数学符号 first; numbers pick in the current row; Enter types them.
    panel.open(PanelPage::Symbol);
    auto view = panel.view();
    REQUIRE_TRUE(view.grid && view.columns == 10 && view.rows == 4);
    REQUIRE_EQ(view.categories.at(0), std::wstring(L"常用数学符号"));
    REQUIRE_EQ(view.categories.at(1), std::wstring(L"常规标点"));
    REQUIRE_EQ(view.cells.at(0).text, std::wstring(L"+"));
    REQUIRE_EQ(view.numbered_row, 0);
    panel.key(vk(L'9'));
    panel.key(vk(VK_RIGHT));
    panel.key(vk(VK_SPACE));
    view = panel.view();
    REQUIRE_EQ(view.tray.size(), size_t{2});
    REQUIRE_EQ(view.tray[0], std::wstring(L"≈"));
    REQUIRE_EQ(view.tray[1], std::wstring(L"≡"));
    panel.key(vk(VK_BACK));
    REQUIRE_EQ(panel.view().tray.size(), size_t{1});
    auto action = panel.key(vk(VK_RETURN));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::Insert);
    REQUIRE_EQ(action.text, std::wstring(L"≈"));
    // Enter alone types the highlighted item.
    panel.key(vk(VK_DOWN));
    action = panel.key(vk(VK_RETURN));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::Insert && !action.text.empty());
    panel.close();

    // What was typed comes first next time (shared with the settings app).
    panel.open();
    view = panel.view();
    REQUIRE_EQ(view.categories.at(0), std::wstring(L"最近使用"));
    REQUIRE_TRUE(has_cell(view, L"≈"));
    REQUIRE_TRUE(std::filesystem::exists(data / L"desktop" / L"panel.json"));

    // ] goes to the next category, Ctrl+D adds to 收藏.
    panel.key(vk(VK_OEM_6));
    REQUIRE_EQ(panel.view().category, 1);
    panel.key(vk(L'D', false, true));
    panel.close();
    panel.open();
    REQUIRE_EQ(panel.view().categories.at(1), std::wstring(L"收藏"));

    // Typing pinyin searches names and keywords: meiyuan finds $.
    type(panel, L"meiyuan");
    view = panel.view();
    REQUIRE_EQ(view.input, std::wstring(L"meiyuan"));
    REQUIRE_EQ(view.categories.at(0), std::wstring(L"搜索结果"));
    REQUIRE_TRUE(has_cell(view, L"$"));
    panel.key(vk(VK_ESCAPE));
    REQUIRE_TRUE(panel.view().input.empty());
    REQUIRE_TRUE(panel.key(vk(VK_ESCAPE)).kind == PanelAction::Kind::Close);

    // Tab and Shift+Tab go through the pages.
    panel.open(PanelPage::Symbol);
    panel.key(vk(VK_TAB));
    REQUIRE_TRUE(panel.page() == PanelPage::Kaomoji);
    REQUIRE_EQ(panel.view().columns, 3);
    panel.key(vk(VK_TAB, true));
    panel.key(vk(VK_TAB, true));
    REQUIRE_TRUE(panel.page() == PanelPage::Clipboard);
    REQUIRE_TRUE(panel.key(vk(VK_RETURN, true)).kind == PanelAction::Kind::Expand);

    // 表情: an emoji typed from here is learned for its Chinese names.
    panel.open(PanelPage::Emoji);
    type(panel, L"kaixin");
    REQUIRE_TRUE(has_cell(panel.view(), L"😄"));
    action = panel.click(PanelHit{PanelHit::Kind::Cell, 0});
    action = panel.key(vk(VK_RETURN));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::Insert);
    REQUIRE_TRUE(!action.learn_emoji.empty());
    REQUIRE_TRUE(!action.learn_emoji[0].second.empty());

    // 工具: what is typed converts at once.
    panel.open(PanelPage::Tools);
    REQUIRE_EQ(panel.view().cells.size(), size_t{4});
    type(panel, L"55+15");
    view = panel.view();
    REQUIRE_EQ(view.cells.at(0).text, std::wstring(L"70"));
    action = panel.key(vk(VK_RETURN));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::Insert && action.text == L"70");
    panel.open(PanelPage::Tools);
    panel.key(vk(VK_DOWN));
    panel.key(vk(VK_RETURN));
    REQUIRE_EQ(panel.view().input, std::wstring(L"123"));
    REQUIRE_EQ(panel.view().cells.at(0).text, std::wstring(L"壹佰贰拾叁"));

    // 剪贴板: newest first, searchable by pinyin, numbers type them.
    localpinyin::ClipboardStore clipboard((data / L"clipboard").wstring());
    clipboard.add_text(L"明天开会", (now - 60) * 1000);
    clipboard.add_text(L"会议纪要第二版", now * 1000);
    panel.open(PanelPage::Clipboard);
    view = panel.view();
    REQUIRE_TRUE(view.numbered_list && view.cells.size() == 2);
    REQUIRE_EQ(view.cells[0].text, std::wstring(L"会议纪要第二版"));
    type(panel, L"huiyi");
    REQUIRE_EQ(panel.view().cells.size(), size_t{1});
    action = panel.key(vk(L'1'));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::Insert && action.text == L"会议纪要第二版");

    // 记录: first a password (asked twice), then items open to their fields.
    panel.open(PanelPage::Records);
    view = panel.view();
    REQUIRE_EQ(view.cells.size(), size_t{2});
    panel.key(vk(L'1'));
    type(panel, L"abc");
    REQUIRE_EQ(panel.view().input, std::wstring(L"•••"));
    panel.key(vk(VK_RETURN));
    type(panel, L"abd");
    panel.key(vk(VK_RETURN));
    REQUIRE_TRUE(panel.view().prompt == L"设置密码");
    type(panel, L"abc");
    panel.key(vk(VK_RETURN));
    type(panel, L"abc");
    panel.key(vk(VK_RETURN));
    view = panel.view();
    REQUIRE_EQ(view.categories.size(), size_t{4});
    REQUIRE_TRUE(view.cells.empty());
    const localpinyin::Vault vault((data / L"vault.json").wstring());
    REQUIRE_TRUE(vault.has_password());
    // The settings app adds an item; the open panel shows it without asking again.
    localpinyin::VaultItem mail{L"0123456789abcdef", L"password", L"QQ 邮箱",
                                {{L"邮箱", L"me@qq.com", false}, {L"密码", L"secret!", true}}, now, now};
    REQUIRE_TRUE(vault.save(L"abc", {mail}));
    panel.close();
    panel.open(PanelPage::Records);
    view = panel.view();
    REQUIRE_EQ(view.cells.size(), size_t{1});
    REQUIRE_EQ(view.cells[0].text, std::wstring(L"QQ 邮箱"));
    REQUIRE_EQ(view.cells[0].tag, std::wstring(L"密码"));
    panel.key(vk(VK_RETURN));
    view = panel.view();
    REQUIRE_EQ(view.cells.size(), size_t{2});
    REQUIRE_EQ(view.cells[1].detail, std::wstring(L"••••••"));
    action = panel.key(vk(L'2'));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::Insert && action.text == L"secret!");
    panel.key(vk(VK_DOWN));
    action = panel.key(vk(L'C', false, true));
    REQUIRE_TRUE(action.kind == PanelAction::Kind::CopySecret && action.text == L"secret!");
    panel.close();
    // Five minutes after the last use it asks again.
    now += 301;
    panel.open(PanelPage::Records);
    REQUIRE_TRUE(panel.view().prompt == L"输入密码");
    type(panel, L"nope");
    panel.key(vk(VK_RETURN));
    REQUIRE_TRUE(panel.view().message.find(L"密码不对") != std::wstring::npos);
    type(panel, L"abc");
    panel.key(vk(VK_RETURN));
    REQUIRE_EQ(panel.view().cells.size(), size_t{1});
    // Address and other filters.
    panel.key(vk(VK_RIGHT));
    panel.key(vk(VK_RIGHT));
    REQUIRE_TRUE(panel.view().cells.empty());
    panel.close();

    REQUIRE_TRUE(ToolPanel::takes_key(L'A', false, false));
    REQUIRE_TRUE(!ToolPanel::takes_key(VK_SHIFT, false, false));
    REQUIRE_TRUE(!ToolPanel::takes_key(L'A', false, true));
    REQUIRE_TRUE(!ToolPanel::takes_key(L'V', true, false));
    REQUIRE_TRUE(ToolPanel::takes_key(L'C', true, false));
    std::filesystem::remove_all(data);
    return 0;
}
