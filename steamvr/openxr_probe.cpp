#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D12
#define XR_NO_PROTOTYPES
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>
#include <cstdio>

// Diagnose Proton's runtime without loading, patching or authenticating D2R.
int main(int argc,char** argv) {
    // Optional output path makes this useful on a friend's machine as well.
    if(!std::freopen(argc>1?argv[1]:"openxr-probe-result.txt","w",stdout))return 1;
    std::setvbuf(stdout,nullptr,_IONBF,0);
    std::puts("Runtime probe started");
    auto module=LoadLibraryW(L"wineopenxr.dll");
    if(!module) module=LoadLibraryW(L"openxr_loader.dll");
    if(!module) {std::printf("Load OpenXR bridge/loader failed: %lu\n",GetLastError());return 1;}
    XrNegotiateRuntimeRequest rr{XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST,XR_RUNTIME_INFO_STRUCT_VERSION,sizeof(XrNegotiateRuntimeRequest)};
    rr.getInstanceProcAddr=reinterpret_cast<PFN_xrGetInstanceProcAddr>(GetProcAddress(module,"xrGetInstanceProcAddr"));
    XrResult result=XR_SUCCESS;
    if(!rr.getInstanceProcAddr) {
        auto negotiate=reinterpret_cast<PFN_xrNegotiateLoaderRuntimeInterface>(GetProcAddress(module,"xrNegotiateLoaderRuntimeInterface"));
        if(!negotiate) {std::puts("No OpenXR entrypoint");return 1;}
        XrNegotiateLoaderInfo li{XR_LOADER_INTERFACE_STRUCT_LOADER_INFO,XR_LOADER_INFO_STRUCT_VERSION,sizeof(XrNegotiateLoaderInfo),1,1,XR_API_VERSION_1_0,XR_API_VERSION_1_0};
        result=negotiate(&li,&rr);
        std::printf("WineOpenXR negotiation: %d\n",result);
        if(XR_FAILED(result)||!rr.getInstanceProcAddr)return 1;
    }
    PFN_xrCreateInstance create=nullptr;
    rr.getInstanceProcAddr(XR_NULL_HANDLE,"xrCreateInstance",reinterpret_cast<PFN_xrVoidFunction*>(&create));
    const char* extensions[]={XR_KHR_D3D12_ENABLE_EXTENSION_NAME};
    XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
    std::snprintf(ci.applicationInfo.applicationName,sizeof ci.applicationInfo.applicationName,"D2R runtime probe");
    ci.applicationInfo.apiVersion=XR_API_VERSION_1_0;
    ci.enabledExtensionCount=1;ci.enabledExtensionNames=extensions;
    XrInstance instance=XR_NULL_HANDLE;
    result=create(&ci,&instance);std::printf("Create D3D12-enabled OpenXR instance: %d\n",result);
    if(XR_FAILED(result))return 1;
    PFN_xrGetInstanceProperties props=nullptr;PFN_xrGetSystem system=nullptr;PFN_xrDestroyInstance destroy=nullptr;
    rr.getInstanceProcAddr(instance,"xrGetInstanceProperties",reinterpret_cast<PFN_xrVoidFunction*>(&props));
    rr.getInstanceProcAddr(instance,"xrGetSystem",reinterpret_cast<PFN_xrVoidFunction*>(&system));
    rr.getInstanceProcAddr(instance,"xrDestroyInstance",reinterpret_cast<PFN_xrVoidFunction*>(&destroy));
    XrInstanceProperties ip{XR_TYPE_INSTANCE_PROPERTIES};props(instance,&ip);std::printf("Runtime: %s\n",ip.runtimeName);
    XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};si.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId id=XR_NULL_SYSTEM_ID;result=system(instance,&si,&id);std::printf("Headset system: %d\n",result);
    if(XR_FAILED(result)){destroy(instance);return 1;}
    PFN_xrGetD3D12GraphicsRequirementsKHR requirements=nullptr;
    PFN_xrCreateSession createSession=nullptr;PFN_xrDestroySession destroySession=nullptr;
    rr.getInstanceProcAddr(instance,"xrGetD3D12GraphicsRequirementsKHR",reinterpret_cast<PFN_xrVoidFunction*>(&requirements));
    rr.getInstanceProcAddr(instance,"xrCreateSession",reinterpret_cast<PFN_xrVoidFunction*>(&createSession));
    rr.getInstanceProcAddr(instance,"xrDestroySession",reinterpret_cast<PFN_xrVoidFunction*>(&destroySession));
    XrGraphicsRequirementsD3D12KHR gr{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
    result=requirements(instance,id,&gr);std::printf("D3D12 graphics requirements: %d\n",result);
    if(XR_FAILED(result)){destroy(instance);return 1;}
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    HRESULT hr=CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr))hr=factory->EnumAdapterByLuid(gr.adapterLuid,IID_PPV_ARGS(&adapter));
    if(SUCCEEDED(hr))hr=D3D12CreateDevice(adapter.Get(),gr.minFeatureLevel,IID_PPV_ARGS(&device));
    D3D12_COMMAND_QUEUE_DESC qd{};qd.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    if(SUCCEEDED(hr))hr=device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue));
    std::printf("Create matching D3D12 device/queue: 0x%08lx\n",static_cast<unsigned long>(hr));
    if(FAILED(hr)){destroy(instance);return 1;}
    XrGraphicsBindingD3D12KHR gb{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};gb.device=device.Get();gb.queue=queue.Get();
    XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};sci.next=&gb;sci.systemId=id;
    XrSession session=XR_NULL_HANDLE;result=createSession(instance,&sci,&session);
    std::printf("Create Proton D3D12 OpenXR session: %d\n",result);
    // Do not begin the session or submit a frame: this probe only checks setup.
    if(XR_SUCCEEDED(result))destroySession(session);
    destroy(instance);return XR_FAILED(result)?1:0;
}
