#pragma once
#include "Config.h"
#include "DX12.h"
#include "Buffer.h"
#include "MathFunc.h"
#include "Material.h"
#include "Light.h"
//#include "ControlFlow.h"


namespace Control {
	class PipelineObject;
}

namespace Graphics {

	enum class DataLocation: char {DEFAULT_HEAP, UPLOAD_HEAP};

	/// 
	/// Info about data,that can be stored with corresponding graphic object
	/// 
	struct InputLayoutInfo {
		//uint32_t sizeInBytes;
		uint32_t strideInBytes;
		uint32_t elementCount;
		DataLocation locationType;
	};


	/// <summary>
	/// Describes buffers, that can be stored to represent an object
	/// </summary>
	class Object3D {
	private:
		D3D12_NS::GPUAdapter* adapter;

		unsigned numberOfInputLayouts;
		InputLayoutInfo* inputLayouts;
		struct BufferInfo {//parts of the BIG buffer
			Buffers::IBuffer* buffer;
			uint32_t elementCount;
		};
		std::vector<std::vector<BufferInfo> > inputLayoutsBuffers{ MAX_INPUT_LAYOUT_COUNT };
		bool isAllocated = false;

	public:
		static const int MAX_INPUT_LAYOUT_COUNT = 15;
		Object3D(D3D12_NS::GPUAdapter* adapter,const std::vector<InputLayoutInfo>& inputLayouts);
		void AllocateMemory(std::vector<int> infoIndexes = {}, std::vector<std::vector<uint32_t>> infoBuffers = {});
		void InitBuffers(std::initializer_list<unsigned> inputIndexList, std::initializer_list<void*> dataList);
		//D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(unsigned index,UINT vertexCount = 1);
		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(unsigned index,INT startVertexIndex,UINT* vertexCountToTheEndOfBuffer);
		//D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(unsigned index,DXGI_FORMAT format);
		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView(unsigned index, INT indexNumber, UINT* indexCountToTheEndOfBuffer);
		static uint32_t GetMaxElementCountInBuffer(uint32_t elementSize, DataLocation locationType);
	};


	class RenderObject3D {//ONLY PRIMITIVE OBJECTS (one material, similar light)
	private:
		Object3D* objectBuffers;
		unsigned vertexBVIndex;//
		UINT vertexCount;
		UINT indexCount;
		unsigned indexBVIndex;//
		DXGI_FORMAT ibvFormat;

		Control::PipelineObject* mPSO;
		D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;

		float sx{ 1 }, sy{ 1 }, sz{ 1 };
		float rx{ 0 }, ry{ 0 }, rz{ 0 };//radians
		float dx{ 0 }, dy{ 0 }, dz{ 0 };
		DirectX::XMMATRIX worldMatrix;

		//lightInfo
		DirectX::XMFLOAT3 ambientLight = {0,0,0};
		std::vector<Light::SourceLight*> sourceLightVector;

		//material info
		Materials::Material material{};
		Materials::TextureFactory::texture_index textureIndex = Materials::TextureFactory::INVALID_TEXTURE_INDEX;

		void CreateDefaultWorldMatrix();
	public:
		RenderObject3D(Object3D* objectBuffers,unsigned vbvIndex, unsigned ibvIndex, DXGI_FORMAT ibvFormat,UINT vertexCount, UINT indexCount, Control::PipelineObject* mPSO,
			D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
		Control::PipelineObject* GetCurrentPSO() { return mPSO; };
		D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() { return primitiveTopology; };
		const DirectX::XMMATRIX* GetWorldMatrix() { return &worldMatrix; };

		bool DrawIndexInstance(ID3D12GraphicsCommandList* cmdList, Materials::TextureFactory* textureFactory = nullptr);
		UINT GetIndexCount() { return indexCount; };
		//transform functions
		void SetObjectTranslate(float dx,float dy, float dz);
		void SetObjectSize(int size);
		void SetObjectRotate(float xRad, float yRad, float zRad);
		void SetObjectScale(float sx, float sy, float sz);
		//light functions
		void SetAmbientLight(DirectX::XMFLOAT3 lightPower);
		void AddLightResources(std::initializer_list<Light::SourceLight*> lights);
		const std::vector<Light::SourceLight*>& GetLightsVector() { return sourceLightVector; };
		DirectX::XMVECTOR GetAmbientColor() { return DirectX::XMVectorSet(ambientLight.x,ambientLight.y,ambientLight.z,1); }
		//material functions
		void SetMaterial(Materials::Material material);
		void CopyMaterialInfo(Materials::MaterialHLSL* dest);
		void SetTextureIndex(Materials::TextureFactory::texture_index texIndex);
	};

	class Camera {
	private:
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR targetPoint;
		DirectX::XMMATRIX viewMatrix;

		//sphere coord according to targetPoint
		float fi,psi,radius;
		float direction;

		void ComputeViewProjMatrix();
	public:
		Camera(float vx,float vy, float vz, float px, float py, float pz);
		void RotateCamera(float deegresFi, float deegresPsi);// rotate in sphera coordinates, targetPoint - center
		void ChangeRadius(float delta);
		DirectX::XMMATRIX GetViewMatrix() { return viewMatrix; };
		DirectX::XMVECTOR GetEyePoint() { return position; };
	};


	//MESH CONSTRUCTOR
	struct MeshInputInfo {
		uint16_t sizeInBytes = 0;
		DataLocation heapLocation = DataLocation::DEFAULT_HEAP;
	};

	struct VertexLayoutInfo {
		uint16_t positionSizeBytes = 0;
		uint16_t positionOffsetBytes = 0;
		uint16_t normalSizeBytes = 0;
		uint16_t normalOffsetBytes = 0;
		uint16_t texCoordOffsetBytes = 0;//2D COORD
	};

	using VertexTransformFunction = void(*)(void* vertexData);

	struct MeshConstructInputInfo {
		MeshInputInfo vertex{};
		VertexLayoutInfo vertexLayout{};
		MeshInputInfo indexes{};
		VertexTransformFunction vertexFunction = nullptr;
	};

	struct MeshOutputInfo {
		uint32_t vertexCount;
		uint32_t indexCount;
	};

	Object3D* CreateConusMesh(D3D12_NS::GPUAdapter* adapter,MeshConstructInputInfo dataInfo, float btmRadius, float upRadius, uint32_t segmentsCount, uint32_t sideRingsCount,
		MeshOutputInfo* outputInfo);

	Object3D* CreateSphereMesh(D3D12_NS::GPUAdapter* adapter, MeshConstructInputInfo dataInfo, float sphereRadius, unsigned tesselationCount,
		MeshOutputInfo* outputInfo);

	Object3D* CreateGridMesh(D3D12_NS::GPUAdapter* adapter, MeshConstructInputInfo dataInfo, unsigned int xWidth, unsigned int zDepth,
		MeshOutputInfo* outputInfo);

}