// Based on code by Microsoft
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.h

#pragma once

#include "DXSample.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class MyD3D12App : public DXSample
{
public:
	D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

private:
	static const UINT FrameCount = 2;

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 colour;
	};

	// Pipeline objects
	CD3DX12_VIEWPORT mViewport;
};

