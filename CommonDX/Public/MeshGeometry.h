// Struct that holds Vertex and Index buffers for a bit of geometry
// Inspired by Frank Luna DirectX12 2ed pp. 268-269

#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct MeshGeometry
{
	ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBufferGPU = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
};
