#pragma once
#include "Config.h"
#include "DX12.h"


namespace MemoryNS {
	//Create buffer in HEAP_DEFAULT throught HEAP_UPDATE 
	ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT bufferSize, UINT numberOfElements, const void* data, D3D12_VERTEX_BUFFER_VIEW* outputVertexView);
}