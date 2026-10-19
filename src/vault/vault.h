#pragma once

#include <string>
#include <vector>

namespace localpinyin {

// 记录: things the user keeps to type again — accounts and passwords,
// addresses, card numbers, anything else — encrypted in <data>\vault.json.
// Items are sealed with AES-256-GCM under a random data key. With a password
// the data key is wrapped by a key derived from it (PBKDF2-SHA256); without
// one, by Windows (DPAPI) for this user account, so the file is unreadable
// elsewhere either way. Changing or removing the password rewraps only the
// data key. The settings bridge writes the file; the input method only reads.
struct VaultField {
    std::wstring name;
    std::wstring value;
    bool secret = false;  // shown as dots until asked for
};

struct VaultItem {
    std::wstring id;
    std::wstring kind;  // "password", "address" or "other"
    std::wstring title;
    std::vector<VaultField> fields;
    long long created = 0;  // seconds since the epoch
    long long updated = 0;
};

class Vault {
public:
    static constexpr unsigned long kIterations = 300000;
    static constexpr size_t kMaxItems = 2000;

    explicit Vault(std::wstring path);
    [[nodiscard]] const std::wstring& path() const noexcept { return path_; }

    [[nodiscard]] bool exists() const;
    // Whether opening needs a password (false when missing or unreadable).
    [[nodiscard]] bool has_password() const;
    // An empty vault; an empty password leaves it to the Windows account.
    bool create(const std::wstring& password) const;
    // false: wrong password, or the file is missing or damaged.
    bool open(const std::wstring& password, std::vector<VaultItem>* items) const;
    bool save(const std::wstring& password, const std::vector<VaultItem>& items) const;
    // An empty new password removes it; an empty old one is for a vault without.
    bool change_password(const std::wstring& old_password, const std::wstring& new_password) const;
    bool destroy() const;

    // A new item id (16 hex digits).
    static std::wstring new_id();

private:
    std::wstring path_;
};

// The items as JSON and back, for the settings bridge.
std::string vault_items_json(const std::vector<VaultItem>& items);
bool parse_vault_item(const std::string& json, VaultItem* item);

}  // namespace localpinyin
