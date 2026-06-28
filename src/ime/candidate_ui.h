#pragma once

#include "candidate_window.h"

namespace localpinyin {

class CandidateUi {
public:
    CandidateWindow& window() noexcept { return window_; }

private:
    CandidateWindow window_;
};

}  // namespace localpinyin
