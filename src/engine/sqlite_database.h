#pragma once

#include <string>

namespace localpinyin {

class SqliteDatabase {
public:
    bool open(const std::wstring& path);
    bool ensure_schema();
    bool is_open() const noexcept { return opened_; }

private:
    std::wstring path_;
    bool opened_ = false;
};

}  // namespace localpinyin
