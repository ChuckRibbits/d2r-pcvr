// Reproduce the Proton mapped-upload write failure on a real D3D12 resource,
// then verify the production store reaches GPU readback and preserves lane 4.
#include "../vr/dlss_depth_range.h"
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdio>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr, const char* what) {
    if (FAILED(hr)) {
        std::fprintf(stderr, "%s: 0x%08lx\n", what, (unsigned long)hr);
        throw std::runtime_error(what);
    }
}
int main() {
    try {
        ComPtr<ID3D12Device> device;
        Check(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), "device");
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = 0x600; desc.Height = desc.DepthOrArraySize = desc.MipLevels = 1;
        desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        ComPtr<ID3D12Resource> upload, readback;
        Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)), "upload");
        heap.Type = D3D12_HEAP_TYPE_READBACK;
        Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)), "readback");
        void* mapped = nullptr; D3D12_RANGE empty{0,0};
        Check(upload->Map(0, &empty, &mapped), "map upload");
        auto* range = static_cast<unsigned char*>(mapped) + 0x350;
        const float native[4] = {510, 1.f/510, 550, 123};
        std::memcpy(range, native, sizeof native);
        SIZE_T copied = 0;
        SetLastError(0);
        const BOOL wrote = WriteProcessMemory(GetCurrentProcess(), range, native, 12, &copied);
        std::printf("Mapped upload WriteProcessMemory: success=%d bytes=%zu error=%lu\n",
            int(wrote), size_t(copied), GetLastError());
        const float projection[16] = {.6f,0,0,0,0,.58f,0,0,0,0,0,-1,0,0,.2f,0};
        float corrected[4]; std::memcpy(corrected, native, sizeof native);
        if (!dlssdepth::PerspectiveRange(projection, corrected))
            throw std::runtime_error("range validation");
        dlssdepth::StoreMappedRange(range, corrected);
        D3D12_COMMAND_QUEUE_DESC qdesc{};
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> commands;
        Check(device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&queue)), "queue");
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "allocator");
        Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commands)), "commands");
        commands->CopyBufferRegion(readback.Get(), 0, upload.Get(), 0x350, sizeof native);
        Check(commands->Close(), "close");
        ID3D12CommandList* lists[] = {commands.Get()};
        queue->ExecuteCommandLists(1, lists);
        ComPtr<ID3D12Fence> fence;
        Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "fence");
        Check(queue->Signal(fence.Get(), 1), "signal");
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event) throw std::runtime_error("event");
        Check(fence->SetEventOnCompletion(1, event), "completion");
        const DWORD wait = WaitForSingleObject(event, 10000);
        CloseHandle(event);
        if (wait != WAIT_OBJECT_0) throw std::runtime_error("GPU timeout");
        void* result = nullptr; D3D12_RANGE read{0, sizeof native};
        Check(readback->Map(0, &read, &result), "map readback");
        const bool equal = std::memcmp(result, corrected, sizeof corrected) == 0;
        readback->Unmap(0, &empty); upload->Unmap(0, nullptr);
        if (!equal) throw std::runtime_error("GPU range/reserved lane mismatch");
        std::puts("VR depth: production mapped store verified by D3D12 GPU readback; reserved lane preserved");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what()); return 1;
    }
}
