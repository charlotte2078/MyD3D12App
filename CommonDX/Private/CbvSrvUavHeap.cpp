#include <CommonDX/Public/CbvSrvUavHeap.h>

#include <cassert>

bool CbvSrvUavHeap::IsInitialised() const
{
	return mIsInitialised;
}

void CbvSrvUavHeap::Init(ID3D12Device* device, UINT capacity)
{
	DescriptorHeap::Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, capacity);

	for (UINT i = 0; i < capacity; ++i)
	{
		mFreeIndices.push(i);
	}

	mIsInitialised = true;
}

uint32_t CbvSrvUavHeap::NextFreeIndex()
{
	assert(!mFreeIndices.empty());

	const uint32_t index = mFreeIndices.front();
	mFreeIndices.pop();

	return index;
}

void CbvSrvUavHeap::ReleaseIndex(uint32_t index)
{
	mFreeIndices.push(index);
}
