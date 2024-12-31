// Based on code from Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/DXSample.cpp

#include "DXSample.h"
#include "Win32Application.h"

using namespace Microsoft::WRL;

DXSample::DXSample(UINT width, UINT height, std::wstring name) :
	mWidth(width),
	mHeight(height),
		mTitle(name),
		mUseWarpDevice(false)
	{
		WCHAR assetsPath[512];
		GetAssetsPath(assetsPath, _countof(assetsPath));
		mAssetsPath = assetsPath;

		mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
	}

	DXSample::~DXSample()
	{
	}

	// Helper function to get full path of assets
	std::wstring DXSample::GetAssetFullPath(LPCWSTR assetName)
	{
		return mAssetsPath + assetName;
	}

	// Helper function to find the first available hardware adapter that supports Direct3D 12.
	// If none are found then *ppAdapter is set to nullptr.
	_Use_decl_annotations_	// Makes sure the same annotation are used as in the header
							// e.g. _In_
		void GetHardwareAdapter(
			IDXGIFactory1* pFactory,
			IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;

		ComPtr<IDXGIAdapter1> adapter;

		ComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (UINT adapterIndex = 0;
				SUCCEEDED(factory6->EnumAdapterByGpuPreference(
					adapterIndex,
					requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
					IID_PPV_ARGS(&adapter)));
					adapterIndex++)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				// Prevents selecting the Basic Render Drive adapter
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				// Check the adapter supports Direct3D 12
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		if (adapter.Get() == nullptr)
		{
			for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); adapterIndex++)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				// Prevents selecting the Basic Render Drive adapter
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				// Check the adapter supports Direct3D 12
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		*ppAdapter = adapter.Detach();
	}

	// Helper function for setting the window's title text
	void DXSample::SetCustomWindowText(LPCWSTR text)
	{
		std::wstring windowText = mTitle + L": " + text;
		SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
	}

	// Helper function for parsing command line args
	_Use_decl_annotations_
		void DXSample::ParseCommandLineArgs(WCHAR* argv[], int argc)
	{
		for (int i = 0; i < argc; i++)
		{
			if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
				_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
			{
				mUseWarpDevice = true;
				mTitle = mTitle + L" (WARP)";
			}
		}
	}
}
