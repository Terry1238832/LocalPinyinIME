#include "picture_clipboard.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace localpinyin {

bool put_picture_on_clipboard(const std::wstring& file, bool keep_out_of_history) {
    using Microsoft::WRL::ComPtr;
    std::vector<std::uint8_t> dib;
    {
        ComPtr<IWICImagingFactory> factory;
        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        UINT width = 0, height = 0;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
            SUCCEEDED(factory->CreateDecoderFromFilename(file.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder)) &&
            SUCCEEDED(decoder->GetFrame(0, &frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
            SUCCEEDED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0,
                                            WICBitmapPaletteTypeCustom)) &&
            SUCCEEDED(converter->GetSize(&width, &height)) && width && height && width <= 8192 && height <= 8192) {
            BITMAPINFOHEADER header{};
            header.biSize = sizeof(header);
            header.biWidth = static_cast<LONG>(width);
            header.biHeight = -static_cast<LONG>(height);
            header.biPlanes = 1;
            header.biBitCount = 32;
            header.biCompression = BI_RGB;
            const UINT stride = width * 4;
            dib.resize(sizeof(header) + static_cast<size_t>(stride) * height);
            std::memcpy(dib.data(), &header, sizeof(header));
            if (FAILED(converter->CopyPixels(nullptr, stride, stride * height, dib.data() + sizeof(header)))) dib.clear();
        }
    }
    bool opened = false;
    for (int attempt = 0; attempt < 10 && !opened; ++attempt) {
        opened = OpenClipboard(nullptr) != FALSE;
        if (!opened) Sleep(20);
    }
    if (!opened) return false;
    EmptyClipboard();
    const auto put = [](UINT format, const void* data, size_t size) {
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size ? size : 1);
        if (!memory) return;
        if (size) {
            void* target = GlobalLock(memory);
            std::memcpy(target, data, size);
            GlobalUnlock(memory);
        }
        if (!SetClipboardData(format, memory)) GlobalFree(memory);
    };
    std::vector<std::uint8_t> drop(sizeof(DROPFILES) + (file.size() + 2) * sizeof(wchar_t), 0);
    auto* files = reinterpret_cast<DROPFILES*>(drop.data());
    files->pFiles = sizeof(DROPFILES);
    files->fWide = TRUE;
    std::memcpy(drop.data() + sizeof(DROPFILES), file.c_str(), file.size() * sizeof(wchar_t));
    put(CF_HDROP, drop.data(), drop.size());
    if (!dib.empty()) put(CF_DIB, dib.data(), dib.size());
    std::ifstream source(std::filesystem::path(file), std::ios::binary);
    const std::vector<char> bytes((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    const auto extension = std::filesystem::path(file).extension().wstring();
    if (!bytes.empty() && (extension == L".png" || extension == L".PNG"))
        put(RegisterClipboardFormatW(L"PNG"), bytes.data(), bytes.size());
    if (!bytes.empty() && (extension == L".gif" || extension == L".GIF"))
        put(RegisterClipboardFormatW(L"GIF"), bytes.data(), bytes.size());
    if (keep_out_of_history) put(RegisterClipboardFormatW(L"ExcludeClipboardContentFromMonitorProcessing"), nullptr, 0);
    CloseClipboard();
    return true;
}

void press_paste() {
    INPUT keys[4]{};
    for (auto& key : keys) key.type = INPUT_KEYBOARD;
    keys[0].ki.wVk = VK_CONTROL;
    keys[1].ki.wVk = 'V';
    keys[2].ki.wVk = 'V';
    keys[2].ki.dwFlags = KEYEVENTF_KEYUP;
    keys[3].ki.wVk = VK_CONTROL;
    keys[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, keys, sizeof(INPUT));
}

}  // namespace localpinyin
