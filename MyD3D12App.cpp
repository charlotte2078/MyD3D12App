#include "MyD3D12App.h"

MyD3D12App::MyD3D12App(UINT width, UINT height, std::wstring name) :
	DXSample(width, height, name),
	mFrameIndex(0),
	mViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	mScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	mRtvDescriptorSize(0)
{
}

void MyD3D12App::OnInit()
{
	LoadPipeline();
	LoadAssets();
}


void MyD3D12App::LoadPipeline()
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
	ComPtr<IDXGIFactory4> factory;
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

	// Describe and create the command queue
	// The command queue holds commands the GPU will execute, which are submitted by the CPU
	// using command lists
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // Default commnad queue (GPU Timeout enabled)
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // A command buffer that the GPU can execute

	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// Describe and create the swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount; // The number of buffers in the swap chain
	swapChainDesc.Width = mWidth; // resolution width
	swapChainDesc.Height = mHeight; // Resolution height
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 32-bit unsigned-normalized-integer format
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
	
	// This prevents the window from responding to alt-enter (which makes the window fullscreen)
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&mSwapChain)); // Check we can use the IDXGISwapChain1 as an IDXGISwapChain3
	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex(); // Introduced in IDSGISwapChain3

	// Create desctiptor heaps
	{
		// Describe and create an RTV (render target view) descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

		mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create an RTV for each frame
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n])));
			mDevice->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, mRtvDescriptorSize);
		}
	}

	ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
}
