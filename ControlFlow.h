#pragma once
#include "Config.h"
#include "DX12.h"
#include "Buffer.h"
#include "MathFunc.h"
#include "Mesh.h"
#include "ConfigShaders.h"
//#include "DXGIh.h"

class GINS::WindowResource;

namespace Control {

	class PipelineObject {
	private:
		D3D12_NS::GPUAdapter* adapter;
		ComPtr<ID3D12PipelineState> mPSO;
	public:
		//переделать передачу по ссылке, а не по копии
		PipelineObject(D3D12_NS::GPUAdapter* adapter, ID3D12RootSignature* rootSignature, Buffers::ShaderByteCodes byteCodes,
			D3D12_STREAM_OUTPUT_DESC streamOutput, D3D12_BLEND_DESC blendState, UINT sampleMask,
			D3D12_RASTERIZER_DESC rasterizerState, D3D12_DEPTH_STENCIL_DESC depthStencilState,
			D3D12_INPUT_LAYOUT_DESC inputLayoutDesc, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibsCutValue,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, UINT numRenderTargets, const DXGI_FORMAT* rtFormats,
			DXGI_FORMAT dsvFormat, DXGI_SAMPLE_DESC sampleDesc, D3D12_CACHED_PIPELINE_STATE cachedPSO,
			D3D12_PIPELINE_STATE_FLAGS flags
		);

		static PipelineObject* CreateDefaultPSO(D3D12_NS::GPUAdapter* adapter, ID3D12RootSignature* rootSignature, Buffers::ShaderByteCodes byteCodes,
			D3D12_INPUT_LAYOUT_DESC inputLayoutDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, UINT numRenderTargets, const DXGI_FORMAT* rtFormats,
			DXGI_FORMAT dsvFormat, DXGI_SAMPLE_DESC sampleDesc, BOOL FrontCounterClockwise = FALSE);

		operator ID3D12PipelineState* () { return mPSO.Get(); }
	};

	/// <summary>
	/// BASIC STRUCTURE IN REGISTER b0 OF VERTEX SHADER (IF YPU WANT TO USE RENDERINGFLOW OBJECT)
	/// </summary>
	


	/// <summary>
	/// THIS CLASS IS USED TO DRAW GRAPHICS OBJECT (FRAME) IN RENDER TARGER
	/// </summary>
	class RenderingFlow {
	private:
		//INFO MEMBERS
		D3D12_NS::GPUAdapter* adapter;
		ComPtr<ID3D12GraphicsCommandList> commandList;
		ComPtr<ID3D12CommandAllocator> commandAllocator;

		GINS::WindowResource* targetWindow;
		Graphics::Camera* camera;

		//THREAD VARIABLES
		HANDLE threadEvent;//event to control thread flow
		HANDLE threadHandle;
		enum class ThreadActivities {FillCommandQueue, TerminateThread};
		ThreadActivities currentActivity;

		//MEMORY
		Buffers::ConstantBuffer* objectRegisterBuffer;
		ComPtr<ID3D12DescriptorHeap> objectBuffersDescriptorHeap;
		Buffers::ConstantBuffer* lightRegisterBuffer;//for direct handle
		Buffers::ConstantBuffer* commonDataRegisterBuffer;
		ComPtr<ID3D12DescriptorHeap> commonDataBuffersDescriptorHeap;
		
		//CONTROL MEMBERS
		bool isFirstFill;//for init commands in the begining
		ComPtr<ID3D12Fence> fence;
		UINT64 fenceValue = 0;
		UINT64 endFenceValue;
		HANDLE fenceEventHandle;
		std::vector<Graphics::RenderObject3D*>* objectsToDraw;

		//Root signature must have table of cbv, which describes object info
		Buffers::RootSignature* rootSignature;
		Materials::TextureFactory* textureFactory;
		
		static D3D12_ROOT_PARAMETER GetTableObjectRootParameter();
		static D3D12_ROOT_PARAMETER GetDirectHanldeLightRootParameter();
		static D3D12_ROOT_PARAMETER GetTableCommonDataRootParameter();
		static D3D12_ROOT_PARAMETER GetTableTextureRootParameter();

		friend DWORD WINAPI RenderingThreadProc(LPVOID param);
	public:
		static const uint8_t CBV_TABLE_B1_SIZE = 10;//max number of object to process in parallel
		static const uint8_t TEXTURE_INDEX_ROOT_PARAM = 3;//index of main texture in root signature slots

		RenderingFlow(D3D12_NS::GPUAdapter* adapter, ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator,
			Buffers::RootSignature* rootSignature, GINS::WindowResource* targetWindow,Graphics::Camera* camera,
			Materials::TextureFactory* textureFactory = nullptr);

		void StartDraw(std::vector<Graphics::RenderObject3D*>* objects,HANDLE eventOnEndCommandList = INVALID_HANDLE_VALUE);

		static std::vector<D3D12_ROOT_PARAMETER> GetRootParams();
	};

}