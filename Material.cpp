#include "Material.h"
#include "Buffer.h"
#include "ControlFlow.h"
#include "DDSTextureLoader12.h"

namespace Materials {

	Material::Material(DirectX::XMFLOAT3 albedo, DirectX::XMFLOAT3 rFresnel, float shininess) {
		materialInfo.albedo = albedo;
		materialInfo.rFrensel = rFresnel;
		materialInfo.shininess = shininess; 
	}

}


//Texture factory
namespace Materials {
	bool TextureFactory::isFactoryCreated = false;

	TextureFactory::TextureFactory(D3D12_NS::GPUAdapter* adapter) :adapter{adapter} {
		if (TextureFactory::isFactoryCreated) throw Exception("TexFactory was already created");
		try {
			if(adapter == nullptr) throw Exception("TexFactory bad args");
			TextureFactory::isFactoryCreated = true;
		}
		catch (...) {
			TextureFactory::isFactoryCreated = false;
			throw;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE TextureFactory::GetCPUTextureDescriptorHandle(TextureFactory::texture_index textureIndex) {
		//if (textureIndex >= totalTexturesCount) throw Exception("bad texture index");
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		uint32_t heapIndex = textureIndex / TextureFactory::MAX_SRV_IN_HEAP;
		uint32_t texIndex = textureIndex % TextureFactory::MAX_SRV_IN_HEAP;
		handle = srvHeaps[heapIndex]->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += texIndex * adapter->GetCbvSrvHandleSize();
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE TextureFactory::GetGPUTextureDescriptorHandle(TextureFactory::texture_index textureIndex) {
		//if (textureIndex >= totalTexturesCount) throw Exception("bad texture index");
		D3D12_GPU_DESCRIPTOR_HANDLE handle;
		uint32_t heapIndex = textureIndex / TextureFactory::MAX_SRV_IN_HEAP;
		uint32_t texIndex = textureIndex % TextureFactory::MAX_SRV_IN_HEAP;
		handle = srvHeaps[heapIndex]->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += texIndex * adapter->GetCbvSrvHandleSize();
		return handle;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC TextureFactory::GetSrvDescFromResource(ID3D12Resource* resource) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		D3D12_RESOURCE_DESC resDesc = resource->GetDesc();
		srvDesc.Format = resDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//ïîêà òîëüêî îáðàáàòûâàåì 2ä
		if (resDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) throw Exception("Bad resource format");
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;
		return srvDesc;
	}


	TextureFactory::texture_index TextureFactory::RegisterTexture(std::wstring fileName) {
		if (totalTexturesCount % TextureFactory::MAX_SRV_IN_HEAP == 0) {
			//add new descriptor heap
			srvHeaps.push_back(Buffers::CreateDescriptorHeap(adapter->GetDevice(), MAX_SRV_IN_HEAP,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE));
		}
		std::unique_ptr<uint8_t[]> ddsData;
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		ComPtr<ID3D12Resource> defaultResource;
		HRESULT hRes = DirectX::LoadDDSTextureFromFile(adapter->GetDevice(), fileName.c_str(),
			defaultResource.GetAddressOf(),ddsData, subresources);
		if (hRes != S_OK) {
			throw Exception("Bad dds file");
		}
		//copy dds data
		textureResources.push_back(new Buffers::VertexBuffer(adapter,defaultResource.Get()));
		textureResources.back()->CopyTextureData(ddsData.get(), subresources);


		try {
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = GetSrvDescFromResource(defaultResource.Get());
			adapter->GetDevice()->CreateShaderResourceView(defaultResource.Get(),
				&srvDesc, GetCPUTextureDescriptorHandle(totalTexturesCount));
		}
		catch (...) {
			textureResources.pop_back();
			//delete data??
			throw;
		}
	
		return totalTexturesCount++;
	}

	TextureFactory::texture_index TextureFactory::lastUsedTexture = TextureFactory::INVALID_TEXTURE_INDEX;

	void TextureFactory::ResetRootTextures() {
		lastUsedTexture = TextureFactory::INVALID_TEXTURE_INDEX;
	}

	bool TextureFactory::SetTextureToRootSignature(ID3D12GraphicsCommandList* cmdList, texture_index texIndex) {
		if (texIndex >= totalTexturesCount) throw Exception("Bad texture index");
		if (texIndex == lastUsedTexture) return false;
		uint32_t currentHeap = texIndex / MAX_SRV_IN_HEAP;
		//if (currentHeap != lastUsedTexture / MAX_SRV_IN_HEAP) {  ÏÅÐÅÄÅËÀÒÜ ÏÎÒÎÌ CONTROLfLOW
			ID3D12DescriptorHeap* heaps[] = {srvHeaps[currentHeap].Get()};
			cmdList->SetDescriptorHeaps(1, heaps);
		//}
		lastUsedTexture = texIndex;
		cmdList->SetGraphicsRootDescriptorTable(Control::RenderingFlow::TEXTURE_INDEX_ROOT_PARAM,
			TextureFactory::GetGPUTextureDescriptorHandle(texIndex));
		return true;
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> TextureFactory::CreateDefaultSamplers() {
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplerVector;
		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		sampler.MinLOD = 0;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		samplerVector.push_back(sampler);
		return samplerVector;
	}
}