// Based on code from Microsoft.
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloWindow/DXSample.h

#pragma once

#include "DXSampleHelper.h"
#include "Win32Application.h"

// Abstract class that holds base functionality for DX12 Apps.
class DXSample
{
public:
	// Constructor
	DXSample(UINT width, UINT height, std::wstring name);

	// Prohibit copying
	DXSample(const DXSample& rhs) = delete;
	DXSample& operator=(const DXSample& rhs) = delete;

	// Virtual destructor - needed so this will call correctly
	virtual ~DXSample();

	virtual void OnInit() = 0;
	virtual void OnUpdate(const float deltaTime) = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	// Getters
	UINT GetWidth() const { return mWidth; }
	UINT GetHeight() const { return mHeight; }
	const WCHAR* GetTitle() const { return mTitle.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

protected:
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter(
		_In_ IDXGIFactory1* pFactory,
		_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	void SetCustomWindowText(LPCWSTR text);

	// Viewport dimensions
	UINT mWidth;
	UINT mHeight;
	float mAspectRatio;

	// Adapter info
	bool mUseWarpDevice;

private:
	// Root assets path
	std::wstring mAssetsPath;

	// Window title
	std::wstring mTitle;
};

