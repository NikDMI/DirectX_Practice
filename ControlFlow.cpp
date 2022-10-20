#include "ControlFlow.h"
#include "ConfigShaders.h"
#include "DXGIh.h"

namespace Control {//PSO

	PipelineObject::PipelineObject(D3D12_NS::GPUAdapter* adapter, ID3D12RootSignature* rootSignature, Buffers::ShaderByteCodes byteCodes,
		D3D12_STREAM_OUTPUT_DESC streamOutput, D3D12_BLEND_DESC blendState, UINT sampleMask, D3D12_RASTERIZER_DESC rasterizerState,
		D3D12_DEPTH_STENCIL_DESC depthStencilState, D3D12_INPUT_LAYOUT_DESC inputLayoutDesc, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibsCutValue,
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, UINT numRenderTargets, const DXGI_FORMAT* rtFormats,
		DXGI_FORMAT dsvFormat, DXGI_SAMPLE_DESC sampleDesc, D3D12_CACHED_PIPELINE_STATE cachedPSO,
		D3D12_PIPELINE_STATE_FLAGS flags) :
		adapter{ adapter } {

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		psoDesc.pRootSignature = rootSignature;
		psoDesc.VS = byteCodes.VS;
		psoDesc.HS = byteCodes.HS;
		psoDesc.DS = byteCodes.DS;
		psoDesc.GS = byteCodes.GS;
		psoDesc.PS = byteCodes.PS;
		psoDesc.StreamOutput = streamOutput;
		psoDesc.BlendState = blendState;
		psoDesc.SampleMask = sampleMask;
		psoDesc.RasterizerState = rasterizerState;
		psoDesc.DepthStencilState = depthStencilState;
		psoDesc.InputLayout = inputLayoutDesc;
		psoDesc.IBStripCutValue = ibsCutValue;
		psoDesc.PrimitiveTopologyType = primitiveTopology;
		psoDesc.NumRenderTargets = numRenderTargets;
		for (int i = 0; i < numRenderTargets; ++i) psoDesc.RTVFormats[i] = rtFormats[i];
		for (int i = numRenderTargets; i < 8; ++i) psoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		psoDesc.DSVFormat = dsvFormat;
		psoDesc.SampleDesc = sampleDesc;
		psoDesc.NodeMask = 0;
		psoDesc.CachedPSO = cachedPSO;
		psoDesc.Flags = flags;
		adapter->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(mPSO.GetAddressOf()));
	}


	PipelineObject* PipelineObject::CreateDefaultPSO(D3D12_NS::GPUAdapter* adapter, ID3D12RootSignature* rootSignature, Buffers::ShaderByteCodes byteCodes,
		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, UINT numRenderTargets, const DXGI_FORMAT* rtFormats,
		DXGI_FORMAT dsvFormat, DXGI_SAMPLE_DESC sampleDesc, BOOL FrontCounterClockwise) {

		static D3D12_STREAM_OUTPUT_DESC streamOutput{};
		D3D12_BLEND_DESC blendState;
		blendState.AlphaToCoverageEnable = FALSE;
		blendState.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			blendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

		D3D12_RASTERIZER_DESC rasterizerState =
		{
			D3D12_FILL_MODE_SOLID,D3D12_CULL_MODE_BACK, FrontCounterClockwise,
			0, 0, 0, true, false,false,0, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		D3D12_DEPTH_STENCIL_DESC depthStencilState;
		depthStencilState.DepthEnable = TRUE;
		depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilState.StencilEnable = FALSE;
		depthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		depthStencilState.FrontFace = defaultStencilOp;
		depthStencilState.BackFace = defaultStencilOp;

		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE ibsCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//???
		D3D12_CACHED_PIPELINE_STATE cachedPSO = { nullptr,0 };
		PipelineObject* pso = new PipelineObject(adapter, rootSignature, byteCodes, streamOutput, blendState,
			UINT_MAX, rasterizerState, depthStencilState, inputLayoutDesc, ibsCutValue, primitiveTopology, numRenderTargets,
			rtFormats, dsvFormat, sampleDesc, cachedPSO, D3D12_PIPELINE_STATE_FLAG_NONE);
		return pso;
	}

}


namespace Control {//RenderingFlow
	DWORD WINAPI RenderingThreadProc(LPVOID param);

	D3D12_ROOT_PARAMETER RenderingFlow::GetTableObjectRootParameter() {
		return Buffers::RootSignature::CreateDescriptorTableWithSingleDescriptorHeap(1, OBJECT_BUFFER_B_);
	}

	D3D12_ROOT_PARAMETER RenderingFlow::GetDirectHanldeLightRootParameter() {
		return Buffers::RootSignature::CreateRootDescriptor(LIGHT_BUFFER_B_, 0, Buffers::RootSignature::RootDescriptorTypes::CBV);
	}

	D3D12_ROOT_PARAMETER RenderingFlow::GetTableCommonDataRootParameter() {
		return Buffers::RootSignature::CreateDescriptorTableWithSingleDescriptorHeap(1, COMMON_BUFFER_B_);
	}

	D3D12_ROOT_PARAMETER RenderingFlow::GetTableTextureRootParameter() {
		return Buffers::RootSignature::CreateDescriptorTableWithSingleDescriptorHeap(1, TEXTURE_REG_INDEX, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	}


	std::vector<D3D12_ROOT_PARAMETER> RenderingFlow::GetRootParams() {
		std::vector<D3D12_ROOT_PARAMETER> params;
		params.push_back(RenderingFlow::GetTableObjectRootParameter());
		params.push_back(RenderingFlow::GetDirectHanldeLightRootParameter());
		params.push_back(RenderingFlow::GetTableCommonDataRootParameter());
		params.push_back(RenderingFlow::GetTableTextureRootParameter());
		return params;
	}

	RenderingFlow::RenderingFlow(D3D12_NS::GPUAdapter* adapter, ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator,
		Buffers::RootSignature* rootSignature, GINS::WindowResource* targetWindow, Graphics::Camera* camera,
		Materials::TextureFactory* textureFactory) :adapter{ adapter }, rootSignature{ rootSignature },
		targetWindow{ targetWindow }, camera{ camera }, textureFactory{textureFactory}
	{
		if (adapter == nullptr || rootSignature == nullptr || targetWindow == nullptr || camera == nullptr) throw Exception("REnderFlow: bad arguments");
		if (commandAllocator == nullptr) {
			adapter->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(this->commandAllocator.GetAddressOf()));
		}
		if (commandList == nullptr) {
			adapter->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->commandAllocator.Get(), nullptr, IID_PPV_ARGS(this->commandList.GetAddressOf()));
			this->commandList->Close();
			ID3D12CommandList* commandLists[] = { this->commandList.Get() };
			adapter->GetCommandQueue()->ExecuteCommandLists(1, commandLists);
			adapter->FlushCommandQueue();
		}
		threadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (threadEvent == INVALID_HANDLE_VALUE) throw Exception("RenderingFlow: fatal error - event creation falled");
		//allocate buffers for rendering objects
		std::vector<Buffers::DataInfo> bufferInfo;
		bufferInfo.push_back({sizeof(RenderObjectInfo) * CBV_TABLE_B1_SIZE,1,nullptr});
		objectRegisterBuffer = new Buffers::ConstantBuffer(adapter, bufferInfo);
		objectBuffersDescriptorHeap = Buffers::CreateDescriptorHeap(adapter->GetDevice(), CBV_TABLE_B1_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		D3D12_GPU_VIRTUAL_ADDRESS startOfBuffer = objectRegisterBuffer->GetGPUVirtualAddress();
		D3D12_CPU_DESCRIPTOR_HANDLE startDescHandle = objectBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		for (int i = 0; i < RenderingFlow::CBV_TABLE_B1_SIZE; ++i) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = startOfBuffer + sizeof(RenderObjectInfo) * i;
			cbvDesc.SizeInBytes = sizeof(RenderObjectInfo);
			adapter->GetDevice()->CreateConstantBufferView(&cbvDesc, startDescHandle);
			startDescHandle.ptr += adapter->GetCbvSrvHandleSize();
		}
		//buffer for frame data
		bufferInfo.clear();
		bufferInfo.push_back({ sizeof(FrameRenderingInfo),1,nullptr });
		commonDataRegisterBuffer = new Buffers::ConstantBuffer(adapter, bufferInfo);
		commonDataBuffersDescriptorHeap = Buffers::CreateDescriptorHeap(adapter->GetDevice(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		startOfBuffer = commonDataRegisterBuffer->GetGPUVirtualAddress();
		startDescHandle = commonDataBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = startOfBuffer;
		cbvDesc.SizeInBytes = sizeof(FrameRenderingInfo);
		adapter->GetDevice()->CreateConstantBufferView(&cbvDesc, startDescHandle);
		//buffer for light data
		bufferInfo.clear();
		bufferInfo.push_back({ sizeof(LightSourcesInfo) * CBV_TABLE_B1_SIZE,1,nullptr });//data for all objects(if all of them have different lights)
		lightRegisterBuffer = new Buffers::ConstantBuffer(adapter, bufferInfo);
		//create control elements
		adapter->GetDevice()->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
		fenceEventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (fenceEventHandle == INVALID_HANDLE_VALUE) throw Exception("RenderingFlow: fatal error - event creation falled");

		threadHandle = CreateThread(NULL, 0, RenderingThreadProc, this, 0, NULL);
		if (threadHandle == INVALID_HANDLE_VALUE) throw Exception("RenderingFlow: fatal error - thread creation falled");
	}


	void RenderingFlow::StartDraw(std::vector<Graphics::RenderObject3D*>* objects, HANDLE eventOnEndCommandList) {
		//fence signaled in the start of each transaction and in te last transaction signals twice
		if (objects != nullptr) {
			int numberOfTransactions = ceil(objects->size() / (float)RenderingFlow::CBV_TABLE_B1_SIZE);
			endFenceValue = fenceValue + numberOfTransactions;// fence value, when gpu performs ALL render actions
		}
		else {
			endFenceValue = fenceValue + 1;
		}
		if (eventOnEndCommandList != INVALID_HANDLE_VALUE) {
			ResetEvent(eventOnEndCommandList);
			fence->SetEventOnCompletion(endFenceValue, eventOnEndCommandList);//flush
		}
		isFirstFill = true;
		objectsToDraw = objects;
		currentActivity = ThreadActivities::FillCommandQueue;
		SetEvent(this->threadEvent);
	}

	bool SetLightData(const std::vector<Light::SourceLight*>& previousLights, const std::vector<Light::SourceLight*>& currentLights,
		void* addressToWrite);//true - if changed lights info

	DWORD WINAPI RenderingThreadProc(LPVOID param) {
		RenderingFlow* renderingFlow = (RenderingFlow*)param;
		HANDLE clockEvent = renderingFlow->threadEvent;
		ID3D12GraphicsCommandList* commandList = renderingFlow->commandList.Get();
		ID3D12CommandQueue* commandQueue = renderingFlow->adapter->GetCommandQueue();
		ID3D12Fence* fence = renderingFlow->fence.Get();
		GINS::WindowResource* windowTarget = renderingFlow->targetWindow;
		std::vector<Graphics::RenderObject3D*>* objectsToDraw;
		unsigned int drawedObjectsCount;

		UINT cbvSbvHandleSize = renderingFlow->adapter->GetCbvSrvHandleSize();

		HANDLE eventOnStep = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (eventOnStep == INVALID_HANDLE_VALUE) throw Exception("RenderingFlowThread: fatal error - event creation falled");
		while (true) {
			WaitForSingleObject(clockEvent, INFINITE);//wait for commands
			ResetEvent(clockEvent);//

			switch (renderingFlow->currentActivity) {

			case RenderingFlow::ThreadActivities::FillCommandQueue:
				objectsToDraw = renderingFlow->objectsToDraw;
				drawedObjectsCount = 0;
				SetEvent(eventOnStep);
				//compute frame info
				{
					FrameRenderingInfo frameInfo;
					//viewProjMatrix
					DirectX::XMMATRIX viewProjMatrix = renderingFlow->camera->GetViewMatrix();
					viewProjMatrix = viewProjMatrix * DirectX::XMMatrixPerspectiveFovLH(0.25 * DirectX::XM_PI, windowTarget->GetRatio(), 1, 1000);
					viewProjMatrix = DirectX::XMMatrixTranspose(viewProjMatrix);
					DirectX::XMStoreFloat4x4(&frameInfo.viewProjMatrix, viewProjMatrix);
					//eyePoint
					DirectX::XMStoreFloat4(&frameInfo.eyePoint, renderingFlow->camera->GetEyePoint());
					//
					void* frameInfoAddr = renderingFlow->commonDataRegisterBuffer->GetMappedAddress();
					memcpy(frameInfoAddr, &frameInfo, sizeof(frameInfo));
				}
				//
				while (true) {//rendering
					WaitForSingleObject(eventOnStep,INFINITE);
					//ResetEvent(eventOnStep);//здесь можно распараллелить на несколько аллокаторов, чтобы не ждать одного!!!
					renderingFlow->commandAllocator->Reset();
					commandList->Reset(renderingFlow->commandAllocator.Get(), nullptr);
					if (renderingFlow->textureFactory) renderingFlow->textureFactory->ResetRootTextures();

					windowTarget->SetViewportToFullWindow(commandList);
					windowTarget->SetScissorRectToFullWindow(commandList);
					const D3D12_CPU_DESCRIPTOR_HANDLE bkBuffHandle = windowTarget->GetCurrentBkBufferView();
					const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle = windowTarget->GetDepthStencilView();
					commandList->OMSetRenderTargets(1, &bkBuffHandle, TRUE, &depthStencilHandle);

					if (renderingFlow->isFirstFill) {//init commands
						D3D12_NS::TranslateResourceBarrier(commandList, windowTarget->GetCurrentBkBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
						commandList->ClearRenderTargetView(bkBuffHandle, DirectX::Colors::Indigo, 0, nullptr);
						commandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);
						renderingFlow->isFirstFill = false;
					}

					commandList->SetGraphicsRootSignature(*renderingFlow->rootSignature);
					ID3D12DescriptorHeap* bufferHeaps[] = { renderingFlow->commonDataBuffersDescriptorHeap.Get() };
					commandList->SetDescriptorHeaps(1, bufferHeaps);
					//set FRAME info to shaders
					commandList->SetGraphicsRootDescriptorTable(2, renderingFlow->commonDataBuffersDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
					//register object descriptor heap
					bufferHeaps[0] = renderingFlow->objectBuffersDescriptorHeap.Get();
					commandList->SetDescriptorHeaps(1, bufferHeaps);

					//add rendering objects to command list
					void* constantBufferAddress = renderingFlow->objectRegisterBuffer->GetMappedAddress();
					void* ligthSourceBufferAddressCPU = renderingFlow->lightRegisterBuffer->GetMappedAddress();//current address to write light
					D3D12_GPU_VIRTUAL_ADDRESS ligthSourceBufferAddressGPU = renderingFlow->lightRegisterBuffer->GetGPUVirtualAddress();
					RenderObjectInfo objectInfo; LightSourcesInfo lightInfo;
					D3D12_GPU_DESCRIPTOR_HANDLE descriptorHeapAddress = renderingFlow->objectBuffersDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
					if (objectsToDraw != nullptr) {//лищняя проверка...
						//additional info
						unsigned int renderObjectsCount = (objectsToDraw->size() - drawedObjectsCount) >= RenderingFlow::CBV_TABLE_B1_SIZE ? RenderingFlow::CBV_TABLE_B1_SIZE : objectsToDraw->size() - drawedObjectsCount;
						unsigned int objectIndex = drawedObjectsCount;
						Control::PipelineObject* currentPSO = nullptr;
						std::vector<Light::SourceLight*> previousObjectLights(MAX_SOURCE_LIGHT_NUMBER + 1);
						//cycle throught objects
						for (int i = 0; i < renderObjectsCount; ++i,++objectIndex) {//set commands for all objects
							Graphics::RenderObject3D* currentRenderObject = (*objectsToDraw)[objectIndex];
							Control::PipelineObject* objectPSO = currentRenderObject->GetCurrentPSO();
							if (currentPSO != objectPSO) {
								currentPSO = objectPSO;
								commandList->SetPipelineState(*currentPSO);
							}
							commandList->IASetPrimitiveTopology(currentRenderObject->GetPrimitiveTopology());
							//SET object data to registers
							//1)transformation info
							DirectX::XMMATRIX worldMatrix = *currentRenderObject->GetWorldMatrix();
							DirectX::XMStoreFloat4x4(&objectInfo.worldMatrix, worldMatrix);
							DirectX::XMStoreFloat3(&objectInfo.ambientLight, currentRenderObject->GetAmbientColor());
							currentRenderObject->CopyMaterialInfo(&objectInfo.material);
							memcpy(constantBufferAddress, &objectInfo, sizeof(objectInfo));
							commandList->SetGraphicsRootDescriptorTable(0, descriptorHeapAddress);
							//light info
							if (SetLightData(previousObjectLights, currentRenderObject->GetLightsVector(), ligthSourceBufferAddressCPU)) {
								commandList->SetGraphicsRootConstantBufferView(1, ligthSourceBufferAddressGPU);
								ligthSourceBufferAddressCPU = (char*)ligthSourceBufferAddressCPU + sizeof(LightSourcesInfo);
								ligthSourceBufferAddressGPU += sizeof(LightSourcesInfo);
								previousObjectLights = currentRenderObject->GetLightsVector();
							}
							//DRAW OBJECTS
							if (currentRenderObject->DrawIndexInstance(commandList, renderingFlow->textureFactory)) {
								//register object descriptor heap
								bufferHeaps[0] = renderingFlow->objectBuffersDescriptorHeap.Get();
								commandList->SetDescriptorHeaps(1, bufferHeaps);
							}
							//nextObject
							constantBufferAddress = (char*)constantBufferAddress+sizeof(objectInfo);
							descriptorHeapAddress.ptr += cbvSbvHandleSize;
						}
						drawedObjectsCount += renderObjectsCount;
					}

					ID3D12CommandList* commandListsToExecute[] = { commandList };
					++(renderingFlow->fenceValue);

					if (renderingFlow->fenceValue < renderingFlow->endFenceValue) {//wait for executing step (because of 1 allocator! we need to create parallel work)
						commandList->Close();
						commandQueue->ExecuteCommandLists(1, commandListsToExecute);
						commandQueue->Signal(fence, renderingFlow->fenceValue);
						if (fence->GetCompletedValue() < renderingFlow->fenceValue) {
							ResetEvent(eventOnStep);
							fence->SetEventOnCompletion(renderingFlow->fenceValue, eventOnStep);
						}
					}
					else {
						D3D12_NS::TranslateResourceBarrier(commandList, windowTarget->GetCurrentBkBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
						commandList->Close();
						commandQueue->ExecuteCommandLists(1, commandListsToExecute);
						renderingFlow->targetWindow->PresentBuffer();
						commandQueue->Signal(fence, renderingFlow->fenceValue);
						break;//send all object to the gpu
					}
				}
				//here we should say outside envirioment that we push all data to gpu...
				break;

			case RenderingFlow::ThreadActivities::TerminateThread:
				return 0;
			}
		}
	}

	bool SetLightData(const std::vector<Light::SourceLight*>& previousLights, const std::vector<Light::SourceLight*>& currentLights,
		void* addressToWrite) {//true - if changed lights i
		bool isChanged = false;
		if (previousLights.size() == currentLights.size()) {
			for (int i = 0; i < previousLights.size(); ++i) {
				if (previousLights[i] != currentLights[i]) {
					isChanged = true;
					break;
				}
			}
		}
		else {
			isChanged = true;
		}
		if (isChanged) {
			std::vector<Light::SourceLight*> parallelLight, dotLight, cylinderLight;
			for (auto light : currentLights) {
				switch (light->GetType())
				{
				case Light::SourceLight::LightType::parallel:
					parallelLight.push_back(light);
					break;
				case Light::SourceLight::LightType::dot:
					dotLight.push_back(light);
					break;
				case Light::SourceLight::LightType::cylinder:
					cylinderLight.push_back(light);
					break;
				}
			}
			LightSourcesInfo* lightInfo = static_cast<LightSourcesInfo*>(addressToWrite);
			UINT index = 0;
			lightInfo->parallelLightSourceNumber = parallelLight.size();
			for (auto light : parallelLight) lightInfo->lights[index++] = light->GetLightHLSL();
			lightInfo->dotLightSourceNumber = dotLight.size();
			for (auto light : dotLight) lightInfo->lights[index++] = light->GetLightHLSL();
			lightInfo->cylinderLightSourceNumber = cylinderLight.size();
			for (auto light : cylinderLight) lightInfo->lights[index++] = light->GetLightHLSL();
		}
		return isChanged;
	}
}


