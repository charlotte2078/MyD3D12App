#include <CommonDX/Public/DescriptorHeap.h>

#include <cassert>
#include <CommonDX/Public/DXSampleHelper.h>

void DescriptorHeap::Init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity)
{
	assert(mHeap == nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = capacity;
	heapDesc.Type = type;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // TODO : update this later when using for more types of heaps
	DXHelpers::ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mHeap.GetAddressOf())));

	mDescriptorSize = device->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* DescriptorHeap::GetD3dHeap() const
{
	return mHeap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CpuHandle(uint32_t index)
{
	auto hCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeap->GetCPUDescriptorHandleForHeapStart());
	hCpu.Offset(index, mDescriptorSize);

	return hCpu;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GpuHandle(uint32_t index)
{
	auto hGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(mHeap->GetGPUDescriptorHandleForHeapStart());
	hGpu.Offset(index, mDescriptorSize);

	return hGpu;
}
