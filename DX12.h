#pragma once
#include <d3d12.h>
#include "Config.h"
#include <vector>
//#include "DXGIh.h"

namespace GINS {
	class WindowResource;
}

namespace D3D12_NS {
	extern DXGI_FORMAT bkBuffFormat;

	class GPUAdapter {
	private:
		ComPtr<ID3D12Device> d3d12Device;
		//command interfeces in the main programm thread
		ComPtr<ID3D12CommandQueue> commandQueue;
		ComPtr<ID3D12CommandAllocator> commandAlloc;//main cmdList
		ComPtr<ID3D12GraphicsCommandList> commandList;
		//Descriptors Sizes
		UINT uRtvHandleSize;
		UINT uDsvHandleSize;
		UINT uCbvSrvHandleSize;
		//formats
		//fences
		ComPtr<ID3D12Fence> mainFence;
		UINT fenceValue = 0;
		
		static const D3D12_COMMAND_LIST_TYPE defaultCmdListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
		void InitCommandObjects();
		//implement functions
		const UINT BASIC_SAMPLE_COUNT = 4;
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS supportQualityLevel;
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS CheckQualityLevel(DXGI_FORMAT format, UINT SampleCount);

		//void EnableInfoQueue();
		//ComPtr<ID3D12InfoQueue> infoQueue;
		friend GINS::WindowResource;
	public:
		static const DXGI_FORMAT bkBuffFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		explicit GPUAdapter(IUnknown* pAdapter = nullptr);
		DXGI_SAMPLE_DESC GetMultiSample();
		void FlushCommandQueue();
		void StartMainDraw();//reset main list and allocator
		ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }
		ID3D12CommandQueue* GetCommandQueue() { return commandQueue.Get(); }
		ID3D12Device* GetDevice() { return d3d12Device.Get(); }
		~GPUAdapter() {
			FlushCommandQueue();
		}
		//void GetLastInfoMessage();
		UINT GetCbvSrvHandleSize() { return uCbvSrvHandleSize; }
	};

	void InitApplication();

	void StartMainLoop(std::vector<GINS::WindowResource*> windows);//starts game loop for this window

	void EnableDebug();

	void TranslateResourceBarrier(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
}