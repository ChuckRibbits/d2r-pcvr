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
#include <reshade.hpp>
#include <xinput.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include "afr_eye_shared.h"
#include "d2r_vr_shared.h"
#include "pad_mirror_shared.h"
#include "transport.h"
#include "math.h"
#include "input.h"
#include "foveation.h"
#include "d2rcam.h"
#include "profile.h"

namespace framevr {
using Microsoft::WRL::ComPtr;
namespace {
constexpr float kRadToDeg=57.295779513f;
#define XR_FUNCTIONS(X) \
 X(EnumerateInstanceExtensionProperties) X(CreateInstance) X(DestroyInstance) \
 X(GetSystem) X(GetInstanceProperties) X(GetD3D12GraphicsRequirementsKHR) \
 X(CreateSession) X(DestroySession) X(PollEvent) X(BeginSession) X(EndSession) \
 X(CreateReferenceSpace) X(DestroySpace) X(LocateSpace) X(LocateViews) \
 X(WaitFrame) X(BeginFrame) X(EndFrame) X(EnumerateSwapchainFormats) \
 X(CreateSwapchain) X(DestroySwapchain) X(EnumerateSwapchainImages) \
 X(AcquireSwapchainImage) X(WaitSwapchainImage) X(ReleaseSwapchainImage) \
 X(StringToPath) X(CreateActionSet) X(DestroyActionSet) X(CreateAction) \
 X(CreateActionSpace) X(SuggestInteractionProfileBindings) X(AttachSessionActionSets) \
 X(SyncActions) X(GetActionStateBoolean) X(GetActionStateFloat) X(GetActionStateVector2f)
#define DECLARE(name) PFN_xr##name p##name=nullptr;
XR_FUNCTIONS(DECLARE)
#undef DECLARE
PFN_xrGetInstanceProcAddr getProc=nullptr;
HMODULE loader=nullptr;
LogFn logger=nullptr;
PanelFn wantsPanel=nullptr;
PanelFn inWorld=nullptr;
PanelFn toggleMenus=nullptr;
FrameTiming timing,lastTiming;
std::atomic<uint32_t> pipelineCreates{0};
std::atomic<double> maxPipelineMs{0};
std::atomic<bool> deferredWait{false};
std::atomic<bool> symmetricProjection{true};
double Milliseconds() {
    static const double scale=[](){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}();
    LARGE_INTEGER t{};QueryPerformanceCounter(&t);return double(t.QuadPart)*scale;
}
bool worldWasLoaded=false,worldCalibrationPending=false,worldRecenter=false;
std::atomic<bool> captureRequested{false};
bool capturePair=false;
uint32_t captureEyes=0,captureSerial=0;
std::recursive_mutex mutex;
XrInstance instance=XR_NULL_HANDLE;
XrSystemId system=XR_NULL_SYSTEM_ID;
XrSession session=XR_NULL_HANDLE;
XrSpace space=XR_NULL_HANDLE, headSpace=XR_NULL_HANDLE;
XrSessionState state=XR_SESSION_STATE_UNKNOWN;
bool running=false, frame=false, valid=false, render=false, registered=false, failed=false, pairDrawing=false;
XrFrameState frameState{XR_TYPE_FRAME_STATE};
std::array<XrView,2> views{{{XR_TYPE_VIEW},{XR_TYPE_VIEW}}};
XrPosef head{};
math::Vec origin{};
bool haveOrigin=false;
HeightResetButton heightResetButton;
MenuTriggerGate menuTriggerGate;
PressEdge inventoryButton,menuButton;
float lastRecenter=0;
uint32_t width=0,height=0;
DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
reshade::api::swapchain* gameSwapchain=nullptr;
ComPtr<ID3D12Device> device;
ComPtr<ID3D12CommandQueue> queue;
ComPtr<ID3D12Fence> fence;
HANDLE fenceEvent=nullptr;
uint64_t fenceValue=0;
ULONGLONG retryAt=0;
struct Eye {
    XrSwapchain chain=XR_NULL_HANDLE;
    std::vector<XrSwapchainImageD3D12KHR> images;
    uint32_t index=0;
    bool acquired=false, copied=false;
    uint64_t copyFence=0;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
};
std::array<Eye,2> eyes;
// Optional timestamp diagnostics. The ordinary pair fence already guarantees
// completion; reading these queries never introduces another GPU wait.
struct GpuProfile {
    ComPtr<ID3D12QueryHeap> heap;
    ComPtr<ID3D12Resource> readback;
    ComPtr<ID3D12CommandAllocator> allocator[2];
    ComPtr<ID3D12GraphicsCommandList> commands[2];
    uint64_t frequency=0;
    unsigned started=0,effects=0,resolved=0,eye=0;
    bool failed=false;
} gpuProfile;
FlatVRAfrEye* afr=nullptr;
HANDLE afrHandle=nullptr;

template<class T> struct Block {
    HANDLE handle=nullptr;
    T* data=nullptr;
    bool Open(const wchar_t* name) {
        handle=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(T),name);
        if (handle) data=static_cast<T*>(MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(T)));
        if (data) std::memset(data,0,sizeof(T));
        return data!=nullptr;
    }
    void Close() { if(data) UnmapViewOfFile(data);if(handle) CloseHandle(handle);data=nullptr;handle=nullptr; }
};
Block<D2RVR_Shared> tracking;
Block<BWPadMirror> pad;
Block<D2RVR_Actions> actions;
XrActionSet actionSet=XR_NULL_HANDLE;
XrAction gripPose=XR_NULL_HANDLE, stick=XR_NULL_HANDLE, trigger=XR_NULL_HANDLE, squeeze=XR_NULL_HANDLE;
XrAction aimPose=XR_NULL_HANDLE;
XrAction primary=XR_NULL_HANDLE, secondary=XR_NULL_HANDLE, stickClick=XR_NULL_HANDLE, menu=XR_NULL_HANDLE;
XrPath hands[2]{}; // right, left, matching the existing wire format.
XrSpace handSpaces[2]{};
XrSpace aimSpace=XR_NULL_HANDLE;
XrPosef menuPose{};
bool menuPlaced=false;
bool panelMode=false,pointerVisible=false,pointerHit=false,mouseDown=false,triggerWasDown=false,cancelWasDown=false;
HWND gameWindow=nullptr;
WORD heldKeyScan=0;
ULONGLONG keyReleaseAt=0;
constexpr float panelWidth=2.0f,panelDistance=1.4f;
XrSwapchain pointerChain=XR_NULL_HANDLE;
XrPosef beamPose{},dotPose{};
float beamLength=0;

math::Quat Q(XrQuaternionf q) { return {q.x,q.y,q.z,q.w}; }
math::Vec V(XrVector3f p) { return {p.x,p.y,p.z}; }
void Say(const char* msg) { if(logger) logger(msg); }
struct PipelineStart {reshade::api::device* device=nullptr;uint64_t layout=0;double at=0;};
thread_local PipelineStart pipelineStart;
bool PipelineCreating(reshade::api::device* dev,reshade::api::pipeline_layout layout,uint32_t,const reshade::api::pipeline_subobject*) {
    pipelineStart={dev,layout.handle,Milliseconds()};return false; // observe; do not change creation
}
void PipelineCreated(reshade::api::device* dev,reshade::api::pipeline_layout layout,uint32_t,const reshade::api::pipeline_subobject*,reshade::api::pipeline) {
    if(pipelineStart.device!=dev||pipelineStart.layout!=layout.handle||!pipelineStart.at)return;
    const double elapsed=Milliseconds()-pipelineStart.at;pipelineStart={};
    ++pipelineCreates;
    double previous=maxPipelineMs.load();while(elapsed>previous&&!maxPipelineMs.compare_exchange_weak(previous,elapsed)){}
    if(elapsed>20) {
        static std::atomic<ULONGLONG> lastLog{0};const auto now=GetTickCount64();auto last=lastLog.load();
        if(now-last>1000&&lastLog.compare_exchange_strong(last,now)) {
            char line[140];std::snprintf(line,sizeof line,"Frame pipeline: native pipeline creation took %.1f ms on thread %lu",elapsed,GetCurrentThreadId());Say(line);
        }
    }
}
bool Check(XrResult r,const char* name) {
    if(XR_SUCCEEDED(r)) return true;
    char b[160];std::snprintf(b,sizeof b,"Frame OpenXR: %s failed (%d)",name,int(r));Say(b);return false;
}
bool Hr(HRESULT r,const char* name) {
    if(SUCCEEDED(r)) return true;
    char b[160];std::snprintf(b,sizeof b,"Frame D3D12: %s failed (0x%08lx)",name,(unsigned long)r);Say(b);return false;
}
bool PrepareGpuProfile() {
    if(gpuProfile.failed)return false;
    if(gpuProfile.heap)return true;
    auto& p=gpuProfile;
    D3D12_QUERY_HEAP_DESC q{};q.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;q.Count=10;
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC r{};r.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;r.Width=10*sizeof(uint64_t);
    r.Height=r.DepthOrArraySize=r.MipLevels=1;r.SampleDesc.Count=1;r.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bool ok=SUCCEEDED(queue->GetTimestampFrequency(&p.frequency))&&p.frequency&&
        SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&r,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&p.readback)));
    for(int e=0;ok&&e<2;++e)ok=SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&p.allocator[e])))&&
        SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,p.allocator[e].Get(),nullptr,IID_PPV_ARGS(&p.commands[e])))&&SUCCEEDED(p.commands[e]->Close());
    if(ok)ok=SUCCEEDED(device->CreateQueryHeap(&q,IID_PPV_ARGS(&p.heap)));
    if(!ok){p.failed=true;Say("Frame profile: GPU timestamps unavailable; CPU diagnostics remain enabled");}
    return ok;
}
void StartGpuEye(unsigned eye) {
    auto& p=gpuProfile;p.eye=eye;
    if(!profile::Active()||!profile::GpuEnabled()||!PrepareGpuProfile())return;
    if(FAILED(p.allocator[eye]->Reset())||FAILED(p.commands[eye]->Reset(p.allocator[eye].Get(),nullptr))){p.failed=true;return;}
    p.commands[eye]->EndQuery(p.heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,eye*5);
    if(FAILED(p.commands[eye]->Close())){p.failed=true;return;}
    ID3D12CommandList* lists[]={p.commands[eye].Get()};queue->ExecuteCommandLists(1,lists);p.started|=1u<<eye;
}
void ProfileEffects(reshade::api::effect_runtime* rt,reshade::api::command_list* cl,reshade::api::resource_view,reshade::api::resource_view) {
    auto& p=gpuProfile;
    if(!profile::Active()||!pairDrawing||!(p.started&(1u<<p.eye))||!gameSwapchain||
       rt->get_current_back_buffer().handle!=gameSwapchain->get_current_back_buffer().handle)return;
    reinterpret_cast<ID3D12GraphicsCommandList*>(cl->get_native())->EndQuery(p.heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,p.eye*5+1);
    p.effects|=1u<<p.eye;
}
void ReadGpuProfile() {
    auto& p=gpuProfile;if(!profile::Active()||p.resolved!=3||p.failed)return;
    void* mapped=nullptr;D3D12_RANGE range{0,10*sizeof(uint64_t)};
    if(FAILED(p.readback->Map(0,&range,&mapped)))return;
    uint64_t ts[10];std::memcpy(ts,mapped,sizeof ts);D3D12_RANGE noWrites{0,0};p.readback->Unmap(0,&noWrites);
    bool valid=true;for(unsigned e=0;e<2;++e)for(unsigned i=1;i<5;++i)valid&=ts[e*5+i]>=ts[e*5+i-1];
    if(!valid)return;
    const double k=1000.0/p.frequency;
    for(unsigned e=0;e<2;++e){const auto* t=ts+e*5;timing.gpuSceneMs[e]=(t[1]-t[0])*k;timing.gpuEffectsMs[e]=(t[2]-t[1])*k;timing.gpuCopyMs[e]=(t[4]-t[3])*k;}
    timing.gpuValid=true;
}
void ReleaseUiKey() {
    if(!heldKeyScan)return;
    INPUT key{};key.type=INPUT_KEYBOARD;key.ki.wScan=heldKeyScan;
    key.ki.dwFlags=KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP;
    SendInput(1,&key,sizeof key);
    heldKeyScan=0;keyReleaseAt=0;
}
void PulseUiKey(WORD virtualKey) {
    ReleaseUiKey();
    const WORD scan=WORD(MapVirtualKeyW(virtualKey,MAPVK_VK_TO_VSC));
    if(!scan)return;
    INPUT key{};key.type=INPUT_KEYBOARD;key.ki.wScan=scan;key.ki.dwFlags=KEYEVENTF_SCANCODE;
    if(SendInput(1,&key,sizeof key)!=1){Say("Frame OpenXR: UI key injection failed");return;}
    heldKeyScan=scan;keyReleaseAt=GetTickCount64()+125;
    Say(virtualKey==VK_ESCAPE?"Frame OpenXR: menu Escape scan code down":"Frame OpenXR: inventory I scan code down");
}
void ClearInput() {
    ReleaseUiKey();
    if(mouseDown){INPUT up{};up.type=INPUT_MOUSE;up.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&up,sizeof up);mouseDown=false;}
    triggerWasDown=cancelWasDown=pointerVisible=pointerHit=false;
    heightResetButton={};
    // Tracking loss ends any pending mouse press, but a held trigger must
    // still be released before it can become a fresh world attack.
    menuTriggerGate.consumed=true;
    if(tracking.data) { tracking.data->headValid=0;tracking.data->handsValid=0;++tracking.data->counter; }
    if(pad.data) { const auto c=pad.data->counter;*pad.data={};pad.data->version=BW_PAD_MIRROR_VERSION;pad.data->writer_pid=GetCurrentProcessId();pad.data->counter=c+1; }
    if(actions.data) { actions.data->held=0;++actions.data->counter; }
}
bool LoadFunctions() {
    // Proton's WineOpenXR exports the runtime negotiation entrypoint.
    // Negotiate it directly, or use a standard Windows loader as a fallback.
    loader=LoadLibraryW(L"wineopenxr.dll");
    if(!loader) loader=LoadLibraryW(L"openxr_loader.dll");
    if(!loader) { Say("Frame OpenXR: no WineOpenXR/Windows OpenXR loader; use Proton 11 or Experimental");return false; }
    getProc=reinterpret_cast<PFN_xrGetInstanceProcAddr>(GetProcAddress(loader,"xrGetInstanceProcAddr"));
    if(!getProc) {
        auto negotiate=reinterpret_cast<PFN_xrNegotiateLoaderRuntimeInterface>(GetProcAddress(loader,"xrNegotiateLoaderRuntimeInterface"));
        if(!negotiate) {Say("Frame OpenXR: runtime exports neither getProc nor negotiation");return false;}
        XrNegotiateLoaderInfo li{XR_LOADER_INTERFACE_STRUCT_LOADER_INFO,XR_LOADER_INFO_STRUCT_VERSION,sizeof(XrNegotiateLoaderInfo),1,1,XR_API_VERSION_1_0,XR_API_VERSION_1_0};
        XrNegotiateRuntimeRequest rr{XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST,XR_RUNTIME_INFO_STRUCT_VERSION,sizeof(XrNegotiateRuntimeRequest)};
        if(!Check(negotiate(&li,&rr),"negotiate WineOpenXR")||!rr.getInstanceProcAddr)return false;
        getProc=rr.getInstanceProcAddr;
    }
    if(!Check(getProc(XR_NULL_HANDLE,"xrEnumerateInstanceExtensionProperties",reinterpret_cast<PFN_xrVoidFunction*>(&pEnumerateInstanceExtensionProperties)),"get enumerate extensions") ||
       !Check(getProc(XR_NULL_HANDLE,"xrCreateInstance",reinterpret_cast<PFN_xrVoidFunction*>(&pCreateInstance)),"get create instance")) return false;
    uint32_t count=0;
    if(!Check(pEnumerateInstanceExtensionProperties(nullptr,0,&count,nullptr),"enumerate extensions")) return false;
    std::vector<XrExtensionProperties> ext(count,{XR_TYPE_EXTENSION_PROPERTIES});
    if(!Check(pEnumerateInstanceExtensionProperties(nullptr,count,&count,ext.data()),"enumerate extensions")) return false;
    const auto supports=[&](const char* name){return std::any_of(ext.begin(),ext.end(),[&](const auto& e){return !std::strcmp(e.extensionName,name);});};
    char capabilities[180];
    std::snprintf(capabilities,sizeof capabilities,"Frame capability: eye gaze advertised=%d; FB foveation=%d; META eye-tracked foveation=%d (not enabled)",
        supports("XR_EXT_eye_gaze_interaction"),supports("XR_FB_foveation"),supports("XR_META_foveation_eye_tracked"));
    Say(capabilities);
    if(std::none_of(ext.begin(),ext.end(),[](const auto& e){return !std::strcmp(e.extensionName,XR_KHR_D3D12_ENABLE_EXTENSION_NAME);})) {
        Say("Frame OpenXR: runtime does not expose XR_KHR_D3D12_enable");return false;
    }
    const char* enabled[]={XR_KHR_D3D12_ENABLE_EXTENSION_NAME};
    XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
    std::strcpy(ci.applicationInfo.applicationName,"D2R Steam Frame prototype");
    ci.applicationInfo.applicationVersion=1;ci.applicationInfo.apiVersion=XR_API_VERSION_1_0;
    ci.enabledExtensionCount=1;ci.enabledExtensionNames=enabled;
    if(!Check(pCreateInstance(&ci,&instance),"create instance")) return false;
#define LOAD(name) if(!Check(getProc(instance,"xr" #name,reinterpret_cast<PFN_xrVoidFunction*>(&p##name)),"load " #name)) return false;
    XR_FUNCTIONS(LOAD)
#undef LOAD
    XrInstanceProperties props{XR_TYPE_INSTANCE_PROPERTIES};pGetInstanceProperties(instance,&props);
    char b[192];std::snprintf(b,sizeof b,"Frame OpenXR: runtime %s",props.runtimeName);Say(b);
    XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};si.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    return Check(pGetSystem(instance,&si,&system),"get headset system");
}
XrPath Path(const char* p) { XrPath out=XR_NULL_PATH;pStringToPath(instance,p,&out);return out; }
bool MakeAction(XrAction& out,const char* name,XrActionType type) {
    XrActionCreateInfo ci{XR_TYPE_ACTION_CREATE_INFO};
    std::snprintf(ci.actionName,sizeof ci.actionName,"%s",name);
    std::snprintf(ci.localizedActionName,sizeof ci.localizedActionName,"%s",name);
    ci.actionType=type;ci.countSubactionPaths=2;ci.subactionPaths=hands;
    return Check(pCreateAction(actionSet,&ci,&out),"create controller action");
}
bool SetupActions() {
    hands[0]=Path("/user/hand/right");hands[1]=Path("/user/hand/left");
    XrActionSetCreateInfo ci{XR_TYPE_ACTION_SET_CREATE_INFO};
    std::strcpy(ci.actionSetName,"d2r");std::strcpy(ci.localizedActionSetName,"Diablo II");
    if(!Check(pCreateActionSet(instance,&ci,&actionSet),"create action set")) return false;
    if(!MakeAction(gripPose,"hands",XR_ACTION_TYPE_POSE_INPUT) || !MakeAction(stick,"sticks",XR_ACTION_TYPE_VECTOR2F_INPUT) ||
       !MakeAction(aimPose,"menu_aim",XR_ACTION_TYPE_POSE_INPUT) ||
       !MakeAction(trigger,"triggers",XR_ACTION_TYPE_FLOAT_INPUT) || !MakeAction(squeeze,"grips",XR_ACTION_TYPE_FLOAT_INPUT) ||
       !MakeAction(primary,"primary",XR_ACTION_TYPE_BOOLEAN_INPUT) || !MakeAction(secondary,"secondary",XR_ACTION_TYPE_BOOLEAN_INPUT) ||
       !MakeAction(stickClick,"stick_click",XR_ACTION_TYPE_BOOLEAN_INPUT) || !MakeAction(menu,"menu",XR_ACTION_TYPE_BOOLEAN_INPUT)) return false;
    for(const auto* profile:{"/interaction_profiles/valve/index_controller","/interaction_profiles/oculus/touch_controller","/interaction_profiles/microsoft/motion_controller"}) {
        const bool index=std::strstr(profile,"index"), touch=std::strstr(profile,"touch");
        std::vector<XrActionSuggestedBinding> bindings;
        for(int i=0;i<2;++i) {
            const std::string base=i==0?"/user/hand/right/input/":"/user/hand/left/input/";
            auto bind=[&](XrAction a,const char* suffix){bindings.push_back({a,Path((base+suffix).c_str())});};
            bind(gripPose,"grip/pose");bind(stick,"thumbstick");bind(stickClick,"thumbstick/click");bind(trigger,"trigger/value");
            bind(aimPose,"aim/pose");
            // Motion controllers expose squeeze/click (boolean), so omit our float grip there.
            if(index||touch) bind(squeeze,index?"squeeze/force":"squeeze/value");
            if(index||touch) {
                bind(primary,touch&&i==1?"x/click":"a/click");
                bind(secondary,touch&&i==1?"y/click":"b/click");
            }
            if((touch&&i==1)||(!index&&!touch)) bind(menu,"menu/click");
        }
        XrInteractionProfileSuggestedBinding suggestion{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        suggestion.interactionProfile=Path(profile);suggestion.countSuggestedBindings=uint32_t(bindings.size());suggestion.suggestedBindings=bindings.data();
        Check(pSuggestInteractionProfileBindings(instance,&suggestion),profile);
    }
    XrSessionActionSetsAttachInfo ai{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};ai.countActionSets=1;ai.actionSets=&actionSet;
    if(!Check(pAttachSessionActionSets(session,&ai),"attach action set")) return false;
    for(int i=0;i<2;++i) {
        XrActionSpaceCreateInfo sci{XR_TYPE_ACTION_SPACE_CREATE_INFO};sci.action=gripPose;sci.subactionPath=hands[i];sci.poseInActionSpace.orientation.w=1;
        if(!Check(pCreateActionSpace(session,&sci,&handSpaces[i]),"create hand space")) return false;
    }
    XrActionSpaceCreateInfo sci{XR_TYPE_ACTION_SPACE_CREATE_INFO};sci.action=aimPose;sci.subactionPath=hands[0];sci.poseInActionSpace.orientation.w=1;
    if(!Check(pCreateActionSpace(session,&sci,&aimSpace),"create menu aim space"))return false;
    return true;
}
bool WaitGpu();
bool WaitGpuValue(uint64_t value);
bool SetupPointer(DXGI_FORMAT eyeFormat) {
    // A tiny static atlas is independent of ReShade's effect compiler.
    XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.createFlags=XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
    ci.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format=eyeFormat;ci.width=64;ci.height=96;ci.sampleCount=ci.faceCount=ci.arraySize=ci.mipCount=1;
    if(!Check(pCreateSwapchain(session,&ci,&pointerChain),"create laser atlas"))return false;
    uint32_t count=0;
    if(!Check(pEnumerateSwapchainImages(pointerChain,0,&count,nullptr),"count laser images"))return false;
    std::vector<XrSwapchainImageD3D12KHR> images(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
    if(!Check(pEnumerateSwapchainImages(pointerChain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),"get laser images"))return false;
    uint32_t index=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if(!Check(pAcquireSwapchainImage(pointerChain,&ai,&index),"acquire laser atlas"))return false;
    XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;
    if(!Check(pWaitSwapchainImage(pointerChain,&wi),"wait laser atlas"))return false;
    auto* dest=images[index].texture;const auto desc=dest->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
    device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC uploadDesc{};uploadDesc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;uploadDesc.Width=bytes;
    uploadDesc.Height=uploadDesc.DepthOrArraySize=uploadDesc.MipLevels=uploadDesc.SampleDesc.Count=1;uploadDesc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> upload;
    if(!Hr(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&uploadDesc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)),"create laser upload"))return false;
    uint8_t* data=nullptr;D3D12_RANGE readRange{};
    if(!Hr(upload->Map(0,&readRange,reinterpret_cast<void**>(&data)),"map laser upload"))return false;
    std::memset(data,0,size_t(bytes));
    for(int y=0;y<96;++y)for(int x=0;x<64;++x) {
        auto* pixel=data+footprint.Offset+y*footprint.Footprint.RowPitch+x*4;
        pixel[0]=eyeFormat==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB?255:40;pixel[1]=210;
        pixel[2]=eyeFormat==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB?40:255;
        const float radius=std::hypot((x-31.5f)/30.f,(y-63.5f)/30.f);
        pixel[3]=y<32?200:uint8_t(std::clamp((1-radius)*10,0.f,1.f)*255);
    }
    upload->Unmap(0,nullptr);
    ComPtr<ID3D12CommandAllocator> alloc;ComPtr<ID3D12GraphicsCommandList> list;
    if(!Hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&alloc)),"create laser allocator")||
       !Hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,alloc.Get(),nullptr,IID_PPV_ARGS(&list)),"create laser list"))return false;
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={dest,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST};
    list->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint;
    D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=dest;dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
    if(!Hr(list->Close(),"close laser list"))return false;
    ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
    if(!Hr(queue->Signal(fence.Get(),++fenceValue),"signal laser fence")||!WaitGpu())return false;
    XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    return Check(pReleaseSwapchainImage(pointerChain,&ri),"release laser atlas");
}
bool Initialize(reshade::api::command_queue* rq,reshade::api::swapchain* swap) {
    if(rq->get_device()->get_api()!=reshade::api::device_api::d3d12) { Say("Frame OpenXR requires D3D12");return false; }
    if(!LoadFunctions()) return false;
    device=reinterpret_cast<ID3D12Device*>(rq->get_device()->get_native());
    queue=reinterpret_cast<ID3D12CommandQueue*>(rq->get_native());
    // This MinGW SDK defines OPTIONS6 but omits its data structure. Match the
    // Windows SDK's five 32-bit fields (the tier enum has UINT representation).
    struct VrsFeatureData {
        BOOL AdditionalShadingRatesSupported;
        BOOL PerPrimitiveShadingRateSupportedWithViewportIndexing;
        UINT VariableShadingRateTier;
        UINT ShadingRateImageTileSize;
        BOOL BackgroundProcessingSupported;
    } vrs{};
    static_assert(sizeof(VrsFeatureData)==20);
    if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6,&vrs,sizeof vrs))) {
        char capabilities[160];
        std::snprintf(capabilities,sizeof capabilities,"Frame capability: D3D12 VRS tier=%d tile=%u additional rates=%d (not enabled)",
            int(vrs.VariableShadingRateTier),vrs.ShadingRateImageTileSize,int(vrs.AdditionalShadingRatesSupported));
        Say(capabilities);
    } else Say("Frame capability: D3D12 VRS feature query unsupported (not enabled)");
    XrGraphicsRequirementsD3D12KHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR};
    if(!Check(pGetD3D12GraphicsRequirementsKHR(instance,system,&req),"get D3D12 requirements")) return false;
    const LUID actual=device->GetAdapterLuid();
    if(actual.HighPart!=req.adapterLuid.HighPart||actual.LowPart!=req.adapterLuid.LowPart) { Say("Frame OpenXR: game and SteamVR are using different GPUs");return false; }
    D3D12_FEATURE_DATA_FEATURE_LEVELS levels{};levels.NumFeatureLevels=1;levels.pFeatureLevelsRequested=&req.minFeatureLevel;
    if(!Hr(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS,&levels,sizeof levels),"check feature level")) return false;
    XrGraphicsBindingD3D12KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D12_KHR};binding.device=device.Get();binding.queue=queue.Get();
    XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};ci.systemId=system;ci.next=&binding;
    if(!Check(pCreateSession(instance,&ci,&session),"create D3D12 session")) return false;
    XrReferenceSpaceCreateInfo sci{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};sci.poseInReferenceSpace.orientation.w=1;sci.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_STAGE;
    if(XR_FAILED(pCreateReferenceSpace(session,&sci,&space))) { sci.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;if(!Check(pCreateReferenceSpace(session,&sci,&space),"create local space"))return false; }
    sci.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;
    if(!Check(pCreateReferenceSpace(session,&sci,&headSpace),"create head space")||!SetupActions()) return false;
    auto* buffer=reinterpret_cast<ID3D12Resource*>(swap->get_current_back_buffer().handle);
    const auto desc=buffer->GetDesc();width=uint32_t(desc.Width);height=desc.Height;format=desc.Format;
    foveation::Initialize(device.Get(),queue.Get(),width,height);
    uint32_t n=0;pEnumerateSwapchainFormats(session,0,&n,nullptr);
    std::vector<int64_t> formats(n);
    if(!Check(pEnumerateSwapchainFormats(session,n,&n,formats.data()),"enumerate formats")) return false;
    DXGI_FORMAT eyeFormat=format;
    if(std::find(formats.begin(),formats.end(),int64_t(eyeFormat))==formats.end()) {
        // CopyResource permits UNORM/SRGB members of the same typeless family.
        // The game's SDR backbuffer is already display-encoded, so preserve bytes.
        if(format==DXGI_FORMAT_R8G8B8A8_UNORM)eyeFormat=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        else if(format==DXGI_FORMAT_B8G8R8A8_UNORM)eyeFormat=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        if(std::find(formats.begin(),formats.end(),int64_t(eyeFormat))==formats.end()) {
            std::string message="Frame OpenXR: unsupported game backbuffer format "+std::to_string(int(format))+"; runtime formats:";
            for(auto f:formats)message+=" "+std::to_string(f);
            Say(message.c_str());return false;
        }
    }
    {char message[128];std::snprintf(message,sizeof message,"Frame OpenXR: game format %d, eye format %d",int(format),int(eyeFormat));Say(message);}
    for(auto& e:eyes) {
        XrSwapchainCreateInfo c{XR_TYPE_SWAPCHAIN_CREATE_INFO};c.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        c.format=eyeFormat;c.width=width;c.height=height;c.sampleCount=1;c.faceCount=1;c.arraySize=1;c.mipCount=1;
        if(!Check(pCreateSwapchain(session,&c,&e.chain),"create eye swapchain")) return false;
        n=0;pEnumerateSwapchainImages(e.chain,0,&n,nullptr);e.images.resize(n,{XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR});
        if(!Check(pEnumerateSwapchainImages(e.chain,n,&n,reinterpret_cast<XrSwapchainImageBaseHeader*>(e.images.data())),"enumerate eye textures")) return false;
        if(!Hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&e.allocator)),"create eye copy allocator")||
           !Hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,e.allocator.Get(),nullptr,IID_PPV_ARGS(&e.commands)),"create eye copy list")||
           !Hr(e.commands->Close(),"close initial eye copy list"))return false;
    }
    if(!Hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"create fence")) return false;
    fenceEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!fenceEvent) return false;
    if(!SetupPointer(eyeFormat))return false;
    if(!tracking.Open(D2RVR_SHARED_NAME)||!pad.Open(BW_PAD_MIRROR_NAME)||!actions.Open(D2RVR_ACTIONS_NAME)) return false;
    tracking.data->version=D2RVR_SHARED_VERSION;pad.data->version=BW_PAD_MIRROR_VERSION;actions.data->version=D2RVR_ACTIONS_VERSION;
    gameSwapchain=swap;
    char b[128];std::snprintf(b,sizeof b,"Frame OpenXR: D3D12 session ready, eyes %ux%u format %d",width,height,int(format));Say(b);
    return true;
}
bool Bool(XrAction action,int hand) {
    XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=action;gi.subactionPath=hands[hand];
    XrActionStateBoolean v{XR_TYPE_ACTION_STATE_BOOLEAN};return XR_SUCCEEDED(pGetActionStateBoolean(session,&gi,&v))&&v.isActive&&v.currentState;
}
float Float(XrAction action,int hand) {
    XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=action;gi.subactionPath=hands[hand];
    XrActionStateFloat v{XR_TYPE_ACTION_STATE_FLOAT};return XR_SUCCEEDED(pGetActionStateFloat(session,&gi,&v))&&v.isActive?v.currentState:0;
}
XrVector2f Stick(int hand) {
    XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};gi.action=stick;gi.subactionPath=hands[hand];
    XrActionStateVector2f v{XR_TYPE_ACTION_STATE_VECTOR2F};if(XR_FAILED(pGetActionStateVector2f(session,&gi,&v))||!v.isActive)return {};return v.currentState;
}
bool FocusGameForUi() {
    // Called only for an intentional menu action in a focused XR session.
    // Hovering the laser must not steal focus from another desktop window.
    if(!gameWindow||!IsWindow(gameWindow))return false;
    if(GetForegroundWindow()==gameWindow)return true;
    SetForegroundWindow(gameWindow);
    const bool focused=GetForegroundWindow()==gameWindow;
    Say(focused?"Frame OpenXR: menu action restored game window focus":
        "Frame OpenXR: game window focus unavailable; OS mouse/keyboard injection withheld");
    return focused;
}
void MenuPointer() {
    pointerVisible=pointerHit=false;
    const bool pressed=Float(trigger,0)>.55f,cancel=Bool(secondary,0);
    bool focused=gameWindow&&GetForegroundWindow()==gameWindow;
    if(panelMode&&cancel&&!cancelWasDown&&!focused)focused=FocusGameForUi();
    XrSpaceLocation aim{XR_TYPE_SPACE_LOCATION};
    const auto flags=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if(panelMode&&menuPlaced&&XR_SUCCEEDED(pLocateSpace(aimSpace,space,frameState.predictedDisplayTime,&aim))&&(aim.locationFlags&flags)==flags) {
        const auto ray=V(aim.pose.position),direction=math::Rotate(Q(aim.pose.orientation),{0,0,-1});
        math::PanelHit hit;
        pointerHit=math::IntersectPanel(ray,direction,V(menuPose.position),Q(menuPose.orientation),panelWidth,panelWidth*height/width,hit);
        beamLength=pointerHit?hit.distance:panelDistance*1.8f;
        const auto mid=ray+direction*(beamLength*.5f);
        const auto rot=math::BeamOrientation(direction,V(head.position)-mid);
        beamPose.position={mid.x,mid.y,mid.z};beamPose.orientation={rot.x,rot.y,rot.z,rot.w};pointerVisible=true;
        if(pointerHit) {
            if(pressed&&!triggerWasDown&&!focused)focused=FocusGameForUi();
            const auto point=hit.point+math::Rotate(Q(menuPose.orientation),{0,0,.006f});
            dotPose=menuPose;dotPose.position={point.x,point.y,point.z};
            RECT client{};
            if(focused&&GetClientRect(gameWindow,&client)) {
                POINT pixel{LONG(hit.u*(client.right-1)),LONG(hit.v*(client.bottom-1))};
                if(ClientToScreen(gameWindow,&pixel))SetCursorPos(pixel.x,pixel.y);
            }
        }
    }
    const bool click=focused&&pointerHit&&pressed;
    if(click&&!triggerWasDown) {
        INPUT down{};down.type=INPUT_MOUSE;down.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
        mouseDown=SendInput(1,&down,sizeof down)==1;
        if(mouseDown)Say("Frame OpenXR: menu laser trigger down");
    }
    if(mouseDown&&!click) {
        INPUT up{};up.type=INPUT_MOUSE;up.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&up,sizeof up);mouseDown=false;
    }
    if(panelMode&&focused&&cancel&&!cancelWasDown) {
        PulseUiKey(VK_ESCAPE);
    }
    triggerWasDown=pressed;cancelWasDown=cancel;
}
void Input() {
    if(!tracking.data) return;
    if(!valid||state!=XR_SESSION_STATE_FOCUSED) { ClearInput();return; }
    XrActiveActionSet active{actionSet,XR_NULL_PATH};
    XrActionsSyncInfo si{XR_TYPE_ACTIONS_SYNC_INFO};si.countActiveActionSets=1;si.activeActionSets=&active;
    if(XR_FAILED(pSyncActions(session,&si))) {ClearInput();return;}
    const bool inventoryPressed=inventoryButton.Update(Bool(primary,1));
    const bool menuPressed=menuButton.Update(Bool(menu,1)||Bool(menu,0));
    bool focused=gameWindow&&GetForegroundWindow()==gameWindow;
    if((menuPressed||inventoryPressed)&&!focused)focused=FocusGameForUi();
    if(heldKeyScan&&(!focused||GetTickCount64()>=keyReleaseAt))ReleaseUiKey();
    if(focused&&menuPressed)PulseUiKey(VK_ESCAPE);
    if(inventoryPressed&&inWorld&&inWorld()) {
        // Keyboard shortcuts can be gated after switching input layouts or
        // panels. Queue the game's own panel actions on its UI thread instead.
        // This action targets D2R directly and only requires XR focus (checked
        // above), so a desktop terminal cannot silently swallow Left X.
        if(toggleMenus&&toggleMenus())Say("Frame OpenXR: Left X menu toggle queued");
        else Say("Frame OpenXR: Left X menu toggle unavailable");
    }
    MenuPointer();
    auto& s=*tracking.data;s.version=D2RVR_SHARED_VERSION;s.headValid=1;
    const auto q=Q(head.orientation);
    const auto forward=math::Rotate(q,{0,0,-1});
    const float heading=math::Heading(q);
    const auto level=math::Multiply(math::Conjugate(math::Yaw(heading)),q);
    const auto right=math::Rotate(level,{1,0,0});
    s.headYawDeg=heading*kRadToDeg;s.headPitchDeg=std::asin(std::clamp(forward.y,-1.f,1.f))*kRadToDeg;
    s.headRollDeg=-std::atan2(right.y,right.x)*kRadToDeg;s.pitchValid=s.rollValid=1;
    s.headHeightM=head.position.y;s.sampleStamp=D2RVRStampNow();s.sampleStampMagic=D2RVR_SAMPLE_STAMP_MAGIC;
    s.roomMagic=D2RVR_ROOM_MAGIC;s.headRoom[0]=head.position.x;s.headRoom[1]=head.position.y;s.headRoom[2]=head.position.z;s.headYawRoomDeg=s.headYawDeg;
    s.gripMagic=D2RVR_GRIP_MAGIC;s.rightGrip=Float(squeeze,0);s.leftGrip=Float(squeeze,1);s.handsValid=0;
    if(heightResetButton.Update(Bool(stickClick,1),s.leftGrip))ResetHeight();
    const auto undo=math::Conjugate(math::Yaw(heading));
    for(int i=0;i<2;++i) {
        XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
        const XrSpaceLocationFlags required=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if(XR_SUCCEEDED(pLocateSpace(handSpaces[i],space,frameState.predictedDisplayTime,&loc))&&(loc.locationFlags&required)==required) {
            s.handsValid|=1u<<i;
            const auto p=math::Rotate(undo,V(loc.pose.position)-V(head.position));
            const auto rot=math::Multiply(undo,Q(loc.pose.orientation));
            float* dest=i==0?s.rightHand:s.leftHand;dest[0]=p.x;dest[1]=p.y;dest[2]=p.z;
            float* r=i==0?s.rightRot:s.leftRot;r[0]=rot.x;r[1]=rot.y;r[2]=rot.z;r[3]=rot.w;
        }
    }
    ++s.counter;
    auto& p=*pad.data;p.writer_pid=GetCurrentProcessId();p.buttons=0;
    if(Bool(primary,0))p.buttons|=XINPUT_GAMEPAD_A;if(Bool(secondary,0))p.buttons|=XINPUT_GAMEPAD_B;
    if(Bool(secondary,1))p.buttons|=XINPUT_GAMEPAD_Y;
    if(heightResetButton.MapPressed())p.buttons|=XINPUT_GAMEPAD_LEFT_THUMB;if(Bool(stickClick,0))p.buttons|=XINPUT_GAMEPAD_RIGHT_THUMB;
    const auto l=Stick(1),r=Stick(0);
    auto axis=[](float a){return int16_t(std::clamp(a,-1.f,1.f)*32767.f);};
    p.thumb_lx=axis(l.x);p.thumb_ly=axis(l.y);p.thumb_rx=axis(r.x);p.thumb_ry=axis(r.y);
    p.left_trigger=uint8_t(std::clamp(Float(trigger,1),0.f,1.f)*255);
    const float rightTrigger=Float(trigger,0);
    const bool attackAllowed=menuTriggerGate.Update(rightTrigger>.1f,panelMode||inventoryPressed||menuPressed);
    p.right_trigger=attackAllowed?uint8_t(std::clamp(rightTrigger,0.f,1.f)*255):0;
    auto& a=*actions.data;a.held=0;
    if(p.right_trigger>128)a.held|=D2RVR_ACT_ATTACK;
    if(Bool(primary,0))a.held|=D2RVR_ACT_INTERACT;
    if(heightResetButton.MapPressed())a.held|=D2RVR_ACT_MAP;
    if(Bool(stickClick,0))a.held|=D2RVR_ACT_SWAP;
    if(panelMode) {
        // Mouse selection must not also activate a gamepad action or switch the
        // inventory back to its controller layout on every trigger press.
        p.buttons=0;p.left_trigger=p.right_trigger=0;
        p.thumb_lx=p.thumb_ly=p.thumb_rx=p.thumb_ry=0;a.held=0;
    }
    ++p.counter;++a.counter;
}
void Events() {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    while(pPollEvent(instance,&event)==XR_SUCCESS) {
        if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            const auto& s=*reinterpret_cast<XrEventDataSessionStateChanged*>(&event);state=s.state;
            char b[96];std::snprintf(b,sizeof b,"Frame OpenXR: session state %d",int(state));Say(b);
            if(state==XR_SESSION_STATE_READY) { XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};bi.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;running=Check(pBeginSession(session,&bi),"begin session"); }
            if(state==XR_SESSION_STATE_STOPPING) { if(frame)EndPair();pEndSession(session);running=false;ClearInput(); }
            if(state==XR_SESSION_STATE_LOSS_PENDING||state==XR_SESSION_STATE_EXITING) {running=false;valid=false;ClearInput();}
        } else if(event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {running=false;valid=false;ClearInput();}
        event={XR_TYPE_EVENT_DATA_BUFFER};
    }
}
bool Begin() {
    if(!session||frame) return frame;
    Events();if(!running)return false;
    XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};frameState={XR_TYPE_FRAME_STATE};
    timing={};const double waitStart=Milliseconds();
    const auto waited=pWaitFrame(session,&wi,&frameState);timing.waitFrameMs=Milliseconds()-waitStart;
    if(!Check(waited,"wait frame"))return false;
    XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};
    if(!Check(pBeginFrame(session,&bi),"begin frame"))return false;
    frame=true;render=frameState.shouldRender;valid=false;
    const bool loaded=inWorld&&inWorld();
    if(loaded&&!worldWasLoaded)worldCalibrationPending=true;
    if(!loaded)worldCalibrationPending=worldRecenter=false;
    worldWasLoaded=loaded;
    const bool panel=wantsPanel&&wantsPanel();
    if(panel!=panelMode){panelMode=panel;menuPlaced=false;Say(panel?"Frame OpenXR: flat menu panel active":"Frame OpenXR: stereo gameplay active");}
    for(auto& e:eyes)e.copied=false;
    XrViewLocateInfo li{XR_TYPE_VIEW_LOCATE_INFO};li.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;li.displayTime=frameState.predictedDisplayTime;li.space=space;
    XrViewState vs{XR_TYPE_VIEW_STATE};uint32_t count=0;
    const double locateStart=Milliseconds();
    const auto flags=XR_VIEW_STATE_POSITION_VALID_BIT|XR_VIEW_STATE_ORIENTATION_VALID_BIT;
    XrSpaceLocation hs{XR_TYPE_SPACE_LOCATION};
    if(XR_SUCCEEDED(pLocateViews(session,&li,&vs,2,&count,views.data()))&&count==2&&(vs.viewStateFlags&flags)==flags&&
       XR_SUCCEEDED(pLocateSpace(headSpace,space,li.displayTime,&hs))&&
       (hs.locationFlags&(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))==(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) {
        head=hs.pose;valid=true;
        // Latch once per pair: camera matrices, foveation and xrEndFrame must
        // all describe the same frustum, including a live settings change.
        // LocateViews refreshes the original runtime FOVs on every frame.
        const bool centered=symmetricProjection.load();
        if(centered) {
            const auto& l=views[0].fov;const auto& r=views[1].fov;
            const auto f=math::EnclosingSymmetricFov(
                {l.angleLeft,l.angleRight,l.angleDown,l.angleUp},
                {r.angleLeft,r.angleRight,r.angleDown,r.angleUp});
            for(auto& view:views)view.fov={f.left,f.right,f.up,f.down};
        }
        static int lastProjectionMode=-1;
        if(lastProjectionMode!=int(centered)) {
            const auto& f=views[0].fov;char b[192];
            std::snprintf(b,sizeof b,"Frame OpenXR: projection %s; left FOV %.5f %.5f %.5f %.5f",
                centered?"symmetric lighting trial":"runtime recommended",f.angleLeft,f.angleRight,f.angleUp,f.angleDown);
            Say(b);lastProjectionMode=int(centered);
        }
        if(!haveOrigin){origin=V(head.position);haveOrigin=true;}
        if(worldCalibrationPending&&state==XR_SESSION_STATE_FOCUSED) {
            origin=V(head.position);worldCalibrationPending=false;worldRecenter=true;
            Say("Frame OpenXR: loaded game calibrated from current headset position");
        }
        if(panelMode&&!menuPlaced) {
            const auto yaw=math::Yaw(math::Heading(Q(head.orientation)));
            const auto pos=V(head.position)+math::Rotate(yaw,{0,0,-panelDistance});
            menuPose.position={pos.x,pos.y,pos.z};menuPose.orientation={yaw.x,yaw.y,yaw.z,yaw.w};menuPlaced=true;
        }
    }
    timing.locateMs=Milliseconds()-locateStart;
    const double inputStart=Milliseconds();Input();timing.inputMs=Milliseconds()-inputStart;return true;
}
bool Acquire(Eye& e) {
    if(e.acquired)return true;
    XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if(!Check(pAcquireSwapchainImage(e.chain,&ai,&e.index),"acquire eye"))return false;
    e.acquired=true;
    XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wi.timeout=XR_INFINITE_DURATION;
    return Check(pWaitSwapchainImage(e.chain,&wi),"wait eye");
}
bool WaitGpu() {
    return WaitGpuValue(fenceValue);
}
bool WaitGpuValue(uint64_t value) {
    if(!value||!fence||fence->GetCompletedValue()>=value)return true;
    if(!Hr(fence->SetEventOnCompletion(value,fenceEvent),"fence completion"))return false;
    // Bounded: a device loss must not hang the game's render thread forever.
    const double start=Milliseconds();const auto result=WaitForSingleObject(fenceEvent,5000);
    timing.waitGpuMs+=Milliseconds()-start;
    if(result!=WAIT_OBJECT_0) {Say("Frame D3D12: GPU copy fence timed out");return false;}
    return true;
}
bool Copy(ID3D12Resource* src,Eye& e,reshade::api::command_queue* rq) {
    // The runtime orders images on our bound D3D12 queue. Our allocator has a
    // separate lifetime: never reset it until its own previous copy completed.
    // Waiting here allows game/CPU work to overlap the preceding GPU frame.
    if(e.copied)return true;
    const auto eye=unsigned(&e-eyes.data());const double acquireStart=Milliseconds();
    if(!Acquire(e))return false;
    timing.acquireMs[eye]+=Milliseconds()-acquireStart;
    if(!WaitGpuValue(e.copyFence))return false;
    const double copyStart=Milliseconds();
    const bool gpu=profile::Active()&&(gpuProfile.effects&(1u<<eye))&&!gpuProfile.failed;
    if(gpu)reinterpret_cast<ID3D12GraphicsCommandList*>(rq->get_immediate_command_list()->get_native())->EndQuery(gpuProfile.heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,eye*5+2);
    rq->flush_immediate_command_list(); // finish all ReShade writes before using the native queue.
    if(!Hr(e.allocator->Reset(),"reset allocator")||!Hr(e.commands->Reset(e.allocator.Get(),nullptr),"reset copy list"))return false;
    ID3D12Resource* dest=e.images[e.index].texture;
    D3D12_RESOURCE_BARRIER barriers[2]{};
    for(auto& b:barriers){b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;}
    barriers[0].Transition.pResource=src;barriers[0].Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;barriers[0].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[1].Transition.pResource=dest;barriers[1].Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;barriers[1].Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_DEST;
    if(gpu)e.commands->EndQuery(gpuProfile.heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,eye*5+3);
    e.commands->ResourceBarrier(2,barriers);e.commands->CopyResource(dest,src);
    for(auto& b:barriers)std::swap(b.Transition.StateBefore,b.Transition.StateAfter);
    e.commands->ResourceBarrier(2,barriers);
    if(gpu){
        e.commands->EndQuery(gpuProfile.heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,eye*5+4);
        e.commands->ResolveQueryData(gpuProfile.heap.Get(),D3D12_QUERY_TYPE_TIMESTAMP,eye*5,5,gpuProfile.readback.Get(),eye*5*sizeof(uint64_t));
        gpuProfile.resolved|=1u<<eye;
    }
    if(!Hr(e.commands->Close(),"close copy list"))return false;
    ID3D12CommandList* lists[]={e.commands.Get()};queue->ExecuteCommandLists(1,lists);
    if(!Hr(queue->Signal(fence.Get(),++fenceValue),"signal copy fence"))return false;
    e.copyFence=fenceValue;
    timing.copySubmitMs[eye]+=Milliseconds()-copyStart;
    e.copied=true;return true;
}
void Capture(reshade::api::command_queue* rq,reshade::api::swapchain* swap) {
    std::lock_guard lock(mutex);
    if(failed)return;
    if(!session)return; // Prepare owns initialization and its bounded retry interval.
    if(swap!=gameSwapchain)return;
    auto* src=reinterpret_cast<ID3D12Resource*>(swap->get_current_back_buffer().handle);
    const auto desc=src->GetDesc();
    if(desc.Width!=width||desc.Height!=height||desc.Format!=format) {
        Say("Frame OpenXR: window resized; recreating eye swapchains");Shutdown();failed=false;return;
    }
    if(!afr) {
        afrHandle=OpenFileMappingW(FILE_MAP_READ,FALSE,FLATVR_AFR_EYE_NAME);
        if(afrHandle)afr=static_cast<FlatVRAfrEye*>(MapViewOfFile(afrHandle,FILE_MAP_READ,0,0,sizeof(FlatVRAfrEye)));
    }
    const bool stereo=afr&&afr->version==FLATVR_AFR_EYE_VERSION&&afr->enabled&&!(wantsPanel&&wantsPanel());
    if(!Begin()||!render||!valid) {if(!stereo)EndPair();return;}
    const uint32_t eye=stereo?((afr->eye^afr->swap)&1u):0u;
    if(!Copy(src,eyes[eye],rq)){failed=true;EndPair();ClearInput();return;}
    if(!stereo) {
        if(!menuPlaced) {
            const auto pos=V(head.position)+math::Rotate(Q(head.orientation),{0,0,-1.6f});
            menuPose=head;menuPose.position={pos.x,pos.y,pos.z};menuPlaced=true;
        }
        EndPair();
    } else if(!pairDrawing&&eyes[0].copied&&eyes[1].copied) {
        EndPair();
    } else if(eye==1&&!eyes[0].copied) {
        // Missed left eye: submit zero layers, never present an old/mono image as stereo.
        EndPair();
    }
}
void Prepare(reshade::api::command_queue* rq,reshade::api::swapchain* swap,const reshade::api::rect*,const reshade::api::rect*,uint32_t,const reshade::api::rect*) {
    static bool once=false;if(!once){once=true;Say("Frame OpenXR: first game Present callback");}
    std::lock_guard lock(mutex);
    if(!failed&&!session&&GetTickCount64()>=retryAt&&!Initialize(rq,swap)){
        Shutdown();retryAt=GetTickCount64()+5000;
        Say("Frame OpenXR: initialization deferred; retrying in 5 seconds");
    }
}
void AfterEffects(reshade::api::effect_runtime* rt) {
    // ReShade's 'present' event is BEFORE its effects. Capture at its later
    // 'reshade_present' event so the sky/fog/HUD edits are included.
    static bool once=false;if(!once){once=true;Say("Frame OpenXR: first effects callback");}
    std::lock_guard lock(mutex);
    gameWindow=static_cast<HWND>(rt->get_hwnd());
    if(gameSwapchain&&rt->get_current_back_buffer().handle==gameSwapchain->get_current_back_buffer().handle) {
        if(capturePair&&pairDrawing&&afr&&afr->enabled&&!panelMode) {
            const unsigned eye=(afr->eye^afr->swap)&1u;
            if(!(captureEyes&(1u<<eye))) {
                char name[80];std::snprintf(name,sizeof name,"Frame-pair-%u-%s",captureSerial,eye?"right":"left");
                rt->save_screenshot(name);captureEyes|=1u<<eye;
                char line[240];const auto& v=views[eye];
                std::snprintf(line,sizeof line,"Frame capture: %s; pose %.4f %.4f %.4f; FOV %.5f %.5f %.5f %.5f",name,v.pose.position.x,v.pose.position.y,v.pose.position.z,v.fov.angleLeft,v.fov.angleRight,v.fov.angleUp,v.fov.angleDown);Say(line);
            }
        }
        Capture(rt->get_command_queue(),gameSwapchain);
    }
}
void DestroySwap(reshade::api::swapchain* swap,bool) {
    std::lock_guard lock(mutex);if(swap==gameSwapchain){Shutdown();failed=false;}
}
} // namespace

void Register(LogFn log,PanelFn panel,PanelFn world,PanelFn toggle) {
    std::lock_guard lock(mutex);logger=log;wantsPanel=panel;inWorld=world;toggleMenus=toggle;if(registered)return;
    reshade::register_event<reshade::addon_event::present>(&Prepare);
    reshade::register_event<reshade::addon_event::reshade_present>(&AfterEffects);
    reshade::register_event<reshade::addon_event::reshade_begin_effects>(&ProfileEffects);
    reshade::register_event<reshade::addon_event::destroy_swapchain>(&DestroySwap);
    reshade::register_event<reshade::addon_event::create_pipeline>(&PipelineCreating);
    reshade::register_event<reshade::addon_event::init_pipeline>(&PipelineCreated);
    foveation::Register(log);
    registered=true;Say("Frame OpenXR: registered D3D12 capture; awaiting game Present");
}
bool Active() {std::lock_guard lock(mutex);return frame&&valid&&render&&!failed&&!panelMode;}
void ConfigureDeferredWait(bool on) {
    if(deferredWait.exchange(on)!=on)Say(on?"Frame OpenXR: GPU wait deferred until copy allocator reuse":"Frame OpenXR: conservative GPU wait before image release");
}
void ConfigureSymmetricProjection(bool on) {symmetricProjection.store(on);}
void BeginEye(int eye) {
    std::lock_guard lock(mutex);
    d2rcam::SetStereoUiEye(Active()?(eye&1):-1);
    d2rcam::TraceStereoLabels(capturePair?eye:-1);
    if(!Active()){foveation::EndWorld();return;}
    profile::BeginEye(eye&1);
    StartGpuEye(eye&1);
    const auto& f=views[eye&1].fov;
    const float l=std::tan(f.angleLeft),r=std::tan(f.angleRight),d=std::tan(f.angleDown),u=std::tan(f.angleUp);
    foveation::BeginEye(eye&1,-l/(r-l),u/(u-d));
}
void ResetHeight() {
    std::lock_guard lock(mutex);
    if(!valid||!haveOrigin)return;
    origin=math::ResetHeight(origin,V(head.position));
    Say("Frame OpenXR: eye height reset to current headset position");
}
void Recenter() {
    std::lock_guard lock(mutex);
    if(!valid)return;
    origin=V(head.position);haveOrigin=true;
    if(panelMode)menuPlaced=false;
    Say("Frame OpenXR: headset position recentered");
}
bool TakeWorldRecenter() {
    std::lock_guard lock(mutex);const bool take=worldRecenter;worldRecenter=false;return take;
}
void RequestStereoCapture(){captureRequested.store(true);}
void BeginPair() {
    std::lock_guard lock(mutex);
    profile::SkipPair();lastTiming={};
    if(session&&!failed){
        if(frame)EndPair();Begin();pairDrawing=true;
        gpuProfile.started=gpuProfile.effects=gpuProfile.resolved=0;
        if(Active())profile::BeginPair();
        if(Active()&&(captureRequested.exchange(false)||d2rcam::TakeLabelCapture())) {
            capturePair=true;captureEyes=0;++captureSerial;
            Say("Frame capture: recording one stereo pair (readback pause is diagnostic, exclude from performance comparison)");
        }
    }
}
void EndPair() {
    foveation::EndWorld();
    d2rcam::SetStereoUiEye(-1);
    std::lock_guard lock(mutex);pairDrawing=false;if(!frame||!session)return;
    d2rcam::TraceStereoLabels(-1);
    if(capturePair){Say(captureEyes==3?"Frame capture: both eye screenshots requested":"Frame capture: incomplete pair; do not use for stereo comparison");capturePair=false;}
    // All image writes and state restoration have been submitted on the queue
    // bound to xrCreateSession. OpenXR orders its work after those commands;
    // it does not require a CPU completion wait before xrReleaseSwapchainImage.
    // WineOpenXR also drains that queue before its release/layout commands.
    // Keep the old path for A/B, and for synchronous diagnostic query readback.
    const bool waitNow=!deferredWait.load()||gpuProfile.started!=0;
    if(waitNow&&fence&&!WaitGpu()){failed=true;for(auto& e:eyes)e.copied=false;}
    if(!failed)ReadGpuProfile();
    gpuProfile.started=gpuProfile.effects=gpuProfile.resolved=0;
    const double releaseStart=Milliseconds();
    for(auto& e:eyes)if(e.acquired){
        XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        if(!Check(pReleaseSwapchainImage(e.chain,&ri),"release copied eye")){failed=true;e.copied=false;}
        e.acquired=false;
    }
    timing.releaseMs=Milliseconds()-releaseStart;
    XrCompositionLayerProjection projection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::array<XrCompositionLayerProjectionView,2> pv{{{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}}};
    XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
    const XrCompositionLayerBaseHeader* layer=nullptr;
    if(render&&valid&&!panelMode&&eyes[0].copied&&eyes[1].copied) {
        for(int i=0;i<2;++i){pv[i].pose=views[i].pose;pv[i].fov=views[i].fov;pv[i].subImage.swapchain=eyes[i].chain;pv[i].subImage.imageRect={{0,0},{int32_t(width),int32_t(height)}};}
        projection.space=space;projection.viewCount=2;projection.views=pv.data();layer=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projection);menuPlaced=false;
    } else if(render&&valid&&eyes[0].copied&&menuPlaced) {
        quad.space=space;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;quad.pose=menuPose;quad.size={panelWidth,panelWidth*height/width};
        quad.subImage.swapchain=eyes[0].chain;quad.subImage.imageRect={{0,0},{int32_t(width),int32_t(height)}};layer=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quad);
    }
    XrFrameEndInfo ei{XR_TYPE_FRAME_END_INFO};ei.displayTime=frameState.predictedDisplayTime;ei.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    std::array<const XrCompositionLayerBaseHeader*,3> layers{};uint32_t count=0;
    if(layer)layers[count++]=layer;
    XrCompositionLayerQuad laser{XR_TYPE_COMPOSITION_LAYER_QUAD},dot{XR_TYPE_COMPOSITION_LAYER_QUAD};
    if(layer&&panelMode&&pointerChain&&pointerVisible) {
        laser.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT|XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
        laser.space=space;laser.eyeVisibility=XR_EYE_VISIBILITY_BOTH;laser.pose=beamPose;laser.size={.003f,beamLength};
        laser.subImage.swapchain=pointerChain;laser.subImage.imageRect={{0,0},{64,32}};
        layers[count++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&laser);
        if(pointerHit) {
            dot=laser;dot.pose=dotPose;dot.size={.018f,.018f};dot.subImage.imageRect={{0,32},{64,64}};
            layers[count++]=reinterpret_cast<const XrCompositionLayerBaseHeader*>(&dot);
        }
    }
    ei.layerCount=count;ei.layers=count?layers.data():nullptr;
    const double start=Milliseconds();Check(pEndFrame(session,&ei),"end frame");
    timing.submitMs=Milliseconds()-start;
    timing.pipelines=pipelineCreates.exchange(0);timing.maxPipelineMs=maxPipelineMs.exchange(0);
    lastTiming=timing;frame=false;
}
FrameTiming LastTiming() {std::lock_guard lock(mutex);return lastTiming;}
bool View(float out[16],const float hero[3],float eyeHeight,float userEyeHeight,float bodyHeading,float recenterYaw,int eye) {
    std::lock_guard lock(mutex);if(!Active())return false;
    if(std::fabs(recenterYaw-lastRecenter)>.001f){origin=V(head.position);lastRecenter=recenterYaw;}
    const float upm=eyeHeight/std::max(userEyeHeight,.5f);
    const auto base=math::Yaw(bodyHeading-recenterYaw);
    const auto& pose=eye<0?head:views[eye&1].pose;
    const auto offset=math::Rotate(base,V(pose.position)-origin)*upm;
    const math::Vec position={hero[0]+offset.x,hero[1]+eyeHeight+offset.y,hero[2]+offset.z};
    math::View(position,math::Multiply(base,Q(pose.orientation)),out);return true;
}
bool Projection(float out[16],float nearWorld,int eye) {
    std::lock_guard lock(mutex);if(!Active())return false;
    const auto& f=views[eye&1].fov;
    math::Projection(f.angleLeft,f.angleRight,f.angleDown,f.angleUp,nearWorld,out);return true;
}
void Shutdown() {
    std::lock_guard lock(mutex);
    EndPair();ClearInput();
    // Completion is mandatory before destroying our commands/resources, even
    // when ordinary frame submission uses queue ordering without a CPU wait.
    if(fence&&!WaitGpu())failed=true;
    if(pointerChain&&pDestroySwapchain)pDestroySwapchain(pointerChain);pointerChain=XR_NULL_HANDLE;
    if(aimSpace&&pDestroySpace)pDestroySpace(aimSpace);aimSpace=XR_NULL_HANDLE;
    for(auto& e:eyes){if(e.chain&&pDestroySwapchain)pDestroySwapchain(e.chain);e={};}
    for(auto& s:handSpaces){if(s&&pDestroySpace)pDestroySpace(s);s=XR_NULL_HANDLE;}
    if(headSpace&&pDestroySpace)pDestroySpace(headSpace);if(space&&pDestroySpace)pDestroySpace(space);
    if(session&&pDestroySession)pDestroySession(session);if(actionSet&&pDestroyActionSet)pDestroyActionSet(actionSet);
    if(instance&&pDestroyInstance)pDestroyInstance(instance);
    session=XR_NULL_HANDLE;instance=XR_NULL_HANDLE;actionSet=XR_NULL_HANDLE;headSpace=space=XR_NULL_HANDLE;
    running=frame=valid=render=haveOrigin=menuPlaced=panelMode=false;state=XR_SESSION_STATE_UNKNOWN;gameWindow=nullptr;
    worldWasLoaded=worldCalibrationPending=worldRecenter=false;
    if(fenceEvent)CloseHandle(fenceEvent);fenceEvent=nullptr;
    fence.Reset();queue.Reset();device.Reset();fenceValue=0;
    gpuProfile={};
    foveation::Shutdown();
    tracking.Close();pad.Close();actions.Close();
    if(afr)UnmapViewOfFile(afr);if(afrHandle)CloseHandle(afrHandle);afr=nullptr;afrHandle=nullptr;gameSwapchain=nullptr;
    if(loader)FreeLibrary(loader);loader=nullptr;
}
} // namespace framevr
