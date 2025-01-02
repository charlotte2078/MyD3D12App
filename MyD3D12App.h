// Based on code by Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.h

#pragma once

#include "DXSample.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class MyD3D12App : public DXSample
{
public:
	MyD3D12App(UINT width, UINT height, std::wstring name);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

private:
	// The number of buffers in the swap chain
	static const UINT FrameCount = 2;

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 colour;
	};

	// Pipeline objects
	CD3DX12_VIEWPORT mViewport;
	CD3DX12_RECT mScissorRect;
	ComPtr<IDXGISwapChain3> mSwapChain;
	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12Resource> mRenderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap; // RTV = Render Target View
	ComPtr<ID3D12PipelineState> mPipelineState;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	UINT mRtvDescriptorSize;

	// App resources
	ComPtr<ID3D12Resource> mVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

	// Synchronisation objects
	UINT mFrameIndex;
	HANDLE mFenceEvent;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mFenceValue;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
};

