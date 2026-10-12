#pragma once

#include "../engine/candidate.h"

#include <msctf.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace localpinyin {

struct CandidateModelSnapshot {
    bool active = false;
    std::wstring composition_text;
    std::vector<std::wstring> candidate_texts;
    std::size_t selection = 0;
    std::uintptr_t context_identity = 0;
    ITfDocumentMgr* document_mgr = nullptr;

    [[nodiscard]] std::size_t count() const noexcept {
        return candidate_texts.size();
    }
};

inline CandidateModelSnapshot make_candidate_model_snapshot(const std::wstring& composition,
                                                            const std::vector<Candidate>& candidates,
                                                            std::size_t selection,
                                                            ITfContext* context) {
    CandidateModelSnapshot snapshot;
    snapshot.active = !composition.empty() && !candidates.empty();
    snapshot.composition_text = composition;
    snapshot.selection = candidates.empty() ? 0 : (std::min)(selection, candidates.size() - 1);
    snapshot.context_identity = reinterpret_cast<std::uintptr_t>(context);
    snapshot.candidate_texts.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        snapshot.candidate_texts.push_back(candidate.text);
    }
    if (context) {
        ITfDocumentMgr* document_mgr = nullptr;
        if (SUCCEEDED(context->GetDocumentMgr(&document_mgr))) {
            snapshot.document_mgr = document_mgr;
        }
    }
    return snapshot;
}

inline void release_candidate_model_snapshot(CandidateModelSnapshot& snapshot) noexcept {
    if (snapshot.document_mgr) {
        snapshot.document_mgr->Release();
        snapshot.document_mgr = nullptr;
    }
}

}  // namespace localpinyin
