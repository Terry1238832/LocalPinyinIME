#include "settings_store.h"

#include "../common/win32_utils.h"

namespace localpinyin {

std::wstring SettingsStore::data_dir() const {
    return local_app_data_path() + L"\\LocalPinyinIME";
}

std::wstring SettingsStore::settings_path() const {
    return data_dir() + L"\\settings.json";
}

bool SettingsStore::ensure_data_dir() const {
    return ensure_directory(data_dir());
}

}  // namespace localpinyin
