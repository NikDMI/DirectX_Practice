#include "Buffer.h"
#include <fstream>
#include <d3dcompiler.h>
#include "Material.h"
#include "d3dx12.h"

#pragma comment(lib,"d3dcompiler.lib")

namespace Buffers {//Common functions

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

	/*
	D3D12_RESOURCE_DESC CreateTexture2DResourceDesc(ID3D12Resource* resource) {
		D3D12_RESOURCE_DESC resDesc = resource->;
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
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
	*/


	ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device,UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
		ID3D12DescriptorHeap* heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Type = type;
		desc.NodeMask = 1;
		desc.NumDescriptors = numDescriptors;
		desc.Flags = flags;
		device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
		return heap;
	}
	
}

namespace Buffers {//ConstanBuffer

	ConstantBuffer::ConstantBuffer(D3D12_NS::GPUAdapter* adapter, std::vector<DataInfo>& structureInfo) : bufferSize{}, adapter{ adapter }, d3d12Device{ adapter->GetDevice() }, bufferInfo{structureInfo} {
		for (auto& info : structureInfo) {
			info.memberSize = ConstantBuffer::CountPaddingStructureSize(info.memberSize);
			bufferSize += info.memberSize * info.memberCount;//can't proccess overflow
		}
		if (bufferSize > MAX_UPLOAD_HEAP_SIZE) throw std::exception("Too large buffer");
		//create buffer
		D3D12_HEAP_PROPERTIES heapProp = InitHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC resDesc = CreateBufferResourceDesc(bufferSize);
		d3d12Device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
			&resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(buffer.GetAddressOf()));
		buffer->Map(0, nullptr, &bufferCPUAddress);
		//fill buffer
		char* currentBufferAddress = (char*)bufferCPUAddress;
		UINT segmentCount = structureInfo.size();
		for (UINT i = 0; i < segmentCount;++i) {
			UINT memCount = structureInfo[i].memberCount;
			UINT memRealSize = bufferInfo[i].memberSize;
			UINT memPaddingSize = structureInfo[i].memberSize;
			char* currentDataAddress = (char*)structureInfo[i].data;
			if (currentDataAddress != nullptr) {
				for (UINT j = 0; j < memCount; ++j) {
					memcpy(currentBufferAddress, currentDataAddress, memRealSize);
					currentBufferAddress += memPaddingSize;
					currentDataAddress += memRealSize;
				}
			}
			else {
				currentBufferAddress += memPaddingSize*memCount;
			}
		}
		bufferInfo = structureInfo;
		//creating desctiptor heap
		descriptorHeap = CreateDescriptorHeap(d3d12Device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferView;
		bufferView.BufferLocation = buffer->GetGPUVirtualAddress();
		bufferView.SizeInBytes = bufferSize;
		d3d12Device->CreateConstantBufferView(&bufferView, descriptorHeap->GetCPUDescriptorHandleForHeapStart());
		this->bufferView = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}

	UINT ConstantBuffer::CountPaddingStructureSize(UINT sizeOfStructure) {
		return (sizeOfStructure + UPLOAD_HEAP_PADDING_SIZE) & ~(UPLOAD_HEAP_PADDING_SIZE-1);
	}

	void ConstantBuffer::CopyData(void* data, unsigned int size) {
		memcpy(bufferCPUAddress, data, size);
	}

	/*
	D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBuffer::GetBufferView() {
		D3D12_CONSTANT_BUFFER_VIEW_DESC bufferView;
		bufferView.BufferLocation = buffer->GetGPUVirtualAddress();
		bufferView.SizeInBytes = bufferSize;
		return bufferView;
	}
	*/

	ComPtr<ID3D12Resource> ConstantBuffer::GetBufferResource() {
		return buffer;
	}

	ID3D12DescriptorHeap* ConstantBuffer::GetDescriptorHeap() {
		return descriptorHeap.Get();
	}

	D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetGPUVirtualAddress() {
		return buffer->GetGPUVirtualAddress();
	}

	uint32_t ConstantBuffer::GetBufferSize() {
		return this->bufferSize;
	}

	ConstantBuffer::~ConstantBuffer() {
	}
}


namespace Buffers {//VertexBuffer

	VertexBuffer::VertexBuffer(D3D12_NS::GPUAdapter* adapter, uint16_t bufferSize) : adapter{adapter} {
		if (bufferSize == 0) throw Exception("Zero buffer size");
		bufferSize = (bufferSize + DEFAULT_HEAP_PADDING_SIZE) & ~(DEFAULT_HEAP_PADDING_SIZE - 1);
		this->bufferW = bufferSize;
		this->bufferH = 1;
		D3D12_HEAP_PROPERTIES heapProp = InitHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_DESC resourceDesc = CreateBufferResourceDesc(bufferSize);
		HRESULT hRes = adapter->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf()));
		if (hRes != S_OK) {
			throw Exception("VertexBuffer: Cant't create buffer");
		}
	}

	VertexBuffer::VertexBuffer(D3D12_NS::GPUAdapter* adapter, ID3D12Resource* defaultResource) :adapter{adapter} {
		if (defaultResource == nullptr || adapter == nullptr) throw Exception("Bad buffer args");
		auto resourceDesc = defaultResource->GetDesc();
		this->defaultBuffer = defaultResource;
		this->bufferW = resourceDesc.Width;
		this->bufferH = resourceDesc.Height;
	}

	void VertexBuffer::CopyData(void* data, unsigned int size) {
		uint32_t bufferSize = bufferH * bufferW;
		if(bufferSize<size) throw Exception("VertexBuffer: buffer size too small");
		if (transmitBuffer.Get() == NULL) {
			D3D12_HEAP_PROPERTIES heapProp = InitHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			D3D12_RESOURCE_DESC resourceDesc = CreateBufferResourceDesc(bufferSize);
			HRESULT hRes = adapter->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(transmitBuffer.GetAddressOf()));
			if (hRes != S_OK) {
				throw Exception("VertexBuffer: Cant't create buffer");
			}
		}
		void* transmitBufferAddress;
		transmitBuffer->Map(0, nullptr, &transmitBufferAddress);
		memcpy(transmitBufferAddress, data, size);
		transmitBuffer->Unmap(0, nullptr);
		destCopyLocation.pResource = defaultBuffer.Get();
		destCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destCopyLocation.PlacedFootprint.Offset = 0;
		destCopyLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8_UINT;
		destCopyLocation.PlacedFootprint.Footprint.Width = bufferW;
		destCopyLocation.PlacedFootprint.Footprint.Height = bufferH;
		destCopyLocation.PlacedFootprint.Footprint.RowPitch = bufferW;
		destCopyLocation.PlacedFootprint.Footprint.Depth = 1;
		srcCopyLocation = destCopyLocation;
		srcCopyLocation.pResource = transmitBuffer.Get();
		static D3D12_BOX box{};
		box.left = 0; box.right = bufferSize;
		box.top = 0; box.bottom = 1;
		box.front = 0; box.back = 1;
		D3D12_NS::TranslateResourceBarrier(adapter->GetCommandList(), defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		//D3D12_NS::TranslateResourceBarrier(commandList, uploadResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
		adapter->GetCommandList()->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, &box);
		D3D12_NS::TranslateResourceBarrier(adapter->GetCommandList(), defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	}

	void VertexBuffer::CopyTextureData(void* data, std::vector<D3D12_SUBRESOURCE_DATA>& subResources) {
		unsigned int ddsSize = 0;
		for (auto subresource : subResources) {
			ddsSize += subresource.SlicePitch;
		}
		if (transmitBuffer.Get() == NULL) {
			D3D12_HEAP_PROPERTIES heapProp = InitHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
			ddsSize += defaultBuffer->GetDesc().Alignment;
			D3D12_RESOURCE_DESC resourceDesc = CreateBufferResourceDesc(ddsSize);
			HRESULT hRes = adapter->GetDevice()->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(transmitBuffer.GetAddressOf()));
			if (hRes != S_OK) {
				throw Exception("VertexBuffer: Cant't create buffer");
			}
		}
		UINT64 res = UpdateSubresources(adapter->GetCommandList(), defaultBuffer.Get(), transmitBuffer.Get(), 0, 0, subResources.size(), &subResources[0]);
		D3D12_NS::TranslateResourceBarrier(adapter->GetCommandList(), defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	ComPtr<ID3D12Resource> VertexBuffer::GetBufferResource() {
		return defaultBuffer;
	}

	D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer::GetGPUVirtualAddress() {
		return defaultBuffer->GetGPUVirtualAddress();
	}

	uint32_t VertexBuffer::GetBufferSize() {
		return bufferH * bufferW;
	}

	VertexBuffer::~VertexBuffer() {

	}
}


namespace Buffers {//RootSignature

	D3D12_ROOT_PARAMETER RootSignature::CreateDescriptorTableWithSingleDescriptorHeap(UINT descriptorsNum,UINT baseShaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE rangeType) {
		D3D12_ROOT_PARAMETER rootParam;
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.DescriptorTable.NumDescriptorRanges = 1;
		D3D12_DESCRIPTOR_RANGE* descriptorRange = new D3D12_DESCRIPTOR_RANGE{};//LEAK OF MEMORY IN SOME CODE
		descriptorRange->RangeType = rangeType;
		descriptorRange->NumDescriptors = descriptorsNum;
		descriptorRange->BaseShaderRegister = baseShaderRegister;
		descriptorRange->RegisterSpace = 0;
		descriptorRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rootParam.DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		return rootParam;
	}

	D3D12_ROOT_PARAMETER RootSignature::CreateRootDescriptor(UINT shaderRegister, UINT registerSpace, RootSignature::RootDescriptorTypes type) {
		D3D12_ROOT_PARAMETER rootParam;
		switch (type) {
		case RootSignature::RootDescriptorTypes::CBV:
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			break;
		case RootSignature::RootDescriptorTypes::SRV:
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			break;
		case RootSignature::RootDescriptorTypes::UAV:
			rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			break;
		}
		rootParam.Descriptor.ShaderRegister = shaderRegister;
		rootParam.Descriptor.RegisterSpace = registerSpace;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		return rootParam;
	}

	D3D12_ROOT_PARAMETER RootSignature::CreateRootConstant(UINT sizeIn32bits, UINT shaderRegister, UINT registerSpace) {
		D3D12_ROOT_PARAMETER rootParam;
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParam.Constants.Num32BitValues = sizeIn32bits;
		rootParam.Constants.ShaderRegister = shaderRegister;
		rootParam.Constants.RegisterSpace = registerSpace;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		return rootParam;
		
	}

	RootSignature::RootSignature(D3D12_NS::GPUAdapter* adapter, UINT paramNum, D3D12_ROOT_PARAMETER* rootParams) : adapter { adapter } {
		//D3D12_ROOT_PARAMETER
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.NumParameters = paramNum;
		rootSignatureDesc.pParameters = rootParams;
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = Materials::TextureFactory::CreateDefaultSamplers();
		rootSignatureDesc.NumStaticSamplers = samplers.size();
		rootSignatureDesc.pStaticSamplers = &samplers[0];
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		ComPtr<ID3DBlob> errorBlob;
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedRS = { D3D_ROOT_SIGNATURE_VERSION_1, rootSignatureDesc };
		//D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
		HRESULT hRes = D3D12SerializeVersionedRootSignature(&versionedRS, serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());
		if (hRes != S_OK) {
			std::string s = (char*)errorBlob->GetBufferPointer();
		}
		adapter->GetDevice()->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}
}


namespace Buffers {//Shader

	/*
	Shader::Shader(std::wstring& fileName) {//without validation of the cso format
		using namespace std;
		ifstream inputStream;
		inputStream.open(fileName,ios_base::binary|ios_base::in);
		inputStream.seekg(0, ios_base::_Seekend);
		unsigned int size = inputStream.tellg();
		D3DCreateBlob(size, shaderData.GetAddressOf());
		inputStream.read((char*)shaderData->GetBufferPointer(), size);
		inputStream.close();
	}	
	*/

	Shader::Shader(const std::wstring fileName, const std::string entryPoint,const std::string target) {//without validation of the cso format
		UINT compileFlags = 0;
		#if defined(DEBUG) || defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG |
			D3DCOMPILE_SKIP_OPTIMIZATION;
		#endif
		ComPtr<ID3DBlob> errorBlob;
		HRESULT hRes = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), target.c_str(), compileFlags, 0, shaderData.GetAddressOf(), errorBlob.GetAddressOf());
		if (hRes != S_OK) {
			MessageBox(NULL, L"Compile shader error", L"d", MB_ICONWARNING | MB_OK);
		}
	}

	void Shader::SetByteCode(D3D12_SHADER_BYTECODE* bytecode) {
		bytecode->BytecodeLength = this->shaderData->GetBufferSize();
		bytecode->pShaderBytecode = this->shaderData->GetBufferPointer();
	}
}

namespace Buffers {//PSO

}