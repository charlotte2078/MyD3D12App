// Helper to handle the heap for constant buffer, shader resource and unorded access descriptors
// Frank Luna DirectX12 2ed pp. 249-250

#include "DescriptorHeap.h"

#include <cstdint>
#include <queue>

#pragma once

class CbvSrvUavHeap : public DescriptorHeap
{
public:
	CbvSrvUavHeap(const DescriptorHeap&) = delete;
	CbvSrvUavHeap& operator=(const CbvSrvUavHeap&) = delete;

	static CbvSrvUavHeap& Get()
	{
		static CbvSrvUavHeap singleton;
		return singleton;
	}

	bool IsInitialised() const;

	void Init(ID3D12Device* device, UINT capacity);

	uint32_t NextFreeIndex();
	void ReleaseIndex(uint32_t index);

private:
	CbvSrvUavHeap() = default;

	bool mIsInitialised = false;

	std::queue<uint32_t> mFreeIndices;
};
