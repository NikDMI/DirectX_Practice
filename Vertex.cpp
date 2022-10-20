#include "Vertex.h"
#include "DX12.h"

//MOVE THIS MODULE INTO BUFFER.CPP
namespace MemoryNS {
	D3D12_HEAP_PROPERTIES InitHeapProperties(D3D12_HEAP_TYPE type) {
		D3D12_HEAP_PROPERTIES heapProp;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProp.Type = type;
		return heapProp;
	}

	D3D12_RESOURCE_DESC CreateBufferResourceDesc(UINT bufferSize) {
		D3D12_RESOURCE_DESC resDesc;
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Alignment = 0;
		resDesc.Width = bufferSize;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc = { 1,0 };
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		return resDesc;
	}


	void CopyDataToUploadBuffer(ID3D12Resource* uploadBuffer, const void* data, UINT size) {
		void* mapData;
		uploadBuffer->Map(0, nullptr, &mapData);
		data = memcpy(mapData, data, size);
		uploadBuffer->Unmap(0, nullptr);
		//uploadBuffer->WriteToSubresource(0, NULL, data, size, size);
	}


	ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device,ID3D12GraphicsCommandList* commandList,UINT bufferSize, UINT numberOfElements,const void* data,D3D12_VERTEX_BUFFER_VIEW* outputVertexView) {
		ComPtr<ID3D12Resource> defaultResource;
		static ComPtr<ID3D12Resource> uploadResource;

		D3D12_HEAP_PROPERTIES heapProp = InitHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_DESC resDesc = CreateBufferResourceDesc(bufferSize);
		device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
			&resDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultResource.GetAddressOf()));

		heapProp = InitHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
			&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadResource.GetAddressOf()));

		CopyDataToUploadBuffer(uploadResource.Get(), data, bufferSize);
		static D3D12_TEXTURE_COPY_LOCATION destCopyLocation;///////////ÏÎÊÀ ÑÒÀÒÈ×ÅÑÊÈÅ, ÒÀÊ ÊÀÊ ÎÄÈÍ ÎÁÚÅÊÒ
		destCopyLocation.pResource = defaultResource.Get();
		destCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destCopyLocation.PlacedFootprint.Offset = 0;
		destCopyLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8_UINT;
		destCopyLocation.PlacedFootprint.Footprint.Width = bufferSize;
		destCopyLocation.PlacedFootprint.Footprint.Height = 1;
		destCopyLocation.PlacedFootprint.Footprint.RowPitch = ceil(bufferSize/256.0)*256;
		destCopyLocation.PlacedFootprint.Footprint.Depth = 1;
		static D3D12_TEXTURE_COPY_LOCATION srcCopyLocation = destCopyLocation;
		srcCopyLocation = destCopyLocation;
		srcCopyLocation.pResource = uploadResource.Get();
		static D3D12_BOX box{};
		box.left = 0; box.right = bufferSize;
		box.top = 0; box.bottom = 1;
		box.front = 0; box.back = 1;
		D3D12_NS::TranslateResourceBarrier(commandList, defaultResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		//D3D12_NS::TranslateResourceBarrier(commandList, uploadResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, &box);
		D3D12_NS::TranslateResourceBarrier(commandList, defaultResource.Get(),D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		//ID3D12Resource::
		//GIVE POINTER TO THE UPLOAD BUFF TO DELETE, WHEN GPU PERFORMS COPY OPERATION!!!
		if (outputVertexView != nullptr) {
			outputVertexView->BufferLocation = defaultResource->GetGPUVirtualAddress();
			outputVertexView->SizeInBytes = bufferSize;
			outputVertexView->StrideInBytes = bufferSize / numberOfElements;
		}
		return defaultResource;
		
	}
}