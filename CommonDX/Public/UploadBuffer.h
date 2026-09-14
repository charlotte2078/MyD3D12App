// Wrapper to help with upload buffers
// Frank Luna DX12 2ed pp. 244-246

#pragma once

#include <wrl/client.h>
#include <Windows.h>

#include <CommonDX/Public/DXSampleHelper.h>

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = isConstantBuffer ? DXHelpers::CalculateConstantBufferByteSize(sizeof(T)) : sizeof(T);

		{
			auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount);

			DXHelpers::ThrowIfFailed(device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&mUploadBuffer)
			));
		}

		DXHelpers::ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
	};

	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator= (const UploadBuffer&) = delete;

	~UploadBuffer()
	{
		if (mUploadBuffer)
		{
			mUploadBuffer->Unmap(0, nullptr);
		}

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource() const
	{
		return mUploadBuffer.Get();
	}

	void CopyData(const int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};
