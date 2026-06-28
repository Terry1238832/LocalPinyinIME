#include "user_learning.h"

#include "../common/utf_utils.h"

#include <fstream>
#include <sstream>
#include <ctime>

namespace localpinyin {
namespace {

std::wstring key_for(const std::wstring& pinyin, const std::wstring& word) {
    return pinyin + L"\t" + word;
}

}  // namespace

bool UserLearning::open(const std::wstring& storage_path) {
    storage_path_ = storage_path;
    return load();
}

int UserLearning::frequency(const std::wstring& pinyin, const std::wstring& word) const {
    const auto it = records_.find(key_for(pinyin, word));
    return it == records_.end() ? 0 : it->second.selection_count;
}

bool UserLearning::record_selection(const std::wstring& pinyin, const std::wstring& word) {
    auto& record = records_[key_for(pinyin, word)];
    ++record.selection_count;
    record.last_selected_time = static_cast<long long>(std::time(nullptr));
    return flush();
}

bool UserLearning::clear() {
    records_.clear();
    return flush();
}

bool UserLearning::load() {
    records_.clear();
    std::ifstream file(storage_path_, std::ios::binary);
    if (!file) {
        return true;
    }

    std::string line;
    while (std::getline(file, line)) {
        const std::wstring wide = utf8_to_wide(line);
        const auto first = wide.find(L'\t');
        const auto second = wide.find(L'\t', first == std::wstring::npos ? first : first + 1);
        if (first == std::wstring::npos || second == std::wstring::npos) {
            continue;
        }
        const std::wstring key = wide.substr(0, second);
        const auto third = wide.find(L'\t', second + 1);
        const std::wstring count_text = wide.substr(second + 1, third == std::wstring::npos ? std::wstring::npos : third - second - 1);
        const int value = std::stoi(count_text);
        long long last_selected_time = 0;
        if (third != std::wstring::npos) {
            last_selected_time = std::stoll(wide.substr(third + 1));
        }
        records_[key] = Record{value, last_selected_time};
    }
    return true;
}

bool UserLearning::flush() const {
    std::ofstream file(storage_path_, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    for (const auto& [key, record] : records_) {
        file << wide_to_utf8(key) << '\t' << record.selection_count << '\t' << record.last_selected_time << '\n';
    }
    return true;
}

}  // namespace localpinyin
