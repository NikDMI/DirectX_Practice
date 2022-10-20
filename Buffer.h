#pragma once
#include "Config.h"
#include "DX12.h"
#include <string>

namespace Buffers {

	ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	const UINT DEFAULT_HEAP_PADDING_SIZE = 256;
	const UINT UPLOAD_HEAP_PADDING_SIZE = 256;
	const UINT MAX_UPLOAD_HEAP_SIZE = 16000;
	const UINT MAX_DEFAULT_HEAP_SIZE = 16000;

	class IBuffer {
	public:
		IBuffer() {};
		virtual ~IBuffer() {};

		virtual void CopyData(void* data, unsigned int size) = 0;
		virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() = 0;
		virtual uint32_t GetBufferSize() = 0;
		virtual ComPtr<ID3D12Resource> GetBufferResource() = 0;

	};

	struct DataInfo {
		UINT memberSize;
		UINT memberCount;
		void* data;
	};

	class ConstantBuffer:public IBuffer {//Buffer in UPLOAD memory
	private:
		D3D12_NS::GPUAdapter* adapter;
		ID3D12Device* d3d12Device;

		ComPtr<ID3D12Resource> buffer; //buffer in an upload heap for interconnection with HLSL
		void* bufferCPUAddress;
		std::vector<DataInfo> bufferInfo;
		UINT bufferSize;

		ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE bufferView;

		static UINT CountPaddingStructureSize(UINT sizeOfStructure);
	public:
		ConstantBuffer(D3D12_NS::GPUAdapter* adapter, std::vector<DataInfo>& structureInfo);
		ConstantBuffer(const ConstantBuffer&) = delete;
		ConstantBuffer& operator=(const ConstantBuffer&) = delete;
		//D3D12_CONSTANT_BUFFER_VIEW_DESC GetBufferView();
		~ConstantBuffer();
		virtual void CopyData(void* data, unsigned int size) override;
		void* GetMappedAddress() { return bufferCPUAddress; };

		ID3D12DescriptorHeap* GetDescriptorHeap();//пока по дефолту
		virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() override;
		virtual uint32_t GetBufferSize() override;
		virtual ComPtr<ID3D12Resource> GetBufferResource() override;
	};


	class VertexBuffer :public IBuffer {//Vertext buffer in default memory (const) - continuous data with padding in the end of the buffer
	private:
		D3D12_NS::GPUAdapter* adapter;

		ComPtr<ID3D12Resource> defaultBuffer;
		ComPtr<ID3D12Resource> transmitBuffer;
		//uint16_t bufferSize;
		uint16_t bufferH;
		uint16_t bufferW;

		//this vars used while executing command queue
		D3D12_TEXTURE_COPY_LOCATION destCopyLocation;
		D3D12_TEXTURE_COPY_LOCATION srcCopyLocation;

	public:
		VertexBuffer(D3D12_NS::GPUAdapter* adapter, uint16_t bufferSize);
		VertexBuffer(D3D12_NS::GPUAdapter* adapter, ID3D12Resource* defaultResource);
		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer& operator=(const VertexBuffer&) = delete;

		virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() override;

		//!!!only one time per command list frame execution!!!
		virtual void CopyData(void* data, unsigned int size) override;
		void CopyTextureData(void* data, std::vector<D3D12_SUBRESOURCE_DATA>& subResources);

		virtual uint32_t GetBufferSize() override;
		virtual ComPtr<ID3D12Resource> GetBufferResource() override;

		~VertexBuffer();
	};



	class RootSignature {
	private:
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3D12RootSignature> rootSignature;
		D3D12_NS::GPUAdapter* adapter;
	public:
		enum class RootDescriptorTypes {CBV, SRV, UAV};

		RootSignature(D3D12_NS::GPUAdapter* adapter,UINT paramsNum, D3D12_ROOT_PARAMETER* rootParams);
		static D3D12_ROOT_PARAMETER CreateDescriptorTableWithSingleDescriptorHeap(UINT descriptorsNum, UINT baseShaderRegister, D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
		static D3D12_ROOT_PARAMETER CreateRootDescriptor(UINT shaderRegister, UINT registerSpace, RootDescriptorTypes type);
		static D3D12_ROOT_PARAMETER CreateRootConstant(UINT sizeIn32bits, UINT shaderRegister, UINT registerSpace);
		operator ID3D12RootSignature* () { return rootSignature.Get(); }
	};


	class Shader {
	private:
		ComPtr<ID3DBlob> shaderData;
	public:
		//Shader(std::wstring fileName);
		Shader(const std::wstring fileName, const std::string entryPoint, const std::string target);
		void SetByteCode(D3D12_SHADER_BYTECODE* bytecode);
		operator ID3DBlob*() { return shaderData.Get(); };
	};


	struct ShaderByteCodes {
		D3D12_SHADER_BYTECODE VS{nullptr, 0};
		D3D12_SHADER_BYTECODE PS{ nullptr, 0 };
		D3D12_SHADER_BYTECODE DS{ nullptr, 0 };
		D3D12_SHADER_BYTECODE HS{ nullptr, 0 };
		D3D12_SHADER_BYTECODE GS{ nullptr, 0 };
	};

}