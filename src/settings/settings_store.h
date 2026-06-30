#pragma once

#include "../ime/candidate_ui.h"

#include <string>

namespace localpinyin {

class SettingsStore {
public:
    SettingsStore() = default;
    explicit SettingsStore(std::wstring data_dir_override);

    [[nodiscard]] std::wstring data_dir() const;
    [[nodiscard]] std::wstring settings_path() const;
    [[nodiscard]] std::wstring learning_data_path() const;
    bool ensure_data_dir() const;
    [[nodiscard]] CandidateWindowOptions load_candidate_options() const;
    bool save_candidate_options(const CandidateWindowOptions& options) const;
    bool clear_learning_data() const;

private:
    std::wstring data_dir_override_;
};

}  // namespace localpinyin
