#pragma once
#include "Config.h"
#include "DX12.h"
#include "MathFunc.h"
#include "Buffer.h"

namespace Materials {

	using DirectX::XMVECTOR;
	using DirectX::XMFLOAT3;
	//using DirectX::XMFLOAT4;

	//#pragma pack(1)
	struct MaterialHLSL {
		//light settings
		XMFLOAT3 albedo = { 0,0,0 }; //amount of ligth, that can be reflected
		float shininess;//0..1, 1 - all specular light only in one direction, 0 - specular light diffuses
		XMFLOAT3 rFrensel = { 0,0,0 }; //amount of specular light when see to the normal of the surface at 0 deegres
		float padding;
	};

	struct Material {
	private:
		MaterialHLSL materialInfo;
	public:
		Material(DirectX::XMFLOAT3 albedo = {0,0,0}, DirectX::XMFLOAT3 rFresnel = { 0.6,0.6,0.6 }, float shininess = 1.0f);
		void CopyMaterial(void* dest) { memcpy(dest,&materialInfo,sizeof(materialInfo)); };
	};


	//Represents all textures in the programm
	class TextureFactory {
	public:
		typedef UINT32 texture_index;
		static const texture_index INVALID_TEXTURE_INDEX = UINT32_MAX;
	private:
		const UINT32 MAX_SRV_IN_HEAP = 20;//amout of srv, that one descriptor heap can process

		D3D12_NS::GPUAdapter* adapter;
		static bool isFactoryCreated;
		std::vector<ComPtr<ID3D12DescriptorHeap>> srvHeaps;
		std::vector<Buffers::VertexBuffer*> textureResources;
		texture_index totalTexturesCount = 0;
		static texture_index lastUsedTexture;

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUTextureDescriptorHandle(texture_index textureIndex);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUTextureDescriptorHandle(texture_index textureIndex);
		D3D12_SHADER_RESOURCE_VIEW_DESC GetSrvDescFromResource(ID3D12Resource* resource);
	public:
		TextureFactory(D3D12_NS::GPUAdapter* adapter);

		TextureFactory(const TextureFactory& t) = delete;
		TextureFactory(TextureFactory&& t) = delete;
		TextureFactory& operator=(const TextureFactory& t) = delete;
		TextureFactory& operator=(TextureFactory&& t) = delete;
		static std::vector<D3D12_STATIC_SAMPLER_DESC> CreateDefaultSamplers();

		texture_index RegisterTexture(std::wstring fileName);
		bool SetTextureToRootSignature(ID3D12GraphicsCommandList* cmdList, texture_index texIndex);
		void ResetRootTextures();
	};

}