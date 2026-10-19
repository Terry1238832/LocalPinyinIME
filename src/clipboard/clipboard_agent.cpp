// LocalPinyinClipboard.exe: records what the user copies, for the tool
// panel's 剪贴板 page, while clipboard history is on (asked once after
// installing or updating). It runs hidden from sign-in (a Run entry the
// settings bridge adds and removes with the setting), keeps texts and
// pictures in <data>\clipboard, reads the words in pictures with Windows'
// own OCR, and drops copies older than the chosen retention. Content an
// application marks as private (password managers) is skipped.
//
//   LocalPinyinClipboard.exe              record until the setting goes off
//   LocalPinyinClipboard.exe --replace    the same, after closing a running
//                                         recorder (an update's new version)
//   LocalPinyinClipboard.exe --quit       close the running recorder
//   LocalPinyinClipboard.exe --uninstall  also stop starting at sign-in
#include "clipboard_store.h"
#include "../settings/settings_store.h"

#include <windows.h>
#include <objidl.h>
#include <shlwapi.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace localpinyin;

namespace {

constexpr wchar_t kWindowClass[] = L"LocalPinyinIME.ClipboardAgent";
constexpr UINT_PTR kCaptureTimer = 1;
constexpr UINT_PTR kSettingsTimer = 2;
constexpr UINT_PTR kPruneTimer = 3;
constexpr UINT kCaptureDelayMs = 120;
constexpr UINT kSettingsCheckMs = 60 * 1000;
constexpr UINT kPruneMs = 60 * 60 * 1000;
constexpr UINT64 kMaxPixels = 40'000'000;

SettingsStore g_settings;
ClipboardStore* g_store = nullptr;

// ── Reading the words in pictures ───────────────────────────────────────
class OcrQueue {
public:
    void start() { worker_ = std::thread([this] { run(); }); }
    void stop() {
        { std::lock_guard lock(mutex_); stopping_ = true; }
        ready_.notify_all();
        if (worker_.joinable()) worker_.join();
    }
    void push(std::string id) {
        { std::lock_guard lock(mutex_); jobs_.push_back(std::move(id)); }
        ready_.notify_one();
    }

private:
    static bool cjk(wchar_t ch) { return (ch >= 0x3000 && ch <= 0x9FFF) || (ch >= 0xF900 && ch <= 0xFFEF); }

    // Words of a line, without the spaces OCR puts between Chinese characters.
    static std::wstring line_text(const winrt::Windows::Media::Ocr::OcrLine& line) {
        std::wstring text;
        for (const auto& word : line.Words()) {
            const std::wstring piece(word.Text());
            if (piece.empty()) continue;
            if (!text.empty() && !(cjk(text.back()) && cjk(piece.front()))) text += L' ';
            text += piece;
        }
        return text;
    }

    static winrt::Windows::Media::Ocr::OcrEngine engine() {
        using namespace winrt::Windows::Media::Ocr;
        using winrt::Windows::Globalization::Language;
        for (const wchar_t* tag : {L"zh-Hans-CN", L"zh-Hans", L"zh-CN"}) {
            try {
                const Language language(tag);
                if (OcrEngine::IsLanguageSupported(language)) if (auto found = OcrEngine::TryCreateFromLanguage(language)) return found;
            } catch (...) {}
        }
        return OcrEngine::TryCreateFromUserProfileLanguages();
    }

    std::wstring recognize(const winrt::Windows::Media::Ocr::OcrEngine& ocr, const std::wstring& path) {
        using namespace winrt::Windows::Graphics::Imaging;
        using namespace winrt::Windows::Storage;
        auto file = StorageFile::GetFileFromPathAsync(path).get();
        auto stream = file.OpenAsync(FileAccessMode::Read).get();
        auto decoder = BitmapDecoder::CreateAsync(stream).get();
        BitmapTransform transform;
        const UINT32 limit = winrt::Windows::Media::Ocr::OcrEngine::MaxImageDimension();
        UINT32 width = decoder.PixelWidth(), height = decoder.PixelHeight();
        if (width > limit || height > limit) {
            const double scale = static_cast<double>(limit) / std::max(width, height);
            width = std::max<UINT32>(1, static_cast<UINT32>(width * scale));
            height = std::max<UINT32>(1, static_cast<UINT32>(height * scale));
            transform.ScaledWidth(width);
            transform.ScaledHeight(height);
        }
        auto bitmap = decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Premultiplied, transform,
                                                     ExifOrientationMode::IgnoreExifOrientation,
                                                     ColorManagementMode::DoNotColorManage).get();
        auto result = ocr.RecognizeAsync(bitmap).get();
        std::wstring text;
        for (const auto& line : result.Lines()) {
            const auto piece = line_text(line);
            if (piece.empty()) continue;
            if (!text.empty()) text += L'\n';
            text += piece;
        }
        return text;
    }

    void run() {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        winrt::Windows::Media::Ocr::OcrEngine ocr{nullptr};
        try { ocr = engine(); } catch (...) {}
        while (true) {
            std::string id;
            {
                std::unique_lock lock(mutex_);
                ready_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
                if (stopping_) break;
                id = std::move(jobs_.front());
                jobs_.pop_front();
            }
            std::wstring text;
            if (ocr) {
                try { text = recognize(ocr, g_store->picture_path(id)); } catch (...) {}
            }
            // Done even without words (or without an OCR language), so it is not tried again.
            g_store->set_ocr(id, text);
        }
        winrt::uninit_apartment();
    }

    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<std::string> jobs_;
    bool stopping_ = false;
};

OcrQueue g_ocr;

// ── Reading the clipboard ───────────────────────────────────────────────
struct Picture {
    std::vector<std::uint8_t> png;   // as copied, when it came as PNG
    HBITMAP bitmap = nullptr;        // otherwise our own copy of the bitmap
};

bool private_content() {
    static const UINT exclude = RegisterClipboardFormatW(L"ExcludeClipboardContentFromMonitorProcessing");
    static const UINT ignore = RegisterClipboardFormatW(L"Clipboard Viewer Ignore");
    static const UINT history = RegisterClipboardFormatW(L"CanIncludeInClipboardHistory");
    if (IsClipboardFormatAvailable(exclude) || IsClipboardFormatAvailable(ignore)) return true;
    if (IsClipboardFormatAvailable(history)) {
        if (HANDLE data = GetClipboardData(history)) {
            const auto* value = static_cast<const DWORD*>(GlobalLock(data));
            const bool excluded = value && GlobalSize(data) >= sizeof(DWORD) && *value == 0;
            GlobalUnlock(data);
            if (excluded) return true;
        }
    }
    return false;
}

bool open_clipboard(HWND window) {
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (OpenClipboard(window)) return true;
        Sleep(25);
    }
    return false;
}

// Pixels as 32-bit BGRA, for the hash and for encoding.
bool pixels_of(IWICImagingFactory* factory, IWICBitmapSource* source, UINT* width, UINT* height, std::vector<BYTE>* pixels,
               ComPtr<IWICBitmapSource>* converted) {
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(source, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom)) ||
        FAILED(converter->GetSize(width, height)) || !*width || !*height || static_cast<UINT64>(*width) * *height > kMaxPixels)
        return false;
    pixels->resize(static_cast<size_t>(*width) * *height * 4);
    if (FAILED(converter->CopyPixels(nullptr, *width * 4, static_cast<UINT>(pixels->size()), pixels->data()))) return false;
    *converted = converter;
    return true;
}

bool encode_png(IWICImagingFactory* factory, IWICBitmapSource* source, std::vector<std::uint8_t>* png) {
    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(nullptr, 0));
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    UINT width = 0, height = 0;
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    if (!stream || FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
        FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) || FAILED(encoder->CreateNewFrame(&frame, nullptr)) ||
        FAILED(frame->Initialize(nullptr)) || FAILED(source->GetSize(&width, &height)) || FAILED(frame->SetSize(width, height)) ||
        FAILED(frame->SetPixelFormat(&format)) || FAILED(frame->WriteSource(source, nullptr)) || FAILED(frame->Commit()) ||
        FAILED(encoder->Commit()))
        return false;
    STATSTG stat{};
    if (FAILED(stream->Stat(&stat, STATFLAG_NONAME))) return false;
    png->resize(static_cast<size_t>(stat.cbSize.QuadPart));
    const LARGE_INTEGER start{};
    ULONG read = 0;
    return SUCCEEDED(stream->Seek(start, STREAM_SEEK_SET, nullptr)) &&
           SUCCEEDED(stream->Read(png->data(), static_cast<ULONG>(png->size()), &read)) && read == png->size();
}

void store_picture(Picture picture) {
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) return;
    ComPtr<IWICBitmapSource> source;
    if (!picture.png.empty()) {
        ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(factory->CreateStream(&stream)) ||
            FAILED(stream->InitializeFromMemory(picture.png.data(), static_cast<DWORD>(picture.png.size()))) ||
            FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) ||
            FAILED(decoder->GetFrame(0, &frame)))
            picture.png.clear();
        else source = frame;
    }
    if (!source && picture.bitmap) {
        ComPtr<IWICBitmap> bitmap;
        if (SUCCEEDED(factory->CreateBitmapFromHBITMAP(picture.bitmap, nullptr, WICBitmapIgnoreAlpha, &bitmap))) source = bitmap;
    }
    UINT width = 0, height = 0;
    std::vector<BYTE> pixels;
    ComPtr<IWICBitmapSource> converted;
    if (source && pixels_of(factory.Get(), source.Get(), &width, &height, &pixels, &converted)) {
        std::string hash = ClipboardStore::hash_bytes(pixels.data(), pixels.size());
        hash += std::to_string(width) + "x" + std::to_string(height);
        std::vector<std::uint8_t> png = std::move(picture.png);
        if (png.empty() && !encode_png(factory.Get(), converted.Get(), &png)) png.clear();
        std::string id;
        if (!png.empty() && g_store->add_picture(png, static_cast<int>(width), static_cast<int>(height),
                                                 ClipboardStore::hash_bytes(hash.data(), hash.size()), ClipboardStore::now_ms(), &id)) {
            for (const auto& item : g_store->read())
                if (item.id == id && !item.ocr_done) { g_ocr.push(id); break; }
        }
    }
    if (picture.bitmap) DeleteObject(picture.bitmap);
}

bool recording() {
    return g_settings.load_input_mode_options().clipboard_history == InputModeOptions::ClipboardHistory::On;
}

void capture(HWND window) {
    if (!recording()) { DestroyWindow(window); return; }
    if (!open_clipboard(window)) return;
    std::wstring text;
    Picture picture;
    bool skip = private_content();
    if (!skip && IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
            if (const auto* value = static_cast<const wchar_t*>(GlobalLock(data))) {
                const size_t limit = GlobalSize(data) / sizeof(wchar_t);
                size_t length = 0;
                while (length < limit && value[length]) ++length;
                if (length <= ClipboardStore::kMaxTextLength) text.assign(value, length);
                GlobalUnlock(data);
            }
        }
    } else if (!skip) {
        static const UINT png = RegisterClipboardFormatW(L"PNG");
        if (IsClipboardFormatAvailable(png)) {
            if (HANDLE data = GetClipboardData(png)) {
                if (const auto* bytes = static_cast<const std::uint8_t*>(GlobalLock(data))) {
                    picture.png.assign(bytes, bytes + GlobalSize(data));
                    GlobalUnlock(data);
                }
            }
        }
        if (picture.png.empty() && (IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_DIB))) {
            if (auto bitmap = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP)))
                picture.bitmap = static_cast<HBITMAP>(CopyImage(bitmap, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        }
    }
    CloseClipboard();
    if (skip) return;
    if (!text.empty()) {
        if (text.find_first_not_of(L" \t\r\n　") != std::wstring::npos) g_store->add_text(text, ClipboardStore::now_ms());
    } else if (!picture.png.empty() || picture.bitmap) {
        store_picture(std::move(picture));
    }
    g_store->prune(g_settings.load_input_mode_options().clipboard_retention_days, ClipboardStore::now_ms());
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
        case WM_CLIPBOARDUPDATE:
            SetTimer(window, kCaptureTimer, kCaptureDelayMs, nullptr);
            return 0;
        case WM_TIMER:
            if (wparam == kCaptureTimer) { KillTimer(window, kCaptureTimer); capture(window); }
            else if (wparam == kSettingsTimer) { if (!recording()) DestroyWindow(window); }
            else if (wparam == kPruneTimer)
                g_store->prune(g_settings.load_input_mode_options().clipboard_retention_days, ClipboardStore::now_ms());
            return 0;
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            RemoveClipboardFormatListener(window);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wparam, lparam);
    }
}

}  // namespace

// Closes every recorder of this session (an old one moved aside by an
// update too) and waits for them to go.
void close_running() {
    HWND window = nullptr;
    while ((window = FindWindowExW(HWND_MESSAGE, window, kWindowClass, nullptr)) != nullptr) PostMessageW(window, WM_CLOSE, 0, 0);
    for (int attempt = 0; attempt < 50 && FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, nullptr); ++attempt) Sleep(100);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int) {
    const std::wstring arguments = command_line ? command_line : L"";
    if (arguments.find(L"--uninstall") != std::wstring::npos) {
        RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"LocalPinyinClipboard");
        close_running();
        return 0;
    }
    if (arguments.find(L"--quit") != std::wstring::npos) { close_running(); return 0; }
    if (arguments.find(L"--replace") != std::wstring::npos) close_running();
    HANDLE single = CreateMutexW(nullptr, TRUE, L"Local\\LocalPinyinIME.ClipboardAgent");
    if (!single || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (single) CloseHandle(single);
        return 0;
    }
    if (!recording()) { CloseHandle(single); return 0; }
    // Each version installs into a folder of its own: sign-in starts this copy
    // from now on (an update's installer starts the new one with --replace).
    wchar_t skip[8]{};
    if (GetEnvironmentVariableW(L"LOCALPINYINIME_NO_AUTOSTART", skip, 8) == 0 || std::wstring(skip) != L"1") {
        wchar_t self[MAX_PATH]{};
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        const std::wstring command = L"\"" + std::wstring(self) + L"\"";
        RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"LocalPinyinClipboard", REG_SZ,
                        command.c_str(), static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ClipboardStore store(g_settings.data_dir() + L"\\clipboard");
    g_store = &store;
    store.prune(g_settings.load_input_mode_options().clipboard_retention_days, ClipboardStore::now_ms());
    g_ocr.start();
    for (const auto& item : store.read()) if (item.image && !item.ocr_done) g_ocr.push(item.id);

    WNDCLASSEXW window_class{sizeof(window_class)};
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = kWindowClass;
    RegisterClassExW(&window_class);
    HWND window = CreateWindowExW(0, kWindowClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
    if (window && AddClipboardFormatListener(window)) {
        SetTimer(window, kSettingsTimer, kSettingsCheckMs, nullptr);
        SetTimer(window, kPruneTimer, kPruneMs, nullptr);
        MSG message;
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    g_ocr.stop();
    CoUninitialize();
    ReleaseMutex(single);
    CloseHandle(single);
    return 0;
}
