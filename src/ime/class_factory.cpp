#include "class_factory.h"

#include "globals.h"
#include "text_service.h"

#include <new>

namespace localpinyin {

ClassFactory::ClassFactory() {
    dll_add_ref();
}

ClassFactory::~ClassFactory() {
    dll_release();
}

HRESULT ClassFactory::QueryInterface(REFIID riid, void** object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
        *object = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG ClassFactory::AddRef() {
    return static_cast<ULONG>(InterlockedIncrement(&ref_count_));
}

ULONG ClassFactory::Release() {
    const long count = InterlockedDecrement(&ref_count_);
    if (count == 0) {
        delete this;
    }
    return static_cast<ULONG>(count);
}

HRESULT ClassFactory::CreateInstance(IUnknown* outer, REFIID riid, void** object) {
    if (!object) {
        return E_POINTER;
    }
    *object = nullptr;
    if (outer) {
        return CLASS_E_NOAGGREGATION;
    }

    auto* service = new (std::nothrow) TextService();
    if (!service) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = service->QueryInterface(riid, object);
    service->Release();
    return hr;
}

HRESULT ClassFactory::LockServer(BOOL lock) {
    if (lock) {
        dll_add_ref();
    } else {
        dll_release();
    }
    return S_OK;
}

}  // namespace localpinyin
