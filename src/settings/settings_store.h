#pragma once

#include "../ime/input_mode.h"
#include "../ime/candidate_ui.h"

#include <string>
#include <cstdint>

namespace localpinyin {

struct RuntimeInputModeStatus {
    unsigned int schema_version = 0;
    bool active = false;
    InputMode mode = InputMode::Chinese;
    std::wstring source;
    unsigned long process_id = 0;
    unsigned long thread_id = 0;
    unsigned long long sequence = 0;
    unsigned long long updated_at_utc_ms = 0;
    bool available = false;
};

[[nodiscard]] unsigned long long runtime_status_current_utc_ms();
[[nodiscard]] unsigned long long runtime_status_stale_after_ms();

class SettingsStore {
public:
    SettingsStore() = default;
    explicit SettingsStore(std::wstring data_dir_override);

    [[nodiscard]] std::wstring data_dir() const;
    [[nodiscard]] std::wstring settings_path() const;
    [[nodiscard]] std::wstring learning_data_path() const;
    [[nodiscard]] std::wstring runtime_status_path() const;
    bool ensure_data_dir() const;
    [[nodiscard]] CandidateWindowOptions load_candidate_options() const;
    [[nodiscard]] InputModeOptions load_input_mode_options() const;
    [[nodiscard]] RuntimeInputModeStatus load_runtime_input_mode_status() const;
    bool save_options(const CandidateWindowOptions& candidate_options,
                      const InputModeOptions& input_mode_options) const;
    bool save_candidate_options(const CandidateWindowOptions& options) const;
    bool save_input_mode_options(const InputModeOptions& options) const;
    bool save_runtime_input_mode_status(const RuntimeInputModeStatus& status) const;
    bool clear_learning_data() const;

private:
    [[nodiscard]] InputModeOptions read_input_mode_options() const;

    std::wstring data_dir_override_;
    // The text service asks for the input options several times per key:
    // the parsed file is kept until its size or write time changes.
    struct InputModeCache {
        std::wstring path;
        unsigned long long write_time = 0;
        unsigned long long size = 0;
        bool exists = false;
        bool valid = false;
        InputModeOptions options;
    };
    mutable InputModeCache input_mode_cache_;
};

}  // namespace localpinyin
