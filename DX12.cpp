#include "DX12.h"
#include "DXGIh.h"
#include "Timer.h"


#pragma comment(lib,"d3d12.lib")

namespace D3D12_NS {

}


namespace D3D12_NS {

	void InitApplication() {//start up the application
		GINS::Init();
	}

	GPUAdapter::GPUAdapter(IUnknown* pAdapter) {
		//create d3d12 with optimal adapter
		HRESULT res = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(d3d12Device.GetAddressOf()));
		if (res != S_OK) {
			exit(-1);
		}
		
		//check descriptor's sizes
		uRtvHandleSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		uDsvHandleSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		uCbvSrvHandleSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//create main fence
		d3d12Device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(mainFence.GetAddressOf()));
		//check MSAA
		supportQualityLevel = CheckQualityLevel(bkBuffFormat, BASIC_SAMPLE_COUNT);
		if (supportQualityLevel.NumQualityLevels == 0) {//can't support 4X MSAA
			exit(-1);
		}
		//inin command interfeces
		InitCommandObjects();
		//infoQueue
		//EnableInfoQueue();
	}

	DXGI_SAMPLE_DESC GPUAdapter::GetMultiSample() {
		DXGI_SAMPLE_DESC sample;
		if (supportQualityLevel.NumQualityLevels == 0) {
			sample.Count = 1;
			sample.Quality = 0;
		}
		else {
			sample.Count = BASIC_SAMPLE_COUNT;
			sample.Quality = BASIC_SAMPLE_COUNT - 1;
		}
		//temp
		sample.Count = 1;
		sample.Quality = 0;
		return sample;
	}

	//Checks the MSAA for current device
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS GPUAdapter::CheckQualityLevel(DXGI_FORMAT format, UINT SampleCount) {
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levelSupport;
		levelSupport.Format = format;
		levelSupport.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		levelSupport.NumQualityLevels = 0;
		levelSupport.SampleCount = SampleCount;
		d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levelSupport, sizeof(levelSupport));
		return levelSupport;
	}

	void GPUAdapter::InitCommandObjects() {
		HRESULT hRes;
		//command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = defaultCmdListType;
		hRes = d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.GetAddressOf()));
		if (hRes != S_OK) {

		}
		//command allocator
		hRes = d3d12Device->CreateCommandAllocator(defaultCmdListType, IID_PPV_ARGS(commandAlloc.GetAddressOf()));
		if (hRes != S_OK) {

		}
		//command list
		hRes = d3d12Device->CreateCommandList(0, defaultCmdListType, commandAlloc.Get(),
			nullptr, IID_PPV_ARGS(commandList.GetAddressOf()));
		if (hRes != S_OK) {

		}
		//hRes = commandList->Close();
		if (hRes != S_OK) {

		}
	}

	void GPUAdapter::FlushCommandQueue() {
		this->commandQueue->Signal(this->mainFence.Get(), ++(this->fenceValue));
		if (this->mainFence->GetCompletedValue() < fenceValue) {
			HANDLE mainFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (mainFenceEvent == INVALID_HANDLE_VALUE) {
				std::wstring debugText = L"FlushCommand - can't create fenceEvent";
				ShowDebugMessage(debugText.c_str());
				exit(-1);
			}
			mainFence->SetEventOnCompletion(fenceValue, mainFenceEvent);
			WaitForSingleObject(mainFenceEvent, INFINITE);
			CloseHandle(mainFenceEvent);
		}
	}

	void GPUAdapter::StartMainDraw() {
		HRESULT hRes = 0;
		
		hRes = commandAlloc->Reset();
		if (hRes != S_OK) {
			ShowDebugMessage(L"Start main Draw commandAlloc Reset");
		}
		hRes = commandList->Reset(commandAlloc.Get(), NULL);
		if (hRes != S_OK) {
			ShowDebugMessage(L"Start main Draw commandList Reset");
		}
	}

	void TranslateResourceBarrier(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
		D3D12_RESOURCE_BARRIER resBarrier;
		resBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		resBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		D3D12_RESOURCE_TRANSITION_BARRIER transition;
		transition.pResource = resource;
		transition.StateBefore = beforeState;
		transition.StateAfter = afterState;
		transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		resBarrier.Transition = transition;
		cmdList->ResourceBarrier(1, &resBarrier);
	}


	void EnableDebug() {
		static ComPtr<ID3D12Debug> debugController0;
		static ComPtr<ID3D12Debug1> debugController1;
		D3D12GetDebugInterface(IID_PPV_ARGS(debugController0.GetAddressOf()));
		debugController0->QueryInterface(IID_PPV_ARGS(debugController1.GetAddressOf()));
		//debugController1->SetEnableGPUBasedValidation(true);
		debugController1->EnableDebugLayer();
		
		//

	}

	void StartMainLoop(std::vector<GINS::WindowResource*> windows) {
		MSG msg;
		int windowsCount = windows.size();
		if (windowsCount == 0) throw std::exception("Null window");
		std::vector<HWND> hWnd;
		for (auto& x : windows) {
			hWnd.push_back(x->GetHwnd());
			ShowWindow(hWnd.back(), SW_SHOW);
		}
		//HWND hWnd = window.GetHwnd();
		//ShowWindow(hWnd, SW_SHOW);
		Timer timer;
		timer.Reset();
		double totalDeltaTime = 0;
		int frames = 0;

		//execute depth/stencil buffer barrier
		GPUAdapter& adapter = windows.back()->GetGPUAdapter();
		adapter.GetCommandList()->Close();
		ID3D12CommandList* cmdListsArray[] = { adapter.GetCommandList() };
		adapter.GetCommandQueue()->ExecuteCommandLists(1, cmdListsArray);
		adapter.FlushCommandQueue();

		while (true) {
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) break;
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}

			for (int i = 0; i < windowsCount; i++) windows[i]->CallDrawProc();
			//window.CallDrawProc();

			frames++;
			totalDeltaTime += timer.Tick();
			if (totalDeltaTime > 1.0f) {
				std::wstring text = L"fps: " + std::to_wstring(frames)+L" mspf: "+ std::to_wstring(totalDeltaTime*1000/frames);
				SetWindowText(hWnd.back(), text.c_str());
				frames = 0;
				totalDeltaTime = 0;
			}
			
		}
	}
}

