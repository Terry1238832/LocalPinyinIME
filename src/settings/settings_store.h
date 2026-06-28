#pragma once

#include <string>

namespace localpinyin {

class SettingsStore {
public:
    [[nodiscard]] std::wstring data_dir() const;
    [[nodiscard]] std::wstring settings_path() const;
    bool ensure_data_dir() const;
};

}  // namespace localpinyin
