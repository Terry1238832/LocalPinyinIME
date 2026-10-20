#pragma once

#include <string>

namespace localpinyin {

// A picture on the clipboard as a file (CF_HDROP: chat apps send GIFs whole)
// and as a bitmap (plus PNG or GIF as stored), by default marked so clipboard
// histories leave it out. False when the clipboard could not be opened.
bool put_picture_on_clipboard(const std::wstring& file, bool keep_out_of_history = true);

// Ctrl+V to the window in front.
void press_paste();

}  // namespace localpinyin
