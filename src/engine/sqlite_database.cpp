#include "sqlite_database.h"

namespace localpinyin {

bool SqliteDatabase::open(const std::wstring& path) {
    path_ = path;
    opened_ = !path_.empty();
    return opened_;
}

bool SqliteDatabase::ensure_schema() {
    return opened_;
}

}  // namespace localpinyin
