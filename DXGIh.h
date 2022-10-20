#pragma once
#include <dxgi1_2.h>
#include <d3d12.h>
#include "Config.h"
#include <string>
#include "DX12.h"
#include <string>
#include <map>
#include <DirectXColors.h>
#include "Vertex.h"
#include "MathFunc.h"
#include "Buffer.h"
#include "Timer.h"
#include "Mesh.h"
#include "ControlFlow.h"
#include <ctime>

using D3D12_NS::GPUAdapter;

namespace GINS {
	extern ComPtr<IDXGIFactory> factory;
	void Init();
	std::wstring GetAdapterInfo();
	std::wstring GetOutputInfo(IDXGIAdapter* adapter);

	using WndProc = LRESULT (CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	using DrawProc = void (*)(WindowResource& window);
	LRESULT CALLBACK DefaultWndProc(HWND, UINT, WPARAM, LPARAM);
	const std::wstring BASIC_WINDOW_CLASS_NAME = L"APPLICATION_BASIC_WINDOW_CLASS_D3D12_";
	const std::wstring BASIC_WINDOW_CLASS_NAME_DEFAULT = L"APPLICATION_BASIC_WINDOW_CLASS_D3D12_0";
	extern unsigned currentWindowClassIndex;
	extern std::map<HWND, WindowResource*> createdWindows;


	class WindowResource {
	private:
		GPUAdapter* currentAdapter;
		//
		int WindowH;
		int WindowW;
		HWND hWnd;
		WndProc wndProc;
		DrawProc drawProc;
		std::wstring windowClassName;
		//
		ComPtr<IDXGISwapChain> bkBuffSwapChain;
		static const UINT BACK_BUFFERS_COUNT = 2;
		UINT currentBkBufferIndex = 0;
		ComPtr<ID3D12Resource> depthStencilResource;
		ComPtr<ID3D12DescriptorHeap> rtvDescHeap;
		ComPtr<ID3D12DescriptorHeap> dsvDescHeap;
		//
		//
		void CreateDescriptorHeap();
		void CreateDepthStencilBuffer(DXGI_SAMPLE_DESC sample);
		void CreateViews(DXGI_SAMPLE_DESC sample);
		//
		D3D12_VIEWPORT vp;
		D3D12_RECT scissorRect;
		//
		
	public:
		const static DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		void CreateBackBufferSwapChain();
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBkBufferView() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
		HWND GetHwnd() { return hWnd; }
		WindowResource(GPUAdapter* pAdapter, int W, int H, DrawProc drawProc = nullptr, WndProc wndProc = DefaultWndProc);
		static std::pair<HWND, WindowResource*> GetWindowResourceByHwnd(HWND hWnd);
		void CallDrawProc();
		GPUAdapter& GetGPUAdapter() { return *currentAdapter; }
		ComPtr<ID3D12Resource> GetCurrentBkBuffer() const;
		void SetViewportToFullWindow(ID3D12GraphicsCommandList* cmdList);
		void SetScissorRectToFullWindow(ID3D12GraphicsCommandList* cmdList);
		void PresentBuffer();
		void ResizeWindow();
		float GetRatio();
	};

	/// <summary>
	/// Test functions (for show, delete afret use)
	/// </summary>
	using DirectX::XMVECTOR;
	using DirectX::XMFLOAT4;
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT2;

	struct Vertex {
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT2 texCoord;
	};

	const D3D12_INPUT_ELEMENT_DESC VERTEX_ELEMENT_DESC[] = { 
												{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
												{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
												{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0} };

	const D3D12_INPUT_LAYOUT_DESC VERTEX_LAYOUT_DESC = { VERTEX_ELEMENT_DESC , sizeof(VERTEX_ELEMENT_DESC)/sizeof(D3D12_INPUT_ELEMENT_DESC)};

	extern ComPtr<ID3D12Resource> cubeDefaultBuffer;
	extern D3D12_VERTEX_BUFFER_VIEW cubeView;
	static ComPtr<ID3D12Resource> cubeDefaultIndexBuffer;
	static D3D12_INDEX_BUFFER_VIEW cubeIndexView;

	/*
	const Vertex CUBE_VERTECES[] = { 
	{XMFLOAT4(-1.0f,-1.0f,-1.0f,1.0f),XMFLOAT4(DirectX::Colors::Aqua)},
	{XMFLOAT4(-1.0f,1.0f,-1.0f,1.0f),XMFLOAT4(DirectX::Colors::Yellow)},
	{XMFLOAT4(1.0f,1.0f,-1.0f,1.0f),XMFLOAT4(DirectX::Colors::Green)},
	{XMFLOAT4(1.0f,-1.0f,-1.0f,1.0f),XMFLOAT4(DirectX::Colors::Red)},
	{XMFLOAT4(-1.0f,-1.0f,1.0f,1.0f),XMFLOAT4(DirectX::Colors::Purple)},
	{XMFLOAT4(-1.0f,1.0f,1.0f,1.0f),XMFLOAT4(DirectX::Colors::Black)},
	{XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT4(DirectX::Colors::HotPink)},
	{XMFLOAT4(1.0f,-1.0f,1.0f,1.0f),XMFLOAT4(DirectX::Colors::LightBlue)},
	};
	*/

	const uint16_t CUBE_INDEXCES[] = {
		// front face
	0, 1, 2,
	0, 2, 3,
	// back face
	4, 6, 5,
	4, 7, 6,
	// left face
	4, 5, 1,
	4, 1, 0,
	// right face
	3, 2, 6,
	3, 6, 7,
	// top face
	1, 5, 6,
	1, 6, 2,
	// bottom face
	4, 0, 3,
	4, 3, 7

	};

	/*
	const Vertex PIRAMID_VERTECES[] = {
	{XMFLOAT4(-1.0f,0.0f,-1.0f,1.0f),XMFLOAT4(DirectX::Colors::Aqua)},
	{XMFLOAT4(1.0f,0.0f,-1.0f,1.0f),XMFLOAT4(DirectX::Colors::Red)},
	{XMFLOAT4(-1.0f,0.0f,1.0f,1.0f),XMFLOAT4(DirectX::Colors::Crimson)},
	{XMFLOAT4(1.0f,0.0f,1.0f,1.0f),XMFLOAT4(DirectX::Colors::Lavender)},
	{XMFLOAT4(0.0f,1.0f,0.0f,1.0f),XMFLOAT4(DirectX::Colors::Snow)},
	};
	*/

	const uint16_t PIRAMID_INDEXCES[] = {
		//base
		0,1,2,   1,3,2,
		//right
		1,4,3,
		//back
		3,4,2,
		//left
		2,4,0,
		//front
		0,4,1,
	};


	struct TransformMatrix {
		DirectX::XMFLOAT4X4 matrix;
		float currentTime;
		float padding[3];
	};

	static TransformMatrix cubeMatrix;
	static Buffers::RootSignature* rootSignature;
	static Control::PipelineObject* mPSO;
	static Control::PipelineObject* mPSO2;
	static Buffers::ShaderByteCodes shaders;
	static Buffers::ConstantBuffer* objectBuffer;

	static int CUBE_SIZE = 30;

	extern float dxCamera;
	extern float dyCamera;
	extern float dzCamera;

	static Timer animTimer{};

	static Graphics::Object3D* cubeObject;
	static Graphics::Object3D* pirObject;
	static Graphics::Object3D* conusObject;

	static Graphics::RenderObject3D* cubeRenderingObject;
	static Graphics::RenderObject3D* cubeRenderingObject2;
	static Graphics::RenderObject3D* pirRenderingObject;
	static Graphics::RenderObject3D* conusRenderingObject;
	static Graphics::RenderObject3D* gridRenderingObject;
	static std::vector<Graphics::RenderObject3D*> vecRenderingObject;

	static Control::RenderingFlow* renderFlow;
	extern Graphics::Camera* camera;

	static Materials::TextureFactory* texFactory;

	void VertexTransformData(void* vertex);


	inline void InitTest(GPUAdapter* adapter, GINS::WindowResource* window) {
		ID3D12Device* device = adapter->GetDevice();
		ID3D12GraphicsCommandList* commandList = adapter->GetCommandList();

		std::vector<Buffers::DataInfo> dataToBuffer;
		dataToBuffer.push_back({ sizeof(TransformMatrix),1,&cubeMatrix });
		objectBuffer = new Buffers::ConstantBuffer{ adapter,dataToBuffer };

		//D3D12_ROOT_PARAMETER rootParam = Buffers::RootSignature::CreateDescriptorTableWithSingleDescriptorHeap(1, 0);
		std::vector<D3D12_ROOT_PARAMETER> rootParams = Control::RenderingFlow::GetRootParams();
		rootSignature = new Buffers::RootSignature(adapter, rootParams.size(), &rootParams[0]);

		Buffers::Shader VS{ L"VertexShader.hlsl","main","vs_5_0"};
		VS.SetByteCode(&shaders.VS);
		Buffers::Shader PS{ L"PixelShader.hlsl","main","ps_5_0" };
		PS.SetByteCode(&shaders.PS);
		mPSO = Control::PipelineObject::CreateDefaultPSO(adapter, *rootSignature, shaders, VERTEX_LAYOUT_DESC, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			1, &GPUAdapter::bkBuffFormat, WindowResource::DEPTH_STENCIL_FORMAT, adapter->GetMultiSample());
		mPSO2 = Control::PipelineObject::CreateDefaultPSO(adapter, *rootSignature, shaders, VERTEX_LAYOUT_DESC, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			1, &GPUAdapter::bkBuffFormat, WindowResource::DEPTH_STENCIL_FORMAT, adapter->GetMultiSample(),TRUE);
		animTimer.Reset();

		//std::vector<Graphics::InputLayoutInfo> cubeInfo;
		//cubeInfo.push_back({ sizeof(CUBE_VERTECES),Graphics::DataLocation::DEFAULT_HEAP });
		//cubeInfo.push_back({sizeof(CUBE_INDEXCES),Graphics::DataLocation::DEFAULT_HEAP});
		//cubeObject = new Graphics::Object3D(adapter, cubeInfo.size(), &cubeInfo[0]);
		//cubeObject->AllocateMemory();
		//cubeObject->InitBuffers({ 0,1 }, { (void*)CUBE_VERTECES,(void*)CUBE_INDEXCES });
		//cubeIndexView = cubeObject->GetIndexBufferView(1, DXGI_FORMAT_R16_UINT);
		//cubeView = cubeObject->GetVertexBufferView(0,8);
		//cubeView.StrideInBytes = sizeof(Vertex);
		//std::vector<Graphics::InputLayoutInfo> pirInfo;
		//pirInfo.push_back({ sizeof(PIRAMID_VERTECES),Graphics::DataLocation::DEFAULT_HEAP });
		//pirInfo.push_back({ sizeof(PIRAMID_INDEXCES),Graphics::DataLocation::DEFAULT_HEAP });
		//pirObject = new Graphics::Object3D(adapter, pirInfo.size(), &pirInfo[0]);
		//pirObject->AllocateMemory();
		//pirObject->InitBuffers({ 0,1 }, { (void*)PIRAMID_VERTECES,(void*)PIRAMID_INDEXCES });

		camera = new Graphics::Camera{ 100,100,100,0,0,0 };
		texFactory = new Materials::TextureFactory(adapter);
		Materials::TextureFactory::texture_index mapTexIndex = texFactory->RegisterTexture(L"map.dds");
		Materials::TextureFactory::texture_index mapTexIndex2 = texFactory->RegisterTexture(L"map2.dds");

		renderFlow = new Control::RenderingFlow(adapter, nullptr, nullptr, rootSignature, window,camera, texFactory);
		//cubeRenderingObject = new Graphics::RenderObject3D(cubeObject, 0, 1, DXGI_FORMAT_R16_UINT, 8, 36, mPSO, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//cubeRenderingObject->SetObjectSize(20);
		//cubeRenderingObject->SetObjectTranslate(150, -100, -100);
		//cubeRenderingObject2 = new Graphics::RenderObject3D(cubeObject, 0, 1, DXGI_FORMAT_R16_UINT, 8, 36, mPSO, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//cubeRenderingObject2->SetObjectSize(50);
		//pirRenderingObject = new Graphics::RenderObject3D(pirObject, 0, 1, DXGI_FORMAT_R16_UINT, 5, 18, mPSO, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//pirRenderingObject->SetObjectSize(40);
		//pirRenderingObject->SetObjectTranslate(-150, -100, -100);

		Graphics::MeshConstructInputInfo meshInfo;
		meshInfo.vertex.heapLocation = Graphics::DataLocation::DEFAULT_HEAP;
		meshInfo.vertex.sizeInBytes = sizeof(Vertex);
		meshInfo.vertexLayout.positionOffsetBytes = 0;
		meshInfo.vertexLayout.positionSizeBytes = sizeof(Vertex::position);
		meshInfo.vertexLayout.normalSizeBytes = sizeof(Vertex::normal);
		meshInfo.vertexLayout.normalOffsetBytes = sizeof(Vertex::position);
		meshInfo.vertexLayout.texCoordOffsetBytes = sizeof(Vertex::position) + sizeof(Vertex::normal);
		meshInfo.indexes.heapLocation = Graphics::DataLocation::DEFAULT_HEAP;
		meshInfo.indexes.sizeInBytes = 2;
		//meshInfo.vertexFunction = VertexTransformData;
		Graphics::MeshOutputInfo outInfo;
		//conusObject = Graphics::CreateConusMesh(adapter, meshInfo, 0.5, 0.2, 5, 40, &outInfo);
		conusObject = Graphics::CreateSphereMesh(adapter, meshInfo, 1, 4, &outInfo);
		//conusObject = Graphics::CreateGridMesh(adapter, meshInfo, 10,10, &outInfo);
		conusRenderingObject = new Graphics::RenderObject3D(conusObject, 0, 1, DXGI_FORMAT_R16_UINT, outInfo.vertexCount, outInfo.indexCount, mPSO, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gridRenderingObject = new Graphics::RenderObject3D(conusObject, 0, 1, DXGI_FORMAT_R16_UINT, outInfo.vertexCount, outInfo.indexCount, mPSO, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		conusRenderingObject->SetObjectSize(20);
		gridRenderingObject->SetObjectSize(10);
		gridRenderingObject->SetObjectTranslate(40, 0, 0);
		//gridRenderingObject->SetObjectScale(30,30,30);
		//gridRenderingObject->SetObjectRotate(DirectX::XM_PI, -DirectX::XM_PIDIV2, 0);
		//gridRenderingObject->SetObjectTranslate(40, 40, 40);
		//conusRenderingObject->SetObjectRotate(0, 0, DirectX::XM_PIDIV4);
		
		conusRenderingObject->SetAmbientLight({ 0.1, 0.1, 0.1 });
		Materials::Material conusMaterial({ 1.0, 1.0, 1.0 }, { 0.7, 0.7, 0.7 }, 1.0f);
		conusRenderingObject->SetMaterial(conusMaterial);
		Light::ParallelLight* light1 = new Light::ParallelLight({ 0.5, 0.5, 0.5 }, { 1.0, 1.0, 1.0 });
		Light::ParallelLight* light2 = new Light::ParallelLight({ 0.3, 0.3, 0.3 }, { -1.0,1.0,-1.0 });
		Light::ParallelLight* light3 = new Light::ParallelLight({ 0.3, 0.3, 0.3 }, { 0.0,1.0,0.0 });
		conusRenderingObject->AddLightResources({ light1 });
		gridRenderingObject->AddLightResources({ light1,light2,light3 });
		gridRenderingObject->SetAmbientLight({ 0.6, 0.6, 0.6 });
		Materials::Material gridMaterial({ 0.5, 0.2, 0.55 }, {1.0, 1.0, 1.0}, 1.0f);
		gridRenderingObject->SetMaterial(gridMaterial);

		conusRenderingObject->SetTextureIndex(mapTexIndex2);
		gridRenderingObject->SetTextureIndex(mapTexIndex2);

		srand(time(0));
		for (int i = 0; i < 10; i++) {
			vecRenderingObject.push_back(new Graphics::RenderObject3D(conusObject, 0, 1, DXGI_FORMAT_R16_UINT, outInfo.vertexCount, outInfo.indexCount, mPSO, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
			vecRenderingObject.back()->SetObjectSize(rand() % 10 + 10);
			vecRenderingObject.back()->SetObjectTranslate(rand() % 200-100, rand() % 200-100, rand() % 200-100);
			vecRenderingObject.back()->SetAmbientLight({ rand() % 1000 / 1000.0f,rand() % 1000 / 1000.0f ,rand() % 1000 / 1000.0f });
			vecRenderingObject.back()->SetMaterial(Materials::Material{ {1.0,1.0,1.0},{1.0,1.0,1.0},1.0 });
			vecRenderingObject.back()->AddLightResources({ light1 });
			vecRenderingObject.back()->SetTextureIndex(mapTexIndex);
		}
	}





	inline void TestDrawFunction(WindowResource& window) {
		/*
		HRESULT hRes;
		using D3D12_NS::TranslateResourceBarrier;
		GPUAdapter& adapter = window.GetGPUAdapter();
		ID3D12GraphicsCommandList* commandList = adapter.GetCommandList();

		DirectX::XMMATRIX matrix = DirectX::XMMatrixScaling(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
		//matrix = DirectX::XMMatrixMultiply(matrix, DirectX::XMMatrixRotationZ(0.5));
		XMVECTOR pos = DirectX::XMVectorSet(dxCamera, dyCamera, dzCamera, 1.0f);
		XMVECTOR target = DirectX::XMVectorZero();
		XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);//меняется только при изменении разрешения таргета
		matrix = DirectX::XMMatrixMultiply(matrix, V);
		matrix = DirectX::XMMatrixMultiply(matrix, DirectX::XMMatrixPerspectiveFovLH(0.25 * DirectX::XM_PI, 800.0 / 600.0, 1, 1000));

		//DirectX::XMMATRIX m = DirectX::XMMatrixSet(0.5f, -0.5f, -0.99f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		//m = DirectX::XMMatrixMultiply(m, matrix);
		//m *= (1.0/m.r[0].m128_f32[3]);
		//matrix = DirectX::XMMatrixIdentity();

		matrix = DirectX::XMMatrixTranspose(matrix);////!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
		DirectX::XMStoreFloat4x4(&cubeMatrix.matrix, matrix);

		static float dt = 0;
		dt += animTimer.Tick();
		cubeMatrix.currentTime = dt;

		objectBuffer->CopyData(&cubeMatrix, sizeof(cubeMatrix));
		adapter.StartMainDraw();

		commandList->SetPipelineState(*cubeRenderingObject->GetCurrentPSO());
		commandList->SetGraphicsRootSignature(*rootSignature);
		ID3D12DescriptorHeap* descHeaps[1]; descHeaps[0] = objectBuffer->GetDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descHeaps);
		commandList->SetGraphicsRootDescriptorTable(0, descHeaps[0]->GetGPUDescriptorHandleForHeapStart());

		TranslateResourceBarrier(commandList, window.GetCurrentBkBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		window.SetViewportToFullWindow(commandList);
		window.SetScissorRectToFullWindow(commandList);
		commandList->ClearRenderTargetView(window.GetCurrentBkBufferView(), DirectX::Colors::Indigo, 0, nullptr);
		commandList->ClearDepthStencilView(window.GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);
		const D3D12_CPU_DESCRIPTOR_HANDLE bkBuffHandle = window.GetCurrentBkBufferView();
		const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle = window.GetDepthStencilView();
		commandList->OMSetRenderTargets(1, &bkBuffHandle, TRUE, &depthStencilHandle);


		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cubeView = cubeRenderingObject->GetVBV();
		cubeIndexView = cubeRenderingObject->GetIBV();
		commandList->IASetVertexBuffers(0, 1, &cubeView);
		commandList->IASetIndexBuffer(&cubeIndexView);
		//commandList->DrawInstanced(10, 1, 0, 0);
		//commandList->DrawIndexedInstanced(sizeof(CUBE_INDEXCES)/sizeof(uint16_t), 1, 0, 0, 0);
		commandList->DrawIndexedInstanced(sizeof(CUBE_INDEXCES)/sizeof(uint16_t), 1, 0, 0, 0);

		TranslateResourceBarrier(commandList, window.GetCurrentBkBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		hRes = commandList->Close();
		ID3D12CommandList* cmdListsArray[] = { commandList };
		adapter.GetCommandQueue()->ExecuteCommandLists(1, cmdListsArray);
		window.PresentBuffer();
		adapter.FlushCommandQueue();
		//adapter.StartMainDraw();
		*/
		static HANDLE eventOnGPU = CreateEvent(NULL, TRUE, FALSE, NULL);
		std::vector<Graphics::RenderObject3D*> objects;
		//objects.push_back(cubeRenderingObject);
		//objects.push_back(cubeRenderingObject2);
		//objects.push_back(pirRenderingObject);
		objects.push_back(conusRenderingObject);
		objects.push_back(gridRenderingObject);
		for (auto x : vecRenderingObject) {
			//x->SetObjectTranslate(rand() % 200 - 100, rand() % 200 - 100, rand() % 200 - 100);
			x->SetObjectRotate(rand() % 1000 / 1000.0f, rand() % 1000 / 1000.0f, rand() % 1000 / 1000.0f);
			objects.push_back(x);
		}

		renderFlow->StartDraw(&objects, eventOnGPU);
		WaitForSingleObject(eventOnGPU,INFINITE);
	}
}