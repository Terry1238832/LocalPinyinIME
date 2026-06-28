#include "class_factory.h"
#include "globals.h"
#include "guids.h"
#include "registration.h"

#include <windows.h>

#include <new>

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID* object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (clsid != localpinyin::CLSID_LocalPinyinTextService) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    auto* factory = new (std::nothrow) localpinyin::ClassFactory();
    if (!factory) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = factory->QueryInterface(riid, object);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow(void) {
    return localpinyin::can_unload_now() ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer(void) {
    return localpinyin::register_server();
}

STDAPI DllUnregisterServer(void) {
    return localpinyin::unregister_server();
}
