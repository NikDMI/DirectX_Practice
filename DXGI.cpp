#include "DXGIh.h"
#include "Config.h"
#include "DX12.h"

#pragma comment(lib,"dxgi.lib")

using std::wstring;

namespace GINS {
	ComPtr<IDXGIFactory> factory;

	unsigned currentWindowClassIndex = 0;
	std::map<HWND, WindowResource*> createdWindows;
}

namespace GINS {

	std::wstring RegisterNewWindowClass(WndProc wndProc) {
		std::wstring ClassName = BASIC_WINDOW_CLASS_NAME + std::to_wstring(currentWindowClassIndex++);
		WNDCLASSEX wc;
		wc.cbClsExtra = 0;
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.cbWndExtra = 0;
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hIconSm = LoadIconW(NULL, IDI_APPLICATION);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = ClassName.c_str();
		wc.hInstance = GetModuleHandle(NULL);
		wc.lpfnWndProc = wndProc;
		RegisterClassEx(&wc);
		return ClassName;
	}
	
	void Init() {
		CreateDXGIFactory(IID_PPV_ARGS(factory.GetAddressOf()));
		//createBasicClass for Windows
		RegisterNewWindowClass(DefaultWndProc);
	}

	WindowResource::WindowResource(D3D12_NS::GPUAdapter* pAdapter, int W, int H, DrawProc drawProc, WndProc wndProc):
		drawProc{drawProc} {
		this->currentAdapter = pAdapter;
		this->wndProc = wndProc;
		if (wndProc != DefaultWndProc) {
			//создать новый оконный класс... (без проверки на повтор оконной процедуры)
			windowClassName = RegisterNewWindowClass(wndProc);
		}
		else {
			windowClassName = BASIC_WINDOW_CLASS_NAME_DEFAULT;
		}
		hWnd = CreateWindow(windowClassName.c_str(), L"Window", WS_OVERLAPPEDWINDOW, 0, 0, W, H, NULL, NULL, GetModuleHandle(NULL), 0);
		createdWindows.insert({ hWnd,this });
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		this->WindowW = clientRect.right;
		this->WindowH = clientRect.bottom;
		this->CreateBackBufferSwapChain();
		this->CreateDescriptorHeap();
		//this->CreateViews(currentAdapter->GetMultiSample());
		this->CreateViews({1,0});
		//this->SetViewportToFullWindow();

	}

	void WindowResource::ResizeWindow() {
		RECT r; GetClientRect(this->hWnd, &r);
		this->WindowH = r.bottom;
		this->WindowW = r.right;
		this->currentAdapter->StartMainDraw();
		bkBuffSwapChain->ResizeBuffers(WindowResource::BACK_BUFFERS_COUNT, WindowW, WindowH, GPUAdapter::bkBuffFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		this->CreateViews({ 1,0 });
		this->currentAdapter->GetCommandList()->Close();
		ID3D12CommandList* cmdLists[1]; cmdLists[0] = this->currentAdapter->GetCommandList();
		this->currentAdapter->GetCommandQueue()->ExecuteCommandLists(1, cmdLists);
		this->currentAdapter->FlushCommandQueue();
		this->currentBkBufferIndex = 0;
	}

	float WindowResource::GetRatio() {
		return (float)(this->WindowW) / this->WindowH;
	}

	void WindowResource::CreateBackBufferSwapChain() {
		
		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		swapChainDesc.BufferDesc.Width = this->WindowW;//width of window
		swapChainDesc.BufferDesc.Height = this->WindowH;//h of window
		swapChainDesc.BufferDesc.Format = GPUAdapter::bkBuffFormat;
		swapChainDesc.BufferDesc.RefreshRate = { 60,1 };
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		
		swapChainDesc.BufferCount = BACK_BUFFERS_COUNT;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.SampleDesc = {1,0};
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;//??
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		HRESULT hRes = factory->CreateSwapChain(currentAdapter->commandQueue.Get(), &swapChainDesc, bkBuffSwapChain.GetAddressOf());
		if (hRes != S_OK) {
			throw std::exception("Can't create swap chain");
		}

		/*
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
		swapChainDesc.Width = this->WindowW;
		swapChainDesc.Height = this->WindowH;
		swapChainDesc.Format = GPUAdapter::bkBuffFormat;
		swapChainDesc.Stereo = TRUE;
		swapChainDesc.SampleDesc = currentAdapter->GetMultiSample();
		swapChainDesc.BufferCount = BACK_BUFFERS_COUNT;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_STRAIGHT;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		HRESULT hRes = factory->CreateSwapChainForHwnd(currentAdapter->commandQueue.Get(), hWnd, &swapChainDesc, NULL, NULL, bkBuffSwapChain.GetAddressOf());
		if (hRes != S_OK) {

		}
		*/
	}

	void WindowResource::CreateDescriptorHeap() {
		//RTV descriptors
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
		heapDesc.NumDescriptors = this->BACK_BUFFERS_COUNT;
		heapDesc.NodeMask = 0;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HRESULT hRes = currentAdapter->d3d12Device->CreateDescriptorHeap(&heapDesc,IID_PPV_ARGS(this->rtvDescHeap.GetAddressOf()));//assume that we work only with one adapter (GPU) - global in other module
		//DSV descr
		heapDesc.NumDescriptors = 1;
		heapDesc.NodeMask = 0;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		hRes = currentAdapter->d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(this->dsvDescHeap.GetAddressOf()));
	}

	void WindowResource::CreateViews(DXGI_SAMPLE_DESC sample) {
		//views for rtv
		HRESULT hRes;
		ComPtr<ID3D12Resource> bufferResources[BACK_BUFFERS_COUNT];
		D3D12_CPU_DESCRIPTOR_HANDLE handleRtv = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
		for (int i = 0; i < BACK_BUFFERS_COUNT; i++) {
			hRes = this->bkBuffSwapChain->GetBuffer(i, IID_PPV_ARGS(bufferResources[i].GetAddressOf()));
			currentAdapter->d3d12Device->CreateRenderTargetView(bufferResources[i].Get(), nullptr, handleRtv);
			handleRtv.ptr += currentAdapter->uRtvHandleSize;
		}
		//view for d/s buffer
		this->CreateDepthStencilBuffer(sample);
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = this->dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
		currentAdapter->d3d12Device->CreateDepthStencilView(this->depthStencilResource.Get(), nullptr, dsvHandle);
		//change the state of buffer
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = this->depthStencilResource.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		currentAdapter->commandList->ResourceBarrier(1, &barrier);
	}

	void WindowResource::CreateDepthStencilBuffer(DXGI_SAMPLE_DESC sample) {
		D3D12_HEAP_PROPERTIES heapProp;
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProp.CreationNodeMask = 1;
		heapProp.VisibleNodeMask = 1;
		heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = 0;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Width = this->WindowW;
		resourceDesc.Height = this->WindowH;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		resourceDesc.Format = DEPTH_STENCIL_FORMAT;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc = sample;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = DEPTH_STENCIL_FORMAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		HRESULT resHandle = currentAdapter->d3d12Device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(depthStencilResource.GetAddressOf()));
		if (resHandle != S_OK) {
			//this->currentAdapter->GetLastInfoMessage();
			ShowDebugMessage(L"WindowResource::CreateDepthStencilBuffer");
		}
	}

	void WindowResource::SetViewportToFullWindow(ID3D12GraphicsCommandList* cmdList) {
		//static D3D12_VIEWPORT vp;
		vp.Height = this->WindowH;
		vp.Width = this->WindowW;
		vp.MaxDepth = 1.0f;
		vp.MinDepth = 0.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		cmdList->RSSetViewports(1, &vp);
	}

	void WindowResource::SetScissorRectToFullWindow(ID3D12GraphicsCommandList* cmdList) {
		scissorRect.left = 0;
		scissorRect.right = this->WindowW;
		scissorRect.top = 0;
		scissorRect.bottom = this->WindowH;
		cmdList->RSSetScissorRects(1, &scissorRect);
	}

	void WindowResource::PresentBuffer() {
		bkBuffSwapChain->Present(0, 0);
		this->currentBkBufferIndex = (currentBkBufferIndex + 1) % this->BACK_BUFFERS_COUNT;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE WindowResource::GetCurrentBkBufferView() const{
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		handle = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += currentAdapter->uRtvHandleSize * this->currentBkBufferIndex;
		return handle;
	}

	ComPtr<ID3D12Resource> WindowResource::GetCurrentBkBuffer() const {
		ComPtr<ID3D12Resource> buffer;
		bkBuffSwapChain->GetBuffer(this->currentBkBufferIndex, IID_PPV_ARGS(buffer.GetAddressOf()));
		return buffer;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE WindowResource::GetDepthStencilView() const {
		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		handle = dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
		return handle;
	}


	wstring GetAdapterInfo() {
		wstring resStr;
		IDXGIAdapter* adapter;
		int i=0;
		while (true) {
			adapter = nullptr;
			factory->EnumAdapters(i, &adapter);
			if (adapter == nullptr) break;
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			resStr += L"Adapter " + std::to_wstring(i) + L": ";
			resStr += desc.Description; resStr += GetOutputInfo(adapter) + L"\n";
			adapter->Release();
			
			i++;
		}
		return resStr;
	}

	wstring GetOutputInfo(IDXGIAdapter* adapter) {
		wstring resStr;
		IDXGIOutput* display;
		int i = 0;
		while (true) {
			display = nullptr;
			adapter->EnumOutputs(i, &display);
			if (display == nullptr) break;
			DXGI_OUTPUT_DESC desc;
			display->GetDesc(&desc);
			resStr += L"Output " + std::to_wstring(i) + L": ";
			resStr += desc.DeviceName; resStr += L"\n";
			display->Release();

			i++;
		}
		return resStr;
	}

	std::pair<HWND, WindowResource*> WindowResource::GetWindowResourceByHwnd(HWND hWnd) {
		std::map<HWND,WindowResource*>::iterator iter = createdWindows.find(hWnd);
		//if (iter == createdWindows.end()) throw std::exception("Can't find the window");
		if (iter == createdWindows.end()) return { 0,nullptr };
		return *iter;
	}

	void WindowResource::CallDrawProc() {
		if (drawProc != nullptr) drawProc(*this);
	}

	LRESULT CALLBACK DefaultWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

		static std::pair<HWND, WindowResource*> currentWindowPair = WindowResource::GetWindowResourceByHwnd(hWnd);
		if(currentWindowPair.first != hWnd)
			currentWindowPair = WindowResource::GetWindowResourceByHwnd(hWnd);

		WindowResource* currentWindow = currentWindowPair.second;
		PAINTSTRUCT ps;
		HDC hdc;
		switch (uMsg) {
		case WM_CREATE:
			break;

		case WM_EXITSIZEMOVE:
			currentWindow->ResizeWindow();
			break;

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_KEYDOWN:
			if (wParam == (WPARAM)0x44) camera->RotateCamera(3,0);
			if (wParam == (WPARAM)0x57) camera->RotateCamera(0, 3);
			if (wParam == (WPARAM)0x53) camera->RotateCamera(0,-3);
			if (wParam == (WPARAM)0x41) camera->RotateCamera(-3, 0);

		case WM_CHAR:
			switch (wParam) {
			case 'z':
				camera->ChangeRadius(-5);
				break;
			case 'x':
				camera->ChangeRadius(5);
				break;
			}
			break;
			
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		return 0;
	}



	///////TEST VARIABLES
	D3D12_VERTEX_BUFFER_VIEW cubeView;
	ComPtr<ID3D12Resource> cubeDefaultBuffer;
	Graphics::Camera* camera;

	float dxCamera = -100;
	float dyCamera = -100;
	float dzCamera=-100;

	void VertexTransformData(void* vertex) {
		XMFLOAT3* pos = (XMFLOAT3*)vertex;
		XMFLOAT4* color = (XMFLOAT4*)((XMFLOAT3 *)vertex + 1);
		pos->y = 0.3f * (pos->z * sin(20.0f * pos->x)+0.5*cos(10*pos->x)) + pos->x * cos(pos->z * 9.0f);
		if (pos->y > 0.1) {
			DirectX::XMStoreFloat4(color, DirectX::XMVectorSet(0.5+pos->y, 0.3 + pos->y, 0.4 + pos->y, 1));
		}
		else if (pos->y < 0.0) {
			DirectX::XMStoreFloat4(color, DirectX::XMVectorSet(0.0 + pos->y, 0.5 + pos->y, 0.5 + pos->y, 1));

		}
		else {
			DirectX::XMStoreFloat4(color, DirectX::XMVectorSet((rand()%100)/100.0, 0.2, 0.2, 1));
		}
	}

}