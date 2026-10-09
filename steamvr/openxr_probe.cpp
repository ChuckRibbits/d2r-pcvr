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
#include <cstring>
#include <vector>

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
    PFN_xrEnumerateInstanceExtensionProperties enumerate=nullptr;
    rr.getInstanceProcAddr(XR_NULL_HANDLE,"xrEnumerateInstanceExtensionProperties",reinterpret_cast<PFN_xrVoidFunction*>(&enumerate));
    if(!create||!enumerate)return 1;
    uint32_t count=0;
    if(XR_FAILED(enumerate(nullptr,0,&count,nullptr)))return 1;
    std::vector<XrExtensionProperties> available(count,{XR_TYPE_EXTENSION_PROPERTIES});
    if(XR_FAILED(enumerate(nullptr,count,&count,available.data())))return 1;
    bool gaze=false;
    for(const auto& e:available)gaze|=std::strcmp(e.extensionName,XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME)==0;
    std::printf("Eye gaze extension advertised: %d\n",gaze);
    const char* extensions[]={XR_KHR_D3D12_ENABLE_EXTENSION_NAME,XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME};
    XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
    std::snprintf(ci.applicationInfo.applicationName,sizeof ci.applicationInfo.applicationName,"D2R runtime probe");
    ci.applicationInfo.apiVersion=XR_API_VERSION_1_0;
    ci.enabledExtensionCount=gaze?2:1;ci.enabledExtensionNames=extensions;
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
    if(gaze) {
        PFN_xrGetSystemProperties getSystemProperties=nullptr;
        rr.getInstanceProcAddr(instance,"xrGetSystemProperties",reinterpret_cast<PFN_xrVoidFunction*>(&getSystemProperties));
        XrSystemEyeGazeInteractionPropertiesEXT eye{XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT};
        XrSystemProperties sp{XR_TYPE_SYSTEM_PROPERTIES};sp.next=&eye;
        if(!getSystemProperties){destroy(instance);return 1;}
        result=getSystemProperties(instance,id,&sp);
        std::printf("Eye gaze system query: %d; supportsEyeGazeInteraction: %u\n",result,eye.supportsEyeGazeInteraction);
    }
    // Query capabilities without creating a competing graphics session.
    if(argc>2&&std::strcmp(argv[2],"--capabilities")==0){destroy(instance);return XR_FAILED(result)?1:0;}
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
    auto dxgi=LoadLibraryW(L"dxgi.dll"),d3d=LoadLibraryW(L"d3d12.dll");
    auto createFactory=reinterpret_cast<decltype(&CreateDXGIFactory1)>(dxgi?GetProcAddress(dxgi,"CreateDXGIFactory1"):nullptr);
    auto createDevice=reinterpret_cast<decltype(&D3D12CreateDevice)>(d3d?GetProcAddress(d3d,"D3D12CreateDevice"):nullptr);
    if(!createFactory||!createDevice){std::puts("Cannot load DXGI/D3D12");destroy(instance);return 1;}
    HRESULT hr=createFactory(IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr))hr=factory->EnumAdapterByLuid(gr.adapterLuid,IID_PPV_ARGS(&adapter));
    if(SUCCEEDED(hr))hr=createDevice(adapter.Get(),gr.minFeatureLevel,IID_PPV_ARGS(&device));
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
