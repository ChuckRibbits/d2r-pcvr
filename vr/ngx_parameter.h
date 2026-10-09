#pragma once

struct ID3D11Resource;
struct ID3D12Resource;

namespace dlssmv {

// NGX supplies a Windows/MSVC C++ object, including when running under Proton.
// MSVC reverses the overloads within each virtual Set/Get group; MinGW keeps
// declaration order. Repeating the SDK's overloaded declaration therefore calls
// the wrong methods in our MinGW build. Give every virtual slot a unique name
// in MSVC order, with non-virtual overloads for the callers. Do not add a virtual
// destructor or reorder these slots. NGX owns the object.
// ABI reference: NVIDIA/DLSS include/nvsdk_ngx_params.h.
class NgxParameter {
    virtual void SetPointer(const char*, void*) = 0;
    virtual void SetD3D12(const char*, ID3D12Resource*) = 0;
    virtual void SetD3D11(const char*, ID3D11Resource*) = 0;
    virtual void SetInt(const char*, int) = 0;
    virtual void SetUInt(const char*, unsigned int) = 0;
    virtual void SetDouble(const char*, double) = 0;
    virtual void SetFloat(const char*, float) = 0;
    virtual void SetUInt64(const char*, unsigned long long) = 0;
    virtual int GetPointer(const char*, void**) const = 0;
    virtual int GetD3D12(const char*, ID3D12Resource**) const = 0;
    virtual int GetD3D11(const char*, ID3D11Resource**) const = 0;
    virtual int GetInt(const char*, int*) const = 0;
    virtual int GetUInt(const char*, unsigned int*) const = 0;
    virtual int GetDouble(const char*, double*) const = 0;
    virtual int GetFloat(const char*, float*) const = 0;
    virtual int GetUInt64(const char*, unsigned long long*) const = 0;
    virtual void ResetSlot() = 0;

public:
    void Set(const char* n, void* v) { SetPointer(n, v); }
    void Set(const char* n, ID3D12Resource* v) { SetD3D12(n, v); }
    void Set(const char* n, ID3D11Resource* v) { SetD3D11(n, v); }
    void Set(const char* n, int v) { SetInt(n, v); }
    void Set(const char* n, unsigned int v) { SetUInt(n, v); }
    void Set(const char* n, double v) { SetDouble(n, v); }
    void Set(const char* n, float v) { SetFloat(n, v); }
    void Set(const char* n, unsigned long long v) { SetUInt64(n, v); }
    int Get(const char* n, void** v) const { return GetPointer(n, v); }
    int Get(const char* n, ID3D12Resource** v) const { return GetD3D12(n, v); }
    int Get(const char* n, ID3D11Resource** v) const { return GetD3D11(n, v); }
    int Get(const char* n, int* v) const { return GetInt(n, v); }
    int Get(const char* n, unsigned int* v) const { return GetUInt(n, v); }
    int Get(const char* n, double* v) const { return GetDouble(n, v); }
    int Get(const char* n, float* v) const { return GetFloat(n, v); }
    int Get(const char* n, unsigned long long* v) const { return GetUInt64(n, v); }
    void Reset() { ResetSlot(); }
};

} // namespace dlssmv
