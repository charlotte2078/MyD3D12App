// Helper class for creating and managing heaps
// Frank Luna DirectX 12 2ed pp. 130-131

#pragma once

#include <cstdint>
#include <d3d12.h>
#include <d3dx12_root_signature.h>
#include <wrl/client.h>

class DescriptorHeap
{
public:
	DescriptorHeap() = default;
	DescriptorHeap(const DescriptorHeap&) = delete;
	DescriptorHeap& operator=(const DescriptorHeap&) = delete;

	void Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity);

	ID3D12DescriptorHeap* GetD3dHeap() const;

	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle(uint32_t index);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle(uint32_t index);

protected:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap = nullptr;
	UINT mDescriptorSize = 0;
};
