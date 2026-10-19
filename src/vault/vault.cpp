#include "vault.h"

#include "../common/json_lite.h"
#include "../common/utf_utils.h"

#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace localpinyin {
namespace {

using Bytes = std::vector<unsigned char>;

constexpr wchar_t kFormat[] = L"localpinyin-vault";
constexpr char kAad[] = "localpinyin-vault-1";
constexpr size_t kNonce = 12;
constexpr size_t kTag = 16;
constexpr size_t kKey = 32;

// Wipes a buffer that held key material or plaintext.
void wipe(Bytes& bytes) {
    if (!bytes.empty()) SecureZeroMemory(bytes.data(), bytes.size());
    bytes.clear();
}

void wipe(std::string& text) {
    if (!text.empty()) SecureZeroMemory(text.data(), text.size());
    text.clear();
}

Bytes random_bytes(size_t count) {
    Bytes bytes(count);
    if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(count), BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
        bytes.clear();
    return bytes;
}

std::wstring base64(const Bytes& bytes) {
    if (bytes.empty()) return {};
    DWORD size = 0;
    const DWORD flags = CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF;
    if (!CryptBinaryToStringW(bytes.data(), static_cast<DWORD>(bytes.size()), flags, nullptr, &size)) return {};
    std::wstring text(size, L'\0');
    if (!CryptBinaryToStringW(bytes.data(), static_cast<DWORD>(bytes.size()), flags, text.data(), &size)) return {};
    text.resize(size);
    return text;
}

Bytes unbase64(const std::wstring& text) {
    DWORD size = 0;
    if (text.empty() || !CryptStringToBinaryW(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64, nullptr, &size,
                                              nullptr, nullptr))
        return {};
    Bytes bytes(size);
    if (!CryptStringToBinaryW(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64, bytes.data(), &size, nullptr,
                              nullptr))
        return {};
    bytes.resize(size);
    return bytes;
}

// AES-256-GCM: nonce, ciphertext and tag in one buffer.
Bytes seal(const Bytes& key, const unsigned char* plain, size_t length) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_KEY_HANDLE handle = nullptr;
    Bytes out;
    Bytes nonce = random_bytes(kNonce);
    if (key.size() != kKey || nonce.size() != kNonce) return out;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0)) &&
        BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
                                         sizeof(BCRYPT_CHAIN_MODE_GCM), 0)) &&
        BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(algorithm, &handle, nullptr, 0, const_cast<PUCHAR>(key.data()),
                                                  static_cast<ULONG>(key.size()), 0))) {
        out.resize(kNonce + length + kTag);
        std::memcpy(out.data(), nonce.data(), kNonce);
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
        BCRYPT_INIT_AUTH_MODE_INFO(info);
        info.pbNonce = out.data();
        info.cbNonce = static_cast<ULONG>(kNonce);
        info.pbAuthData = reinterpret_cast<PUCHAR>(const_cast<char*>(kAad));
        info.cbAuthData = static_cast<ULONG>(sizeof(kAad) - 1);
        info.pbTag = out.data() + kNonce + length;
        info.cbTag = static_cast<ULONG>(kTag);
        ULONG written = 0;
        if (!BCRYPT_SUCCESS(BCryptEncrypt(handle, const_cast<PUCHAR>(plain), static_cast<ULONG>(length), &info, nullptr, 0,
                                          out.data() + kNonce, static_cast<ULONG>(length), &written, 0)) ||
            written != length)
            out.clear();
    }
    if (handle) BCryptDestroyKey(handle);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    return out;
}

bool open_sealed(const Bytes& key, const Bytes& sealed, Bytes* plain) {
    if (key.size() != kKey || sealed.size() < kNonce + kTag) return false;
    const size_t length = sealed.size() - kNonce - kTag;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_KEY_HANDLE handle = nullptr;
    bool opened = false;
    plain->assign(length, 0);
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0)) &&
        BCRYPT_SUCCESS(BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
                                         sizeof(BCRYPT_CHAIN_MODE_GCM), 0)) &&
        BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(algorithm, &handle, nullptr, 0, const_cast<PUCHAR>(key.data()),
                                                  static_cast<ULONG>(key.size()), 0))) {
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO info;
        BCRYPT_INIT_AUTH_MODE_INFO(info);
        info.pbNonce = const_cast<PUCHAR>(sealed.data());
        info.cbNonce = static_cast<ULONG>(kNonce);
        info.pbAuthData = reinterpret_cast<PUCHAR>(const_cast<char*>(kAad));
        info.cbAuthData = static_cast<ULONG>(sizeof(kAad) - 1);
        info.pbTag = const_cast<PUCHAR>(sealed.data() + kNonce + length);
        info.cbTag = static_cast<ULONG>(kTag);
        ULONG written = 0;
        opened = BCRYPT_SUCCESS(BCryptDecrypt(handle, const_cast<PUCHAR>(sealed.data() + kNonce), static_cast<ULONG>(length), &info,
                                              nullptr, 0, plain->empty() ? nullptr : plain->data(), static_cast<ULONG>(length),
                                              &written, 0)) &&
                 written == length;
    }
    if (handle) BCryptDestroyKey(handle);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!opened) wipe(*plain);
    return opened;
}

Bytes derive(const std::wstring& password, const Bytes& salt, unsigned long iterations) {
    Bytes key(kKey);
    std::string secret = wide_to_utf8(password);
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    const bool derived = BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                                                    BCRYPT_ALG_HANDLE_HMAC_FLAG)) &&
                         BCRYPT_SUCCESS(BCryptDeriveKeyPBKDF2(algorithm, reinterpret_cast<PUCHAR>(secret.data()),
                                                              static_cast<ULONG>(secret.size()), const_cast<PUCHAR>(salt.data()),
                                                              static_cast<ULONG>(salt.size()), iterations, key.data(),
                                                              static_cast<ULONG>(key.size()), 0));
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    wipe(secret);
    if (!derived) wipe(key);
    return key;
}

DATA_BLOB entropy_blob() {
    static const char entropy[] = "LocalPinyinIME.Vault";
    return DATA_BLOB{static_cast<DWORD>(sizeof(entropy) - 1), reinterpret_cast<BYTE*>(const_cast<char*>(entropy))};
}

Bytes protect_for_user(const Bytes& key) {
    DATA_BLOB in{static_cast<DWORD>(key.size()), const_cast<BYTE*>(key.data())};
    DATA_BLOB extra = entropy_blob();
    DATA_BLOB out{};
    Bytes result;
    if (CryptProtectData(&in, L"LocalPinyinIME", &extra, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        result.assign(out.pbData, out.pbData + out.cbData);
        LocalFree(out.pbData);
    }
    return result;
}

Bytes unprotect_for_user(const Bytes& blob) {
    DATA_BLOB in{static_cast<DWORD>(blob.size()), const_cast<BYTE*>(blob.data())};
    DATA_BLOB extra = entropy_blob();
    DATA_BLOB out{};
    Bytes result;
    if (CryptUnprotectData(&in, nullptr, &extra, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        result.assign(out.pbData, out.pbData + out.cbData);
        SecureZeroMemory(out.pbData, out.cbData);
        LocalFree(out.pbData);
    }
    return result;
}

std::optional<JsonValue> read_file(const std::wstring& path) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) return std::nullopt;
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto value = parse_json(text);
    if (!value || !value->is_object() || value->text(L"format") != kFormat || value->num(L"version") != 1) return std::nullopt;
    return value;
}

bool write_file(const std::wstring& path, const JsonValue& value) {
    std::error_code error;
    const std::filesystem::path target(path);
    if (target.has_parent_path()) std::filesystem::create_directories(target.parent_path(), error);
    const std::wstring temporary = path + L".tmp";
    {
        std::ofstream file(std::filesystem::path(temporary), std::ios::binary | std::ios::trunc);
        if (!file) return false;
        const std::string text = write_json(value) + "\n";
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!file) return false;
    }
    return MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}

// The data key from the file, with the password when it has one.
bool unwrap_key(const JsonValue& file, const std::wstring& password, Bytes* key) {
    const Bytes wrapped = unbase64(file.text(L"key"));
    if (file.text(L"lock") == L"windows") {
        *key = unprotect_for_user(wrapped);
        return key->size() == kKey;
    }
    const Bytes salt = unbase64(file.text(L"salt"));
    const double iterations = file.num(L"iterations");
    if (salt.size() < 16 || iterations < 1000 || iterations > 10000000) return false;
    Bytes kek = derive(password, salt, static_cast<unsigned long>(iterations));
    const bool opened = open_sealed(kek, wrapped, key) && key->size() == kKey;
    wipe(kek);
    if (!opened) wipe(*key);
    return opened;
}

// Writes the lock (how the data key is kept) into `file`.
bool wrap_key(JsonValue& file, const std::wstring& password, const Bytes& key) {
    if (password.empty()) {
        const Bytes blob = protect_for_user(key);
        if (blob.empty()) return false;
        file.set(L"lock", JsonValue::make_string(L"windows"));
        file.set(L"key", JsonValue::make_string(base64(blob)));
        file.set(L"salt", JsonValue::make_string({}));
        file.set(L"iterations", JsonValue::make_number(0));
        return true;
    }
    const Bytes salt = random_bytes(16);
    if (salt.size() != 16) return false;
    Bytes kek = derive(password, salt, Vault::kIterations);
    const Bytes wrapped = seal(kek, key.data(), key.size());
    wipe(kek);
    if (wrapped.empty()) return false;
    file.set(L"lock", JsonValue::make_string(L"password"));
    file.set(L"key", JsonValue::make_string(base64(wrapped)));
    file.set(L"salt", JsonValue::make_string(base64(salt)));
    file.set(L"iterations", JsonValue::make_number(Vault::kIterations));
    return true;
}

JsonValue item_json(const VaultItem& item) {
    JsonValue value = JsonValue::make_object();
    value.set(L"id", JsonValue::make_string(item.id));
    value.set(L"kind", JsonValue::make_string(item.kind));
    value.set(L"title", JsonValue::make_string(item.title));
    JsonValue fields = JsonValue::make_array();
    for (const auto& field : item.fields) {
        JsonValue entry = JsonValue::make_object();
        entry.set(L"name", JsonValue::make_string(field.name));
        entry.set(L"value", JsonValue::make_string(field.value));
        entry.set(L"secret", JsonValue::make_bool(field.secret));
        fields.array.push_back(std::move(entry));
    }
    value.set(L"fields", std::move(fields));
    value.set(L"created", JsonValue::make_number(static_cast<double>(item.created)));
    value.set(L"updated", JsonValue::make_number(static_cast<double>(item.updated)));
    return value;
}

bool item_from_json(const JsonValue& value, VaultItem* item) {
    if (!value.is_object()) return false;
    *item = {};
    item->id = value.text(L"id");
    item->kind = value.text(L"kind", L"other");
    if (item->kind != L"password" && item->kind != L"address") item->kind = L"other";
    item->title = value.text(L"title");
    item->created = static_cast<long long>(value.num(L"created"));
    item->updated = static_cast<long long>(value.num(L"updated"));
    if (const auto* fields = value.find(L"fields"); fields && fields->is_array()) {
        for (const auto& entry : fields->array) {
            if (!entry.is_object()) continue;
            item->fields.push_back(VaultField{entry.text(L"name"), entry.text(L"value"), entry.flag(L"secret")});
        }
    }
    return item->title.size() <= 200 && item->fields.size() <= 32;
}

}  // namespace

Vault::Vault(std::wstring path) : path_(std::move(path)) {}

bool Vault::exists() const {
    return read_file(path_).has_value();
}

bool Vault::has_password() const {
    const auto file = read_file(path_);
    return file && file->text(L"lock") == L"password";
}

bool Vault::create(const std::wstring& password) const {
    if (exists()) return false;
    Bytes key = random_bytes(kKey);
    if (key.size() != kKey) return false;
    JsonValue file = JsonValue::make_object();
    file.set(L"format", JsonValue::make_string(kFormat));
    file.set(L"version", JsonValue::make_number(1));
    std::string empty = "[]";
    const Bytes sealed = seal(key, reinterpret_cast<const unsigned char*>(empty.data()), empty.size());
    const bool created = !sealed.empty() && wrap_key(file, password, key) &&
                         (file.set(L"items", JsonValue::make_string(base64(sealed))), write_file(path_, file));
    wipe(key);
    return created;
}

bool Vault::open(const std::wstring& password, std::vector<VaultItem>* items) const {
    items->clear();
    const auto file = read_file(path_);
    if (!file) return false;
    Bytes key;
    if (!unwrap_key(*file, password, &key)) return false;
    Bytes plain;
    const bool opened = open_sealed(key, unbase64(file->text(L"items")), &plain);
    wipe(key);
    if (!opened) return false;
    std::string text(plain.begin(), plain.end());
    wipe(plain);
    auto value = parse_json(text);
    wipe(text);
    if (!value || !value->is_array()) return false;
    for (const auto& entry : value->array) {
        VaultItem item;
        if (item_from_json(entry, &item) && !item.id.empty()) items->push_back(std::move(item));
    }
    return true;
}

bool Vault::save(const std::wstring& password, const std::vector<VaultItem>& items) const {
    if (items.size() > kMaxItems) return false;
    auto file = read_file(path_);
    if (!file) return false;
    Bytes key;
    if (!unwrap_key(*file, password, &key)) return false;
    JsonValue list = JsonValue::make_array();
    for (const auto& item : items) list.array.push_back(item_json(item));
    std::string text = write_json(list);
    const Bytes sealed = seal(key, reinterpret_cast<const unsigned char*>(text.data()), text.size());
    wipe(text);
    wipe(key);
    if (sealed.empty()) return false;
    file->set(L"items", JsonValue::make_string(base64(sealed)));
    return write_file(path_, *file);
}

bool Vault::change_password(const std::wstring& old_password, const std::wstring& new_password) const {
    auto file = read_file(path_);
    if (!file) return false;
    Bytes key;
    if (!unwrap_key(*file, old_password, &key)) return false;
    const bool wrapped = wrap_key(*file, new_password, key);
    wipe(key);
    return wrapped && write_file(path_, *file);
}

bool Vault::destroy() const {
    std::error_code error;
    std::filesystem::remove(std::filesystem::path(path_), error);
    return !error && !std::filesystem::exists(std::filesystem::path(path_));
}

std::wstring Vault::new_id() {
    const Bytes bytes = random_bytes(8);
    static constexpr wchar_t hex[] = L"0123456789abcdef";
    std::wstring id;
    for (unsigned char byte : bytes) {
        id += hex[byte >> 4];
        id += hex[byte & 15];
    }
    if (id.empty()) id = std::to_wstring(std::chrono::steady_clock::now().time_since_epoch().count());
    return id;
}

std::string vault_items_json(const std::vector<VaultItem>& items) {
    JsonValue list = JsonValue::make_array();
    for (const auto& item : items) list.array.push_back(item_json(item));
    return write_json(list);
}

bool parse_vault_item(const std::string& json, VaultItem* item) {
    const auto value = parse_json(json);
    return value && item_from_json(*value, item);
}

}  // namespace localpinyin
