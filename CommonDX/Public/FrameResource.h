//// Frame Resources - holds the resources needed for the CPU to build command lists
//// for a frame
//// based on Frank Luna DX12 2ed p. 293
//
//#pragma once
//
//#include <d3d12.h>
//#include <wrl/client.h>
//#include <memory>
//
//#include <CommonDX/Public/UploadBuffer.h>
//
//struct FrameResource
//{
//public:
//	FrameResource(ID3D12Device* device, UINT passCount) {};
//	FrameResource(const FrameResource&) = delete;
//	FrameResource& operator=(const FrameResource&) = delete;
//	~FrameResource() {};
//
//	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
//
//	//std::unique_ptr<UploadBuffer<PassConstants>>
//};
