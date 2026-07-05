#!/usr/bin/env python3
import argparse
import ctypes
import json
import struct
import sys
from ctypes import wintypes
from pathlib import Path

from PIL import Image, ImageDraw


EXPECTED_SIZES = {16, 20, 24, 32, 40, 48, 64, 128, 256}
RT_ICON = 3
RT_GROUP_ICON = 14
LOAD_LIBRARY_AS_DATAFILE = 0x00000002
LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)
shell32 = ctypes.WinDLL("shell32", use_last_error=True)


EnumResNameProc = ctypes.WINFUNCTYPE(
    wintypes.BOOL,
    wintypes.HMODULE,
    ctypes.c_void_p,
    ctypes.c_void_p,
    wintypes.LPARAM,
)


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [
        ("bmiHeader", BITMAPINFOHEADER),
        ("bmiColors", wintypes.DWORD * 3),
    ]


kernel32.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
kernel32.LoadLibraryExW.restype = wintypes.HMODULE
kernel32.FreeLibrary.argtypes = [wintypes.HMODULE]
kernel32.FreeLibrary.restype = wintypes.BOOL
kernel32.EnumResourceNamesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, EnumResNameProc, wintypes.LPARAM]
kernel32.EnumResourceNamesW.restype = wintypes.BOOL
kernel32.FindResourceW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p]
kernel32.FindResourceW.restype = wintypes.HRSRC
kernel32.LoadResource.argtypes = [wintypes.HMODULE, wintypes.HRSRC]
kernel32.LoadResource.restype = wintypes.HGLOBAL
kernel32.LockResource.argtypes = [wintypes.HGLOBAL]
kernel32.LockResource.restype = wintypes.LPVOID
kernel32.SizeofResource.argtypes = [wintypes.HMODULE, wintypes.HRSRC]
kernel32.SizeofResource.restype = wintypes.DWORD

shell32.ExtractIconExW.argtypes = [
    wintypes.LPCWSTR,
    ctypes.c_int,
    ctypes.POINTER(wintypes.HICON),
    ctypes.POINTER(wintypes.HICON),
    wintypes.UINT,
]
shell32.ExtractIconExW.restype = wintypes.UINT
user32.DestroyIcon.argtypes = [wintypes.HICON]
user32.DestroyIcon.restype = wintypes.BOOL
user32.DrawIconEx.argtypes = [
    wintypes.HDC,
    ctypes.c_int,
    ctypes.c_int,
    wintypes.HICON,
    ctypes.c_int,
    ctypes.c_int,
    wintypes.UINT,
    wintypes.HBRUSH,
    wintypes.UINT,
]
user32.DrawIconEx.restype = wintypes.BOOL

gdi32.CreateCompatibleDC.argtypes = [wintypes.HDC]
gdi32.CreateCompatibleDC.restype = wintypes.HDC
gdi32.DeleteDC.argtypes = [wintypes.HDC]
gdi32.DeleteDC.restype = wintypes.BOOL
gdi32.CreateDIBSection.argtypes = [
    wintypes.HDC,
    ctypes.POINTER(BITMAPINFO),
    wintypes.UINT,
    ctypes.POINTER(ctypes.c_void_p),
    wintypes.HANDLE,
    wintypes.DWORD,
]
gdi32.CreateDIBSection.restype = wintypes.HBITMAP
gdi32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
gdi32.SelectObject.restype = wintypes.HGDIOBJ
gdi32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
gdi32.DeleteObject.restype = wintypes.BOOL


def int_resource(value: int) -> ctypes.c_void_p:
    return ctypes.c_void_p(value)


def resource_name_to_text(name: ctypes.c_void_p) -> str:
    value = name if isinstance(name, int) else ctypes.cast(name, ctypes.c_void_p).value
    if value is not None and value <= 0xFFFF:
        return f"#{value}"
    return ctypes.wstring_at(value)


def enum_resource_names(module: wintypes.HMODULE, resource_type: int) -> list[str]:
    names: list[str] = []

    def callback(_module, _type, name, _param):
        names.append(resource_name_to_text(name))
        return True

    proc = EnumResNameProc(callback)
    kernel32.EnumResourceNamesW(module, int_resource(resource_type), proc, 0)
    return names


def resource_bytes(module: wintypes.HMODULE, resource_type: int, name_text: str) -> bytes:
    name = int_resource(int(name_text[1:])) if name_text.startswith("#") else ctypes.cast(ctypes.c_wchar_p(name_text), ctypes.c_void_p)
    resource = kernel32.FindResourceW(module, name, int_resource(resource_type))
    if not resource:
        raise AssertionError(f"missing resource type={resource_type} name={name_text}")
    size = kernel32.SizeofResource(module, resource)
    handle = kernel32.LoadResource(module, resource)
    pointer = kernel32.LockResource(handle)
    if not pointer or size == 0:
        raise AssertionError(f"empty resource type={resource_type} name={name_text}")
    return ctypes.string_at(pointer, size)


def parse_group_icon(data: bytes) -> list[dict[str, int]]:
    reserved, icon_type, count = struct.unpack_from("<HHH", data, 0)
    if reserved != 0 or icon_type != 1:
        raise AssertionError("RT_GROUP_ICON is not an icon group")
    entries = []
    offset = 6
    for _ in range(count):
        width, height, color_count, reserved, planes, bit_count, bytes_in_res, icon_id = struct.unpack_from(
            "<BBBBHHIH", data, offset
        )
        width = 256 if width == 0 else width
        height = 256 if height == 0 else height
        entries.append(
            {
                "width": width,
                "height": height,
                "color_count": color_count,
                "planes": planes,
                "bit_count": bit_count,
                "bytes_in_res": bytes_in_res,
                "icon_id": icon_id,
            }
        )
        offset += 14
    return entries


def render_hicon(icon: wintypes.HICON, size: int) -> Image.Image:
    hdc = gdi32.CreateCompatibleDC(None)
    if not hdc:
        raise AssertionError("CreateCompatibleDC failed")
    bits = ctypes.c_void_p()
    info = BITMAPINFO()
    info.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    info.bmiHeader.biWidth = size
    info.bmiHeader.biHeight = -size
    info.bmiHeader.biPlanes = 1
    info.bmiHeader.biBitCount = 32
    info.bmiHeader.biCompression = 0
    bitmap = gdi32.CreateDIBSection(hdc, ctypes.byref(info), 0, ctypes.byref(bits), None, 0)
    if not bitmap:
        gdi32.DeleteDC(hdc)
        raise AssertionError("CreateDIBSection failed")
    old = gdi32.SelectObject(hdc, bitmap)
    ok = user32.DrawIconEx(hdc, 0, 0, icon, size, size, 0, None, 0x0003)
    raw = ctypes.string_at(bits, size * size * 4)
    gdi32.SelectObject(hdc, old)
    gdi32.DeleteObject(bitmap)
    gdi32.DeleteDC(hdc)
    if not ok:
        raise AssertionError("DrawIconEx failed")
    image = Image.frombytes("RGBA", (size, size), raw, "raw", "BGRA")
    return image


def make_preview(small: Image.Image, big: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    canvas = Image.new("RGBA", (360, 160), (255, 255, 255, 255))
    draw = ImageDraw.Draw(canvas)
    draw.rectangle([180, 0, 359, 159], fill=(30, 35, 45, 255))
    canvas.alpha_composite(big.resize((64, 64), Image.Resampling.LANCZOS), (58, 34))
    canvas.alpha_composite(small.resize((32, 32), Image.Resampling.NEAREST), (74, 104))
    canvas.alpha_composite(big.resize((64, 64), Image.Resampling.LANCZOS), (238, 34))
    canvas.alpha_composite(small.resize((32, 32), Image.Resampling.NEAREST), (254, 104))
    draw.text((20, 12), "ExtractIconExW index 0 from LocalPinyinIME.dll", fill=(10, 20, 40, 255))
    path.write_bytes(b"")
    canvas.convert("RGBA").save(path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dll", required=True, type=Path)
    parser.add_argument("--preview", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()

    dll = args.dll.resolve()
    if not dll.exists():
        raise AssertionError(f"DLL does not exist: {dll}")

    module = kernel32.LoadLibraryExW(str(dll), None, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)
    if not module:
        raise AssertionError(f"LoadLibraryExW failed for {dll}: {ctypes.get_last_error()}")

    try:
        group_names = enum_resource_names(module, RT_GROUP_ICON)
        icon_names = enum_resource_names(module, RT_ICON)
        if not group_names:
            raise AssertionError("LocalPinyinIME.dll has no RT_GROUP_ICON resource")
        if not icon_names:
            raise AssertionError("LocalPinyinIME.dll has no RT_ICON resources")
        if len(group_names) != 1:
            raise AssertionError(f"expected one profile group icon for index 0, found {group_names}")

        entries = parse_group_icon(resource_bytes(module, RT_GROUP_ICON, group_names[0]))
        sizes = {entry["width"] for entry in entries}
        if sizes != EXPECTED_SIZES:
            raise AssertionError(f"unexpected profile icon sizes: {sorted(sizes)}")
        icon_ids = {f"#{entry['icon_id']}" for entry in entries}
        missing_icon_ids = sorted(icon_ids - set(icon_names))
        if missing_icon_ids:
            raise AssertionError(f"group icon references missing RT_ICON resources: {missing_icon_ids}")
    finally:
        kernel32.FreeLibrary(module)

    large = wintypes.HICON()
    small = wintypes.HICON()
    extracted = shell32.ExtractIconExW(str(dll), 0, ctypes.byref(large), ctypes.byref(small), 1)
    if extracted < 1 or not large.value or not small.value:
        raise AssertionError(f"ExtractIconExW index 0 failed: extracted={extracted}")
    try:
        small_image = render_hicon(small, 16)
        big_image = render_hicon(large, 32)
        if args.preview:
            make_preview(small_image, big_image, args.preview)
    finally:
        user32.DestroyIcon(large)
        user32.DestroyIcon(small)

    result = {
        "dll": str(dll),
        "group_icon_count": len(group_names),
        "group_icons": group_names,
        "rt_icon_count": len(icon_names),
        "rt_icons": icon_names,
        "index_0_group_icon": group_names[0],
        "sizes": sorted(sizes),
        "extract_icon_ex_index_0": True,
        "extract_icon_ex_return": extracted,
    }
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
