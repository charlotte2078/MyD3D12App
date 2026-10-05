// Frame Resources - holds the resources needed for the CPU to build command lists
// for a frame
// based on Frank Luna DX12 2ed p. 293

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

#include <CommonDX/Public/UploadBuffer.h>

template<typename TPassConstants>
struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount)
	{
		ThrowIfFailed(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(cmdListAlloc.GetAddressOf())
		));

		passCB = std::make_unique<UploadBuffer<TPassConstants>>(device, passCount, true);
	}
	
	FrameResource(const FrameResource&) = delete;
	FrameResource& operator=(const FrameResource&) = delete;
	~FrameResource() {};

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdListAlloc;

	std::unique_ptr<UploadBuffer<TPassConstants>> passCB;

	UINT fence = 0;
};
