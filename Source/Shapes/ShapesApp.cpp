#include "ShapesApp.h"

#include <CommonDX/Public/CbvSrvUavHeap.h>
#include <CommonDX/Public/Win32Application.h>

#include <array>
#include <sstream>
#include <BufferHelpers.h>
#include <DirectXColors.h>

constexpr UINT CBV_SRV_UAV_HEAP_CAPACITY = 16384;

ShapesApp::ShapesApp(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	mFrameIndex(0),
	mViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	mScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	mFenceValue(0)
{
}

ShapesApp::~ShapesApp()
{
}

void ShapesApp::OnInit()
{
	InitD3D();
	
	mUploadBatch = std::make_unique<DirectX::ResourceUploadBatch>(mDevice.Get());

	OnResize();

	mUploadBatch->Begin();

	CreateVertexAndIndexBuffers();

	std::future<void> result = mUploadBatch->End(mCommandQueue.Get());
	
	CreateConstantBuffers();
	CreateRootSignature();
	CreateFrameResources();
	CreatePSO();

	result.wait();
}

void ShapesApp::InitD3D()
{
	UINT dxgiFactoryFlags = 0;

	// Creates a debug controller if the Debug layer is active
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	// Create the DXGI factory - which enables creating DXGI objects (e.g. swap chain)
	ComPtr<IDXGIFactory6> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (mUseWarpDevice)
	{
		// The WARP (Windows Advanced Rasterizer Platform) device is software adapter
		// It can be used instead of hardware if required
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)));

	}
	else // Use hardware adapter
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),	// The display adapter the device will represent
			D3D_FEATURE_LEVEL_11_0, // The min. feature level the app needs support for
			IID_PPV_ARGS(&mDevice))); // COM ID of the Device to create and the pDevice
	}

	CreateCommandObjects();
	CreateSwapChain(factory.Get());
	CreateDescriptorHeaps();

	{
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		DXHelpers::SetName(mFence.Get(), L"Fence");
	}

	// This prevents the window from responding to alt-enter (which makes the window fullscreen)
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));
}

// Update frame based values
void ShapesApp::OnUpdate(const float deltaTime)
{
	if (mAppPaused)
	{
		return;
	}

	// Cycle the frame resource
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->fence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// Update the world, view, and projection matrices
	XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f); // TODO: add a moveable camera
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = view * proj;

	XMStoreFloat4x4(&mMainPassConstants.ViewProj, XMMatrixTranspose(viewProj));

	auto currPassCB = mCurrFrameResource->passCB.get();
	currPassCB->CopyData(0, mMainPassConstants);

	//// Update the per-pass buffer
	//PassConstants passConstants;
	//XMStoreFloat4x4(&passConstants.ViewProj, XMMatrixTranspose(viewProj));
	//mPassCB->CopyData(0, passConstants);

	// Make the cube rotate over time
	static float pitch, yaw, roll = 0.0f;
	constexpr float smallNumber = 0.001f;
	pitch += smallNumber * deltaTime;
	yaw += smallNumber * deltaTime * 2;
	roll += smallNumber * deltaTime * 3;

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	world = world * rotation;

	// Update the per-object buffer
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
	mObjectCB->CopyData(0, objConstants);
}

// Render the scene
void ShapesApp::OnRender()
{
	if (mAppPaused)
	{
		return;
	}

	auto cmdListAllocator = mCurrFrameResource->cmdListAlloc;
	ThrowIfFailed(cmdListAllocator->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdListAllocator.Get(), mPipelineState.Get()));

	CbvSrvUavHeap& cbvSrvUavHeap = CbvSrvUavHeap::Get();
	ID3D12DescriptorHeap* descriptorHeaps[] = { cbvSrvUavHeap.GetD3dHeap() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	
	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	CD3DX12_RESOURCE_BARRIER rbTransitionPresentRT = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &rbTransitionPresentRT);

	auto rtvHandle = mRtvHeap.CpuHandle(mFrameIndex);
	auto dsvHandle = mDsvHeap.CpuHandle(0);
	const float clearColour[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	mCommandList->ClearRenderTargetView(rtvHandle, clearColour, 0, nullptr);
	mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	mCommandList->SetPipelineState(mPipelineState.Get());
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->SetGraphicsRootDescriptorTable(ROOT_ARG_OBJECT_CBV, cbvSrvUavHeap.GpuHandle(mBoxCBHeapIndex));

	ID3D12Resource* passCB = mCurrFrameResource->passCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(ROOT_ARG_PASS_CBV, passCB->GetGPUVirtualAddress());

	mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	mCommandList->IASetIndexBuffer(&mIndexBufferView);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	mCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);

	CD3DX12_RESOURCE_BARRIER rbTransitionRTPresent = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &rbTransitionRTPresent);

	ThrowIfFailed(mCommandList->Close());

	// Execute the command list
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mFrameIndex = (mFrameIndex + 1) % gFrameCount;


	mCurrFrameResource->fence = ++mFenceValue;
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
}

void ShapesApp::OnResize()
{
	assert(mDevice);
	assert(mSwapChain);
	assert(mCommandAllocator);

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < gFrameCount; ++i)
	{
		mRenderTargets[i].Reset();
	}

	mDepthStencilBuffer.Reset();

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		gFrameCount,
		mWidth, mHeight,
		mkSwapChainFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mFrameIndex = 0;

	CreateRTVsForSwapChain();
	CreateDepthStencilBuffer();

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = static_cast<float>(mWidth);
	mViewport.Height = static_cast<float>(mHeight);

	mScissorRect.left = 0;
	mScissorRect.top = 0;
	mScissorRect.right = mWidth;
	mScissorRect.bottom = mHeight;

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, static_cast<float>(mWidth)/mHeight, 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void ShapesApp::OnDestroy()
{
	FlushCommandQueue();
}

void ShapesApp::FlushCommandQueue()
{
	++mFenceValue;

	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void ShapesApp::CreateCommandObjects()
{
	// Describe and create the command queue
	// The command queue holds commands the GPU will execute, which are submitted by the CPU
	// using command lists
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // Default commnad queue (GPU Timeout enabled)
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // A command buffer that the GPU can execute

	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
	DXHelpers::SetName(mCommandQueue.Get(), L"Command Queue");

	ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
	DXHelpers::SetName(mCommandAllocator.Get(), L"Command Allocator");

	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
	DXHelpers::SetName(mCommandList.Get(), L"Command List");

	ThrowIfFailed(mCommandList->Close());
}

void ShapesApp::CreateSwapChain(IDXGIFactory6* factory)
{
	assert(factory);

	// Describe and create the swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = gFrameCount; // The number of buffers in the swap chain
	swapChainDesc.Width = mWidth; // resolution width
	swapChainDesc.Height = mHeight; // Resolution height
	swapChainDesc.Format = mkSwapChainFormat; 
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // Rendering to the back buffer
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // discard pixels after presenting
	swapChainDesc.SampleDesc.Count = 1; // The number of multisamples (single sampling here)

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		mCommandQueue.Get(), // Pointer to the command queue
		Win32Application::GetHwnd(), // Window Handler
		&swapChainDesc, // Pointer to the swap chain description
		nullptr, // Pointer to the full screen window swap chain description
		nullptr, // Pointer to the IDXGIOutput interface to restrict content to
		&swapChain)); // Output pp for the swap chain

	ThrowIfFailed(swapChain.As(&mSwapChain)); // Check we can use the IDXGISwapChain1 as an IDXGISwapChain3
}

// Create desctiptor heaps
void ShapesApp::CreateDescriptorHeaps()
{
	mRtvHeap.Init(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, gFrameCount);
	mDsvHeap.Init(mDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
}

void ShapesApp::CreateDepthStencilBuffer()
{
	{
		// Create the DSV
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = mWidth;
		depthStencilDesc.Height = mHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = mkDepthStencilFormat;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optimisedClearValue;
		optimisedClearValue.Format = mkDepthStencilFormat;
		optimisedClearValue.DepthStencil.Depth = 1.0f;
		optimisedClearValue.DepthStencil.Stencil = 0;

		auto heapProps = D3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		ThrowIfFailed(mDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optimisedClearValue,
			IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
		));
		
		DXHelpers::SetName(mDepthStencilBuffer.Get(), L"Depth Stencil Buffer");
	}

	mDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(),
		nullptr,
		mDsvHeap.CpuHandle(0)
	);

	auto rbDepthCommonDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
		mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_DEPTH_WRITE);

	mCommandList->ResourceBarrier(1, &rbDepthCommonDepthWrite);
}

void ShapesApp::CreateRTVsForSwapChain()
{
	// Create an RTV for each frame
	for (UINT i = 0; i < gFrameCount; ++i)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
		mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, mRtvHeap.CpuHandle(i));

#if defined(DEBUG) || defined(_DEBUG)
		// UINT to LPCWSTR conversion https://stackoverflow.com/a/8626503
		std::wstringstream wss;
		std::wstring wstr;

		wss << L"Render target ";
		wss << i;

		wss >> wstr;

		DXHelpers::SetName(mRenderTargets[i].Get(), wstr.c_str());
#endif
	}
}

// Create root signature
// A root signature defines what types of resources are bound to the graphics pipeline
void ShapesApp::CreateRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[ROOT_ARG_COUNT] = {};

	// Table for per-object constants
	{
		CD3DX12_DESCRIPTOR_RANGE objectCbvTable;
	
		constexpr UINT numDescriptors = 1;
		constexpr UINT baseRegister = 0;

		objectCbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numDescriptors, baseRegister);

		slotRootParameter[ROOT_ARG_OBJECT_CBV].InitAsDescriptorTable(1, &objectCbvTable);
	}

	// CBV for per-pass constants
	{
		constexpr UINT baseRegister = 1;

		slotRootParameter[ROOT_ARG_PASS_CBV].InitAsConstantBufferView(baseRegister);
	}
	
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(
		ROOT_ARG_COUNT, // Num parameters
		slotRootParameter, // Ptr to root parameter
		0, // Num static samplers
		nullptr, // Pointer to static samplers desc
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT); // Flags - this one opts the app into using the input assembler

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

// Create the pipeline state (compile and load shaders)
void ShapesApp::CreatePSO()
{
#if defined(DEBUG) || defined(_DEBUG)  
#define COMMA_DEBUG_ARGS ,DXC_ARG_DEBUG, DXC_ARG_SKIP_OPTIMIZATIONS
#else
#define COMMA_DEBUG_ARGS
#endif

	std::vector<LPCWSTR> vsArgs = { L"-E VSMain", L"-T vs_6_6" COMMA_DEBUG_ARGS};
	ComPtr<IDxcBlob> vertexShader = DXHelpers::CompileShader(L"Shaders\\cube_shaders.hlsl", vsArgs);
	
	std::vector<LPCWSTR> psArgs = { L"-E PSMain", L"-T ps_6_6" COMMA_DEBUG_ARGS};
	ComPtr<IDxcBlob> pixelShader = DXHelpers::CompileShader(L"Shaders\\cube_shaders.hlsl", psArgs);

	// Define the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Describe and create the graphics pipeline state object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = DXHelpers::ByteCodeFromBlob(vertexShader.Get());
	psoDesc.PS = DXHelpers::ByteCodeFromBlob(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mkSwapChainFormat;
	psoDesc.DSVFormat = mkDepthStencilFormat;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));
}

// Create the vertex buffer (also define geometry)
void ShapesApp::CreateVertexAndIndexBuffers()
{
	constexpr int numCubeVertices = 8;

	std::array<Vertex, numCubeVertices> cubeVertices =
	{
		Vertex({ -1.0f, -1.0f, -1.0f }, XMFLOAT4(Colors::AliceBlue)),
		Vertex({ -1.0f, +1.0f, -1.0f }, XMFLOAT4(Colors::Bisque)),
		Vertex({ +1.0f, +1.0f, -1.0f }, XMFLOAT4(Colors::DarkGreen)),
		Vertex({ +1.0f, -1.0f, -1.0f }, XMFLOAT4(Colors::LightPink)),
		Vertex({ -1.0f, -1.0f, +1.0f }, XMFLOAT4(Colors::Yellow)),
		Vertex({ -1.0f, +1.0f, +1.0f }, XMFLOAT4(Colors::Aqua)),
		Vertex({ +1.0f, +1.0f, +1.0f }, XMFLOAT4(Colors::Gold)),
		Vertex({ +1.0f, -1.0f, +1.0f }, XMFLOAT4(Colors::MediumPurple))
	};

	std::array<std::uint16_t, 36> cubeIndices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	CreateStaticBuffer(
		mDevice.Get(),
		*mUploadBatch,
		cubeVertices.data(),
		cubeVertices.size(),
		sizeof(Vertex),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		&mVertexBufferGPU);

	CreateStaticBuffer(
		mDevice.Get(),
		*mUploadBatch,
		cubeIndices.data(),
		cubeIndices.size(),
		sizeof(std::uint16_t),
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		&mIndexBufferGPU);

	mVertexBufferView.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = static_cast<UINT>(cubeVertices.size() * sizeof(Vertex));
	mVertexBufferView.StrideInBytes = sizeof(Vertex);

	mIndexBufferView.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	mIndexBufferView.SizeInBytes = static_cast<UINT>(cubeIndices.size() * sizeof(std::uint16_t));
}

void ShapesApp::CreateConstantBuffers()
{
	CbvSrvUavHeap& cbvSrvUavHeap = CbvSrvUavHeap::Get();

	if (!cbvSrvUavHeap.IsInitialised())
	{
		cbvSrvUavHeap.Init(mDevice.Get(), CBV_SRV_UAV_HEAP_CAPACITY);
	}

	// Object constant buffer and view
	mBoxCBHeapIndex = cbvSrvUavHeap.NextFreeIndex();
	
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(
		mDevice.Get(),
		1,
		true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC objectCBV;
	objectCBV.BufferLocation = mObjectCB->Resource()->GetGPUVirtualAddress();
	objectCBV.SizeInBytes = mObjectCB->ElementByteSize();

	mDevice->CreateConstantBufferView(&objectCBV, cbvSrvUavHeap.CpuHandle(mBoxCBHeapIndex));
}

void ShapesApp::CreateFrameResources()
{
	constexpr UINT passCount = 1;

	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<ShapesFrameResource>(mDevice.Get(), passCount));
	}
}
