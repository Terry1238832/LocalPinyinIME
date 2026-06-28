#pragma once

#include "input_mode.h"

namespace localpinyin {

class LanguageBarState {
public:
    void set_mode(InputMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] InputMode mode() const noexcept { return mode_; }

private:
    InputMode mode_ = InputMode::Chinese;
};

}  // namespace localpinyin
