#include "Mesh.h"
#include "Buffer.h"
#include <map>
#include "MathFunc.h"

namespace Graphics {
	using D3D12_NS::GPUAdapter;

	Object3D::Object3D(GPUAdapter* adapter, const std::vector<InputLayoutInfo>& inputLayouts) :adapter{adapter}, numberOfInputLayouts { inputLayouts.size() } {
		if(adapter == nullptr) throw Exception("Object3D: nullptr adapter");
		if (numberOfInputLayouts > MAX_INPUT_LAYOUT_COUNT || numberOfInputLayouts == 0) throw Exception("Object3D: bad number of input layouts");
		this->inputLayouts = new InputLayoutInfo[numberOfInputLayouts];
		for (int i = 0; i < numberOfInputLayouts; ++i) {
			//uint32_t bufferSize = inputLayouts[i].sizeInBytes;
			uint32_t elementSize = inputLayouts[i].strideInBytes;
			if(elementSize == 0) throw Exception("Object3D: input LAyout info");
			//if (bufferSize < elementSize) throw Exception("Object3D: buffer's size is smaller than element's size");
			//if(bufferSize % elementSize != 0) throw Exception("Object3D: buffer's size doesn't moduled by elem size ");
			switch (inputLayouts[i].locationType) {
			case Graphics::DataLocation::DEFAULT_HEAP:
				if(inputLayouts[i].strideInBytes>Buffers::MAX_DEFAULT_HEAP_SIZE) throw Exception("Object3D: too large element's size");
				break;
			case Graphics::DataLocation::UPLOAD_HEAP:
				if (inputLayouts[i].strideInBytes > Buffers::MAX_UPLOAD_HEAP_SIZE) throw Exception("Object3D: too large element's size");
				break;
			}
			this->inputLayouts[i] = inputLayouts[i];
		}
	}

	//additional info consists of 1)index of layout 2)list of element count to 0,1,2.. parts of buffer
	void Object3D::AllocateMemory(std::vector<int> infoIndexes,std::vector<std::vector<uint32_t>> infoBuffers) {
		//if (inputLayoutsBuffers == nullptr) inputLayoutsBuffers = new Buffers::IBuffer * [numberOfInputLayouts];
		if(isAllocated)  throw Exception("Object3D: memory was allocated recently");
		isAllocated = true;
		auto infoIndexesIter = infoIndexes.begin(), infoIndexesIterEnd = infoIndexes.end();
		auto infoBufferIter = infoBuffers.begin(), infoBufferIterEnd = infoBuffers.end();

		std::vector<Buffers::DataInfo> dataInfo(1);
		for (int i = 0; i < numberOfInputLayouts; ++i) {
			uint32_t elementSize = inputLayouts[i].strideInBytes;
			int32_t elementCountToWrite = inputLayouts[i].elementCount;
			bool isAddedInfo = false;  std::vector<uint32_t>::iterator additionalInfoIter, additionalInfoIterEnd;
			if (infoIndexesIter != infoIndexesIterEnd && *infoIndexesIter == i) {//must see added information about buffers
				isAddedInfo = true;
				if(infoBufferIter == infoBufferIterEnd) throw Exception("Object3D: bad args");
				additionalInfoIter = (*infoBufferIter).begin();
				additionalInfoIterEnd = (*infoBufferIter).end();
				infoBufferIter++;
				infoIndexesIter++;
			}

			uint16_t elemCount;
			uint32_t maxElemCountInBuffer;
			switch (inputLayouts[i].locationType) {
			case DataLocation::DEFAULT_HEAP:
				maxElemCountInBuffer = Buffers::MAX_DEFAULT_HEAP_SIZE / elementSize;
				while (elementCountToWrite > 0 ) {
					if (!isAddedInfo || additionalInfoIter == additionalInfoIterEnd) {
						if (elementCountToWrite <= maxElemCountInBuffer) {
							elemCount = elementCountToWrite;
						}
						else {
							elemCount = maxElemCountInBuffer;
						}
					}
					else {
						elemCount = *(additionalInfoIter++);
					}
					uint16_t partBufferSize = elemCount * elementSize;
					if(partBufferSize > Buffers::MAX_DEFAULT_HEAP_SIZE) throw Exception("Object3D: large buffers");
					inputLayoutsBuffers[i].push_back(BufferInfo{ new Buffers::VertexBuffer(adapter, partBufferSize),elemCount});
					elementCountToWrite -= elemCount;
				}
				break;

			case  DataLocation::UPLOAD_HEAP:
				maxElemCountInBuffer = Buffers::MAX_UPLOAD_HEAP_SIZE / elementSize;
				while (elementCountToWrite > 0) {
					dataInfo.clear();
					if (!isAddedInfo || additionalInfoIter == additionalInfoIterEnd) {
						if (elementCountToWrite <= maxElemCountInBuffer) {
							elemCount = elementCountToWrite;
						}
						else {
							elemCount = maxElemCountInBuffer;
						}
					}
					else {
						elemCount = *(additionalInfoIter++);
					}
					uint16_t partBufferSize = elemCount * elementSize;
					if (partBufferSize > Buffers::MAX_UPLOAD_HEAP_SIZE) throw Exception("Object3D: large buffers");
					dataInfo.push_back({ partBufferSize , 1, nullptr });
					inputLayoutsBuffers[i].push_back(BufferInfo{ new Buffers::ConstantBuffer(adapter, dataInfo),elemCount });
					elementCountToWrite -= elemCount;
				}
				break;

			default:
				throw Exception("Object3D InitMemory: bad data location");
			}
		}
	}

	//init ALL memory
	void Object3D::InitBuffers(std::initializer_list<unsigned> inputIndexList, std::initializer_list<void*> dataList) {
		if(!isAllocated) throw Exception("Object3D: didn't allocate memory");
		std::initializer_list<void*>::iterator iterData = dataList.begin();
		for (auto& index : inputIndexList) {
			if(index>this->numberOfInputLayouts) throw Exception("Object3D: bad index number - initBuffer");
			uint32_t elementSize = inputLayouts[index].strideInBytes;
			std::vector<BufferInfo>& buffersInfo = inputLayoutsBuffers[index];
			char* data = (char*)*iterData;
			for (auto bufferInfo : buffersInfo) {
				unsigned int partSize = bufferInfo.elementCount * elementSize;
				bufferInfo.buffer->CopyData(data, partSize);
				data += partSize;
			}
			iterData++;
		}
	}

	/*
	D3D12_VERTEX_BUFFER_VIEW Object3D::GetVertexBufferView(unsigned index, UINT vertexCount) {
		if(index>=numberOfInputLayouts) throw Exception("Object3D: bad index int get vert buff view");
		if(!isAllocated)  throw Exception("Object3D: buffers did't allocated");
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = inputLayoutsBuffers[index][0].buffer->GetGPUVirtualAddress();
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = gpuAddress;
		vbv.SizeInBytes = inputLayouts[index].sizeInBytes;
		vbv.StrideInBytes = inputLayouts[index].sizeInBytes / vertexCount;
		return vbv;
	}
	*/

	D3D12_VERTEX_BUFFER_VIEW Object3D::GetVertexBufferView(unsigned index, INT startVertexIndex, UINT* vertexCountToTheEndOfBuffer) {
		if (index >= numberOfInputLayouts) throw Exception("Object3D: bad index int get vert buff view");
		if (!isAllocated)  throw Exception("Object3D: buffers did't allocated");
		if(startVertexIndex < 0) throw Exception("Object3D: bad start index");
		std::vector<BufferInfo>::iterator bufferInfoIter = inputLayoutsBuffers[index].begin(), bufferInfoIterEnd = inputLayoutsBuffers[index].end();
		while (bufferInfoIter != bufferInfoIterEnd) {
			startVertexIndex = startVertexIndex - (*bufferInfoIter).elementCount;
			if (startVertexIndex<0) break;
			bufferInfoIter++;
		}
		if (bufferInfoIter == bufferInfoIterEnd) throw Exception("GetVBV: vertexIndex is invalid");
		if (vertexCountToTheEndOfBuffer != nullptr) *vertexCountToTheEndOfBuffer = -startVertexIndex;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = (*bufferInfoIter).buffer->GetGPUVirtualAddress();
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = gpuAddress;
		vbv.SizeInBytes = (*bufferInfoIter).buffer->GetBufferSize();
		vbv.StrideInBytes = inputLayouts[index].strideInBytes;
		return vbv;
	}

	/*
	D3D12_INDEX_BUFFER_VIEW Object3D::GetIndexBufferView(unsigned index, DXGI_FORMAT format) {
		if (index >= numberOfInputLayouts) throw Exception("Object3D: bad index int get vert buff view");
		if (!isAllocated)  throw Exception("Object3D: buffers did't allocated");
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = inputLayoutsBuffers[index][0].buffer->GetGPUVirtualAddress();
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = gpuAddress;
		ibv.SizeInBytes = inputLayouts[index].sizeInBytes;
		ibv.Format = format;
		return ibv;
	}
	*/

	D3D12_INDEX_BUFFER_VIEW Object3D::GetIndexBufferView(unsigned index, INT indexNumber, UINT* indexCountToTheEndOfBuffer) {
		if (index >= numberOfInputLayouts) throw Exception("Object3D: bad index int get vert buff view");
		if (!isAllocated)  throw Exception("Object3D: buffers did't allocated");
		if (indexNumber < 0) throw Exception("Object3D: bad start index");
		std::vector<BufferInfo>::iterator bufferInfoIter = inputLayoutsBuffers[index].begin(), bufferInfoIterEnd = inputLayoutsBuffers[index].end();
		while (bufferInfoIter != bufferInfoIterEnd) {
			indexNumber = indexNumber - (*bufferInfoIter).elementCount;
			if (indexNumber < 0) break;
			bufferInfoIter++;
		}
		if (bufferInfoIter == bufferInfoIterEnd) throw Exception("GetVBV: vertexIndex is invalid");
		if (indexCountToTheEndOfBuffer != nullptr) *indexCountToTheEndOfBuffer = -indexNumber;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = (*bufferInfoIter).buffer->GetGPUVirtualAddress();
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = gpuAddress;
		ibv.SizeInBytes = (*bufferInfoIter).buffer->GetBufferSize();
		if (inputLayouts[index].strideInBytes == 2) {
			ibv.Format = DXGI_FORMAT_R16_UINT;
		}
		else if (inputLayouts[index].strideInBytes == 4) {
			ibv.Format = DXGI_FORMAT_R32_UINT;
		}
		else {
			throw Exception("GetIBV: bad index format");
		}
		return ibv;
	}

	uint32_t Object3D::GetMaxElementCountInBuffer(uint32_t elementSize, DataLocation locationType) {
		switch (locationType) {
		case DataLocation::DEFAULT_HEAP:
			return Buffers::MAX_DEFAULT_HEAP_SIZE / elementSize;
		case DataLocation::UPLOAD_HEAP:
			return Buffers::MAX_UPLOAD_HEAP_SIZE / elementSize;
		}
	}

}

namespace Graphics {

	using namespace DirectX;

	RenderObject3D::RenderObject3D(Object3D* objectBuffers, unsigned vbvIndex, unsigned ibvIndex, DXGI_FORMAT ibvFormat,UINT vertexCount, UINT indexCount,
		Control::PipelineObject* mPSO,D3D12_PRIMITIVE_TOPOLOGY primitiveTopology) :
		objectBuffers{ objectBuffers }, vertexBVIndex{ vbvIndex }, indexBVIndex{ ibvIndex }, vertexCount{ vertexCount }, indexCount{indexCount},
		ibvFormat{ ibvFormat }, mPSO{ mPSO }, primitiveTopology{primitiveTopology}
	{
		CreateDefaultWorldMatrix();
	}

	void RenderObject3D::CreateDefaultWorldMatrix() {
		XMMATRIX scale = XMMatrixScaling(sx, sy, sz);
		XMMATRIX rotate = XMMatrixRotationX(rx)* XMMatrixRotationY(ry)* XMMatrixRotationZ(rz);
		XMMATRIX translate = XMMatrixTranslation(dx, dy, dz);
		worldMatrix = scale * rotate * translate;
	}

	void RenderObject3D::SetObjectTranslate(float dx, float dy, float dz) {
		this->dx = dx;
		this->dy = dy;
		this->dz = dz;
		this->CreateDefaultWorldMatrix();
	}

	void RenderObject3D::SetObjectSize(int size) {
		this->sx = size;
		this->sy = size;
		this->sz = size;
		this->CreateDefaultWorldMatrix();
	}

	void RenderObject3D::SetObjectScale(float sx, float sy, float sz) {
		this->sx = sx;
		this->sy = sy;
		this->sz = sz;
		this->CreateDefaultWorldMatrix();
	}

	void RenderObject3D::SetObjectRotate(float xAngle, float yAngle, float zAngle) {
		this->rx = xAngle;
		this->ry = yAngle;
		this->rz = zAngle;
		this->CreateDefaultWorldMatrix();
	}

	void RenderObject3D::SetAmbientLight(DirectX::XMFLOAT3 lightPower) {
		this->ambientLight = lightPower;
	}

	void RenderObject3D::AddLightResources(std::initializer_list<Light::SourceLight*> lights) {
		for (auto& light : lights) {
			this->sourceLightVector.push_back(light);
		}
	}

	void RenderObject3D::SetMaterial(Materials::Material material) {
		this->material = material;
	}

	void RenderObject3D::CopyMaterialInfo(Materials::MaterialHLSL* dest) {
		material.CopyMaterial(dest);
	}

	void RenderObject3D::SetTextureIndex(Materials::TextureFactory::texture_index texIndex) {
		this->textureIndex = texIndex;
	}


	bool RenderObject3D::DrawIndexInstance(ID3D12GraphicsCommandList* cmdList, Materials::TextureFactory* textureFactory) {
		//Set Texture
		bool isDescriptorHeapsChange = false;
		if (textureFactory && this->textureIndex != Materials::TextureFactory::INVALID_TEXTURE_INDEX) {
			isDescriptorHeapsChange = textureFactory->SetTextureToRootSignature(cmdList, textureIndex);
		}
		//Draw Object
		uint32_t indexCount = this->indexCount;
		uint32_t startVertexIndex = 0;
		uint32_t startIndexNumber = 0;
		while (indexCount != startIndexNumber) {//draw all indexes
			UINT currentVertexCount,currentIndexCount;
			D3D12_VERTEX_BUFFER_VIEW vbv = objectBuffers->GetVertexBufferView(vertexBVIndex, startVertexIndex, &currentVertexCount);
			D3D12_INDEX_BUFFER_VIEW ibv = objectBuffers->GetIndexBufferView(indexBVIndex, startIndexNumber, &currentIndexCount);
			cmdList->IASetVertexBuffers(0, 1, &vbv);
			cmdList->IASetIndexBuffer(&ibv);
			cmdList->DrawIndexedInstanced(currentIndexCount, 1, 0, 0, 0);
			startVertexIndex += currentVertexCount;
			startIndexNumber += currentIndexCount;

		}
		return isDescriptorHeapsChange;
	}
}


namespace Graphics {

	void Camera::ComputeViewProjMatrix() {
		float cosPsi = cosf(psi);
		float sinPsi = sinf(psi);
		float cosFi = cosf(fi);
		float sinFi = sinf(fi);

		DirectX::XMVECTOR radiusVector = DirectX::XMVectorSet(radius * cosPsi * cosFi, radius * sinPsi, radius * cosPsi * sinFi, 0);
		position = targetPoint + radiusVector;
		DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(-sinPsi * cosFi, cosPsi, -sinPsi * sinFi, 0);
		viewMatrix = DirectX::XMMatrixLookAtLH(position, targetPoint, upDirection);
	}


	Camera::Camera(float vx, float vy, float vz, float px, float py, float pz) {
		position = DirectX::XMVectorSet(vx, vy, vz, 1);
		targetPoint = DirectX::XMVectorSet(px, py, pz, 1);
		//sphere positions
		DirectX::XMVECTOR radiusVector = position - targetPoint;
		radius = DirectX::XMVector3Length(radiusVector).m128_f32[0];
		psi = asinf(radiusVector.m128_f32[1] / radius);
		fi = acosf(radiusVector.m128_f32[0] / (radius * cos(psi)));
		//fi = asinf(radiusVector.m128_f32[2] / (radius*cos(psi)));
		DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(-sinf(psi) * cosf(fi), cosf(psi), -sinf(psi) * sinf(fi), 0);
		viewMatrix = DirectX::XMMatrixLookAtLH(position, targetPoint, upDirection);
	}

	void Camera::RotateCamera(float degreesFi, float degreesPsi) {
		psi += DirectX::XMConvertToRadians(degreesPsi);
		psi = fmod(psi, XM_2PI);
		if ((psi > XM_PIDIV2 && psi < 3 * XM_PIDIV2) || (psi<-XM_PIDIV2 && psi>-3 * XM_PIDIV2)) {
			fi -= DirectX::XMConvertToRadians(degreesFi);
		}
		else {
			fi += DirectX::XMConvertToRadians(degreesFi);
		}
		this->ComputeViewProjMatrix();
	}

	void Camera::ChangeRadius(float delta) {
		radius += delta;
		if (radius - 0.000001 < 0) throw Exception("Radius has negative value");
		this->ComputeViewProjMatrix();
	}

}

/////////////////////////////////////CONUS MESH//////////////////////////
namespace Graphics {

	std::vector<InputLayoutInfo> CreateInputLayoutInfoFromMeshInfo(MeshConstructInputInfo dataInfo, uint32_t vertexCount, uint32_t indexCount) {
		std::vector<InputLayoutInfo> inputBuffersInfo;
		if (dataInfo.vertex.sizeInBytes > 0) {
			InputLayoutInfo info{dataInfo.vertex.sizeInBytes,vertexCount, dataInfo.vertex.heapLocation};
			inputBuffersInfo.push_back(info);
		}
		if (dataInfo.indexes.sizeInBytes > 0) {
			InputLayoutInfo info{ dataInfo.indexes.sizeInBytes,indexCount,dataInfo.indexes.heapLocation };
			inputBuffersInfo.push_back(info);
		}
		return inputBuffersInfo;
	}

	void WriteIndexData(unsigned char*& dest, uint32_t index, unsigned indexSizeInBytes) {
		if (indexSizeInBytes == 4) {
			memcpy(dest, &index, sizeof(index));
		}
		else {
			uint16_t index16_t = index;
			memcpy(dest, &index16_t, sizeof(index16_t));

		}
		dest += indexSizeInBytes;
	}

	//break single buffer into small buffers compatible with gpu heap
	Object3D* CreateValidBuffers(GPUAdapter* adapter,const MeshConstructInputInfo& dataInfo, const void* vertexData,uint32_t& vertexCount,
		const void* indexData, uint32_t& indexCount) {
		const unsigned char* oldIndexData = static_cast<const unsigned char*>(indexData);
		uint16_t vertexSize = dataInfo.vertex.sizeInBytes;
		uint16_t indexSize = dataInfo.indexes.sizeInBytes;
		uint32_t maxVertexCountInSingleBuffer = Object3D::GetMaxElementCountInBuffer(vertexSize, dataInfo.vertex.heapLocation);
		uint32_t maxIndexCountInSingleBuffer = Object3D::GetMaxElementCountInBuffer(indexSize, dataInfo.indexes.heapLocation);
		uint32_t maxVertexBufferSize = maxVertexCountInSingleBuffer * vertexSize;
		uint32_t maxIndexBufferSize = maxIndexCountInSingleBuffer * indexSize;
		std::vector<std::pair<void*, uint32_t>> indexBuffers;
		std::vector<std::pair<void*, uint32_t>> vertexBuffers;
		uint32_t currentVertexCountInBuffer = 0;
		uint32_t currentIndexCountInBuffer = 0;
		unsigned char* currentVertexBuffer = new unsigned char[maxVertexBufferSize];
		unsigned char* currentIndexBuffer = new unsigned char[maxIndexCountInSingleBuffer];
		unsigned char* currentIndexBufferToWrite = currentIndexBuffer;
		std::map<uint32_t, uint32_t> mapVertexesNumbers;//key => last vertex index, value => new vertex index
		std::map<uint32_t, uint32_t>::iterator vertexNumberIter;
		uint32_t totalVertexNumber = 0, totalIndexNumber = 0;
		while (true) {//cycle througt all indexes
			if (currentVertexCountInBuffer + 3 <= maxVertexCountInSingleBuffer &&
				currentIndexCountInBuffer + 3 <= maxIndexCountInSingleBuffer 
				&& indexCount > 0) {//3 - number of max vertexes and indexes in triangle
				//get next 3 indexes
				for (int i = 0; i < 3; ++i) {
					uint32_t index;
					if (indexSize == 2) {
						index = *(uint16_t*)oldIndexData;
					}
					else {
						index = *(uint32_t*)oldIndexData;
					}
					vertexNumberIter = mapVertexesNumbers.find(index);
					if (vertexNumberIter == mapVertexesNumbers.end()) {//add vertex to buffer
						memcpy(currentVertexBuffer + currentVertexCountInBuffer*vertexSize, (char*)vertexData + index * vertexSize, vertexSize);
						if (dataInfo.vertexFunction != nullptr) dataInfo.vertexFunction(currentVertexBuffer + currentVertexCountInBuffer * vertexSize);
						mapVertexesNumbers.insert({ index,currentVertexCountInBuffer });
						index = currentVertexCountInBuffer++;
					}
					else {
						index = (*vertexNumberIter).second;//new index number
					}
					WriteIndexData(currentIndexBufferToWrite, index, indexSize);
					oldIndexData += indexSize;
					currentIndexCountInBuffer++;
				}
				indexCount -= 3;
			}
			else {//create next buffer part
				//add to buffers
				indexBuffers.push_back({ currentIndexBuffer, currentIndexCountInBuffer });
				vertexBuffers.push_back({ currentVertexBuffer, currentVertexCountInBuffer });
				//clear data
				totalVertexNumber += currentVertexCountInBuffer;
				totalIndexNumber += currentIndexCountInBuffer;
				currentVertexCountInBuffer = 0;
				currentIndexCountInBuffer = 0;
				currentVertexBuffer = new unsigned char[maxVertexBufferSize];
				currentIndexBuffer = new unsigned char[maxIndexCountInSingleBuffer];
				currentIndexBufferToWrite = currentIndexBuffer;
				mapVertexesNumbers.clear();
				if (indexCount == 0)break;
			}
		}
		//Create buffers
		std::vector<std::vector<uint32_t>> infoToInit;
		//vertexes
		infoToInit.push_back({});
		unsigned char* totalVertexBuffer = new unsigned char[totalVertexNumber * vertexSize];
		unsigned char* totalVertexBufferData = totalVertexBuffer;
		for (auto& vertexBuffer : vertexBuffers) {
			uint32_t bufferSize = vertexBuffer.second * vertexSize;
			memcpy(totalVertexBufferData, vertexBuffer.first, bufferSize);
			infoToInit[0].push_back(vertexBuffer.second);
			totalVertexBufferData += bufferSize;
			delete[] vertexBuffer.first;
		}
		//indexes
		infoToInit.push_back({});
		unsigned char* totalIndexBuffer = new unsigned char[totalIndexNumber * indexSize];
		unsigned char* totalIndexBufferData = totalIndexBuffer;
		for (auto& indexBuffer : indexBuffers) {
			uint32_t bufferSize = indexBuffer.second * indexSize;
			memcpy(totalIndexBufferData, indexBuffer.first, bufferSize);
			infoToInit[1].push_back(indexBuffer.second);
			totalIndexBufferData += bufferSize;
			delete[] indexBuffer.first;
		}
		//create Object3D
		Object3D* objectMesh = new Object3D(adapter, CreateInputLayoutInfoFromMeshInfo(dataInfo, totalVertexNumber, totalIndexNumber));
		objectMesh->AllocateMemory({ 0,1 }, infoToInit);
		objectMesh->InitBuffers({ 0,1 }, { totalVertexBuffer,totalIndexBuffer });
		delete[] totalVertexBuffer;
		delete[] totalIndexBuffer;
		vertexCount = totalVertexNumber;
		indexCount = totalIndexNumber;
		return objectMesh;
	}

	//brute algotitm to create surface normals
	void CreateNormalsForVertex(const MeshConstructInputInfo& dataInfo, const void* vertexData, uint32_t vertexCount,
		const void* indexData, uint32_t indexCount) {
		char* indexDataAddr = (char*)indexData;
		char* vertexDataAddr = (char*)vertexData;
		uint32_t vertexSize = dataInfo.vertex.sizeInBytes;
		uint32_t indexSize = dataInfo.indexes.sizeInBytes;
		uint32_t positionOffset = dataInfo.vertexLayout.positionOffsetBytes;
		uint32_t positionSize = dataInfo.vertexLayout.positionSizeBytes;
		uint32_t normalOffset = dataInfo.vertexLayout.normalOffsetBytes;
		uint32_t normalSize = dataInfo.vertexLayout.normalSizeBytes;
		uint32_t triangleCount = indexCount / 3;
		for (int i = 0; i < triangleCount; ++i) {
			uint32_t index0,index1,index2;
			if (indexSize == 2) {
				index0 = *(uint16_t*)(indexDataAddr); indexDataAddr += indexSize;
				index1 = *(uint16_t*)(indexDataAddr); indexDataAddr += indexSize;
				index2 = *(uint16_t*)(indexDataAddr); indexDataAddr += indexSize;
			}
			else {
				index0 = *(uint32_t*)(indexDataAddr); indexDataAddr += indexSize;
				index1 = *(uint32_t*)(indexDataAddr); indexDataAddr += indexSize;
				index2 = *(uint32_t*)(indexDataAddr); indexDataAddr += indexSize;
			}
			//DirectX::XMVECTOR normal = Math3D::GetTriangleNormal()
		}

	}

	//this function needs only valid arguments
	//Radius>0 and <1 segmentCount>2 RingsCount>0
	Object3D* CreateConusMesh(D3D12_NS::GPUAdapter* adapter, MeshConstructInputInfo dataInfo, float btmRadius, float upRadius, uint32_t segmentsCount, uint32_t sideRingsCount,
		MeshOutputInfo* outputInfo) {
		if (dataInfo.vertexLayout.positionSizeBytes < 3 * sizeof(float)) throw Exception("CreateConus: bad args");
		//count number of vertex and triangels
		uint32_t vertexCount = (segmentsCount + 1) * 2;//two bases
		vertexCount += (sideRingsCount - 1) * segmentsCount;//rings
		uint32_t triangelsNumber = segmentsCount * 2;//bases
		triangelsNumber += 2 * sideRingsCount * segmentsCount;//rings
		uint32_t centralPointIndex = 0;
		//CREATE BUFFERS
		//std::vector<InputLayoutInfo> inputData = CreateInputLayoutInfoFromMeshInfo(dataInfo, vertexCount, triangelsNumber);
		///Object3D* mesh = new Object3D(adapter, inputData.size(), &inputData[0]);
		//mesh->AllocateMemory();
		//FILL BUFFERS INFO
		//vertexes
		unsigned char* vertexData = new unsigned char[vertexCount * dataInfo.vertex.sizeInBytes];

		if (dataInfo.vertex.sizeInBytes > 0) {
			uint32_t positionOffset = dataInfo.vertexLayout.positionOffsetBytes;
			uint32_t positionSize = dataInfo.vertexLayout.positionSizeBytes;
			uint32_t vertexDataSize = dataInfo.vertex.sizeInBytes;
			//form top
			float dFi = XM_2PI / segmentsCount;//in circle
			float radius = upRadius;
			float dh = 1.0 / sideRingsCount;
			float tgAlpha = 1.0 / (btmRadius - upRadius);//Û„ÓÎ Ì‡ÍÎÓÌ‡ ÍÓÌÛÒ‡(ÔË‡ÏË‰˚)
			float dR = dh / tgAlpha;
			float currentY = 0.5;
			XMVECTOR pos;
			for (int ring = 0; ring < sideRingsCount + 1; ++ring) {
				//circle points
				float fi = dFi;
				for (int segment = 0; segment < segmentsCount; ++segment) {
					pos = XMVectorSet(radius * cosf(fi), currentY, radius * sinf(fi), 1);
					memcpy((void*)(vertexData + positionOffset), &pos, positionSize);
					positionOffset += vertexDataSize;
					fi += dFi;
					centralPointIndex++;
				}
				currentY -= dh;
				radius += dR;
			}
			//central points
			pos = XMVectorSet(0, 0.5, 0, 1);
			memcpy((void*)(vertexData + positionOffset), &pos, positionSize);
			positionOffset += vertexDataSize;
			pos = XMVectorSet(0, -0.5, 0, 1);
			memcpy((void*)(vertexData + positionOffset), &pos, positionSize);
			positionOffset += vertexDataSize;

		}
		//indexes
		unsigned char* indexData = new unsigned char[triangelsNumber * 3 * dataInfo.indexes.sizeInBytes];
		unsigned char* startIndexDataPtr = indexData;
		if (dataInfo.indexes.sizeInBytes > 0) {
			uint16_t indexSize = dataInfo.indexes.sizeInBytes;
			//triangles for ·ÓÍÓ‚˚Â sides
			uint32_t lowIndexRing = 0;
			for (uint32_t ring = 0; ring < sideRingsCount; ++ring) {
				for (uint32_t segment = 0; segment < segmentsCount; ++segment) {
					//first triangle
					WriteIndexData(indexData, lowIndexRing + segment, indexSize);
					if (segment != segmentsCount - 1) {
						WriteIndexData(indexData, lowIndexRing + segment + 1, indexSize);
					}
					else {
						WriteIndexData(indexData, lowIndexRing, indexSize);
					}
					WriteIndexData(indexData, lowIndexRing + segment + segmentsCount, indexSize);
					//second triangle
					if (segment != segmentsCount - 1) {
						WriteIndexData(indexData, lowIndexRing + segment + 1, indexSize);
						WriteIndexData(indexData, lowIndexRing + segment + segmentsCount + 1, indexSize);
					}
					else {
						WriteIndexData(indexData, lowIndexRing, indexSize);
						WriteIndexData(indexData, lowIndexRing + segmentsCount, indexSize);
					}
					WriteIndexData(indexData, lowIndexRing + segment + segmentsCount, indexSize);
				}
				lowIndexRing += segmentsCount;
			}
			//triangles for bases
			uint32_t btmStartIndex = centralPointIndex - segmentsCount;
			for (uint32_t segment = 0; segment < segmentsCount; ++segment) {
				//top
				WriteIndexData(indexData, centralPointIndex, indexSize);
				WriteIndexData(indexData, segment, indexSize);
				if (segment == 0) {
					WriteIndexData(indexData, segmentsCount - 1, indexSize);
				}
				else {
					WriteIndexData(indexData, (segment - 1), indexSize);
				}
				//btm
				WriteIndexData(indexData, centralPointIndex + 1, indexSize);
				if (segment == 0) {
					WriteIndexData(indexData, btmStartIndex + segmentsCount - 1, indexSize);
				}
				else {
					WriteIndexData(indexData, btmStartIndex + (segment - 1), indexSize);
				}
				WriteIndexData(indexData, btmStartIndex + segment, indexSize);

			}

		}
		//mesh->InitBuffers({ 0,1 }, { vertexData,startIndexDataPtr });
		triangelsNumber *= 3;
		Object3D* mesh = CreateValidBuffers(adapter, dataInfo, vertexData, vertexCount, startIndexDataPtr, triangelsNumber);
		delete[] vertexData;
		delete[] startIndexDataPtr;
		if (outputInfo != nullptr) {
			outputInfo->vertexCount = vertexCount;
			//outputInfo->indexCount = triangelsNumber * 3;
			outputInfo->indexCount = triangelsNumber;
		}
		return mesh;
	}


}


/// <summary>
/// //////////////////////////////////////SPHERE MESH
/// </summary>
namespace Graphics{
	//check vertex in buffer
	bool isUsedVertex(uint32_t* indexMatrix,uint32_t rowLength, uint32_t invalidValue, uint32_t index0, uint32_t index1,uint32_t& outIndex) {
		if (index0 > index1) {
			uint32_t temp = index0;
			index0 = index1;
			index1 = temp;
		}
		uint32_t offset = rowLength * index0;//start of the row
		for (int i = 1; i < index0; i++) offset -= i;
		offset += (index1 - index0) - 1;
		return (indexMatrix[offset] == invalidValue) ? false : (outIndex=indexMatrix[offset],true);
	}

	//create new vertex
	void ChangeIndexMatrix(uint32_t* indexMatrix, uint32_t rowLength, uint32_t index0, uint32_t index1, uint32_t index2) {
		if (index0 > index1) {
			uint32_t temp = index0;
			index0 = index1;
			index1 = temp;
		}
		uint32_t offset = rowLength * index0;//start of the row
		for (int i = 1; i < index0; i++) offset -= i;
		offset += (index1 - index0) - 1;
		indexMatrix[offset] = index2;
	}

	
	Object3D* CreateSphereMesh(D3D12_NS::GPUAdapter* adapter, MeshConstructInputInfo dataInfo, float sphereRadius,unsigned tesselationCount,
		MeshOutputInfo* outputInfo) {
		if (dataInfo.vertexLayout.positionSizeBytes < 3 * sizeof(float)) throw Exception("CreateConus: bad args");
		const float X = -0.525731f;
		const float Z = -0.850651f;
		const int BASIC_VERTEX_COUNT = 12;
		const int BASIC_INDEX_COUNT = 60;
		XMFLOAT3 pos[BASIC_VERTEX_COUNT] =
		{
		XMFLOAT3(-X, 0.0f, Z), XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X), XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X), XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f), XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f), XMFLOAT3(-Z, -X, 0.0f)
		};
		uint32_t k[BASIC_INDEX_COUNT] =
		{
		1,4,0, 4,9,0, 4,5,9, 8,5,4, 1,8,4,
		1,10,8, 10,3,8, 8,3,5, 3,2,5, 3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9, 11,2,7
		};

		//count number of vertex and triangels
		uint32_t triangelsNumber = (BASIC_INDEX_COUNT/3)*pow(4, tesselationCount - 1);
		uint32_t vertexCount = triangelsNumber/2+2;//??
		//perform division
		uint32_t matrixSize = vertexCount*vertexCount / 2;
		uint32_t* indexMatrix = new uint32_t[matrixSize];
		uint32_t invalidValue = vertexCount + 1;
		uint32_t* indexBuffers[2]; int currentIndexBufferIndex = 0; int outBufferIndex;
		indexBuffers[0] = new uint32_t[triangelsNumber * 3];
		indexBuffers[1] = new uint32_t[triangelsNumber * 3];
		//XMFLOAT3* positionBuffers[2];
		XMFLOAT3* positionBuffer = new XMFLOAT3[vertexCount];
		//positionBuffers[1] = new XMFLOAT3[vertexCount];
		//init
		for (int i = 0; i < BASIC_VERTEX_COUNT; i++) {
			positionBuffer[i] = pos[i];
		}
		uint32_t currentMaxVertex = BASIC_VERTEX_COUNT;
		for (int i = 0; i < BASIC_INDEX_COUNT; i++) {
			indexBuffers[0][i] = k[i];
		}
		uint32_t currentTriangelsNumber = BASIC_INDEX_COUNT/3;
		uint32_t outBufferIndexOffset = 0;
		//devision
		for (int i = 1; i < tesselationCount; ++i) {
			for (int j = 0; j< matrixSize; ++j) {
				indexMatrix[j] = invalidValue;
			}
			uint32_t indexOffset = 0;
			outBufferIndex = (currentIndexBufferIndex + 1) % 2;
			outBufferIndexOffset = 0;
			for (int j = 0; j < currentTriangelsNumber; j++) {
				uint32_t index0 = indexBuffers[currentIndexBufferIndex][indexOffset++];
				uint32_t index1 = indexBuffers[currentIndexBufferIndex][indexOffset++];
				uint32_t index2 = indexBuffers[currentIndexBufferIndex][indexOffset++];
				uint32_t index3,index4,index5;
				//¬€Õ≈—“» ›“Œ“ Ã”—Œ– ¬ œ–Œ÷≈ƒ”–”,  Œ√ƒ¿ ¡”ƒ≈“ ∆≈À≈Õ»≈
				if (!isUsedVertex(indexMatrix, vertexCount, invalidValue, index0, index1, index3)) {
					//create new vertex
					index3 = currentMaxVertex++;
					ChangeIndexMatrix(indexMatrix, vertexCount, index0, index1, index3);//
					positionBuffer[index3].x = (positionBuffer[index0].x + positionBuffer[index1].x) / 2;
					positionBuffer[index3].y = (positionBuffer[index0].y + positionBuffer[index1].y) / 2;
					positionBuffer[index3].z = (positionBuffer[index0].z + positionBuffer[index1].z)/2;
				}
				if (!isUsedVertex(indexMatrix, vertexCount, invalidValue, index1, index2, index4)) {
					//create new vertex
					index4 = currentMaxVertex++;
					ChangeIndexMatrix(indexMatrix, vertexCount, index1, index2, index4);//
					positionBuffer[index4].x = (positionBuffer[index1].x + positionBuffer[index2].x) / 2;
					positionBuffer[index4].y = (positionBuffer[index1].y + positionBuffer[index2].y) / 2;
					positionBuffer[index4].z = (positionBuffer[index1].z + positionBuffer[index2].z) / 2;
				}
				if (!isUsedVertex(indexMatrix, vertexCount, invalidValue, index0, index2, index5)) {
					//create new vertex
					index5 = currentMaxVertex++;
					ChangeIndexMatrix(indexMatrix, vertexCount, index0, index2, index5);//
					positionBuffer[index5].x = (positionBuffer[index0].x + positionBuffer[index2].x) / 2;
					positionBuffer[index5].y = (positionBuffer[index0].y + positionBuffer[index2].y) / 2;
					positionBuffer[index5].z = (positionBuffer[index0].z + positionBuffer[index2].z) / 2;
				}
				//fill new indexes
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index0;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index5;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index3;
				//
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index1;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index3;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index4;
				//
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index5;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index4;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index3;
				//
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index2;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index4;
				indexBuffers[outBufferIndex][outBufferIndexOffset++] = index5;
			}
			currentTriangelsNumber *= 4;
			currentIndexBufferIndex = (currentIndexBufferIndex + 1) % 2;
		}

		//count Vertex
		vertexCount = currentMaxVertex;
		//CREATE BUFFERS
		//std::vector<InputLayoutInfo> inputData = CreateInputLayoutInfoFromMeshInfo(dataInfo, vertexCount, triangelsNumber);
		//Object3D* mesh = new Object3D(adapter, inputData.size(), &inputData[0]);
		//mesh->AllocateMemory();
		unsigned char* vertexData = new unsigned char[vertexCount * dataInfo.vertex.sizeInBytes];
		unsigned char* indexData = new unsigned char[triangelsNumber * 3 * dataInfo.indexes.sizeInBytes];
		uint32_t positionOffset = dataInfo.vertexLayout.positionOffsetBytes;
		uint32_t positionSize = dataInfo.vertexLayout.positionSizeBytes;
		uint32_t normalOffset = dataInfo.vertexLayout.normalOffsetBytes;
		uint32_t textureOffset = dataInfo.vertexLayout.texCoordOffsetBytes;
		for (int i = 0; i < vertexCount; i++) {
			XMVECTOR vec = XMLoadFloat3(&positionBuffer[i]);
			vec = XMVector3Normalize(vec);
			vec *= sphereRadius;
			vec.m128_f32[3] = 1;
			memcpy((void*)(vertexData + positionOffset), &vec, positionSize);
			//normals
			memcpy((void*)(vertexData + normalOffset), &vec, dataInfo.vertexLayout.normalSizeBytes);
			//texCoord
			vec = DirectX::XMVector3Normalize(vec);
			//x = 1 (u = 0); y = 1 (v = 0)
			XMFLOAT2 texCoord;
			texCoord.x = 0.5 * ((vec.m128_f32[0] - 1)/-2);//in 1 and 2 quadrant
			if (vec.m128_f32[2] < 0) texCoord.x = 1 - texCoord.x;
			texCoord.y = 0.5 * ((vec.m128_f32[1] - 1) / -2);//in 1 and 2 quadrant
			if (vec.m128_f32[2] < 0) texCoord.y = 1 - texCoord.y;
			memcpy((void*)(vertexData + textureOffset), &vec, sizeof(texCoord));

			positionOffset += dataInfo.vertex.sizeInBytes;
			normalOffset += dataInfo.vertex.sizeInBytes;
			textureOffset += dataInfo.vertex.sizeInBytes;
		}
		unsigned indexSize = dataInfo.indexes.sizeInBytes;
		void* indexDataAddr = indexData;
		for (uint32_t i = 0; i < outBufferIndexOffset; i++) {
			WriteIndexData(indexData, indexBuffers[outBufferIndex][i], indexSize);
			//indexData[i] = indexBuffers[outBufferIndex][i];
		}
		//mesh->InitBuffers({ 0,1 }, { vertexData,indexDataAddr });
		triangelsNumber *= 3;
		Object3D* mesh = CreateValidBuffers(adapter, dataInfo, vertexData, vertexCount, indexDataAddr, triangelsNumber);

		
		if (outputInfo != nullptr) {
			outputInfo->vertexCount = vertexCount;
			//outputInfo->indexCount = triangelsNumber * 3;
			outputInfo->indexCount = triangelsNumber;
		}
		delete[] indexBuffers[0];
		delete[] indexBuffers[1];
		delete[] indexDataAddr;
		delete[] vertexData;
		delete[] indexMatrix;
		delete[] positionBuffer;
		return mesh;
	}
}


/////////////////////////////////GRID MESH
namespace Graphics {
	void StoreVertexPosition(void* buffer, XMVECTOR vec, unsigned char sizeInBytes) {
		if (sizeInBytes == 12) {
			XMFLOAT3 pos;  XMStoreFloat3(&pos, vec);
			memcpy(buffer, &pos, sizeInBytes);
		}
		else if (sizeInBytes == 16) {
			XMFLOAT4 pos;  XMStoreFloat4(&pos, vec);
			memcpy(buffer, &pos, sizeInBytes);
		}
		else {
			throw Exception("StoreVertexPosition: bad size");
		}
	}

	Object3D* CreateGridMesh(D3D12_NS::GPUAdapter* adapter, MeshConstructInputInfo dataInfo, unsigned int xWidth, unsigned int zDepth,
		MeshOutputInfo* outputInfo) {
		if (dataInfo.vertexLayout.positionSizeBytes < 3 * sizeof(float)) throw Exception("CreateGrid: bad args");
		constexpr float MAX_LENGTH = 1.0f;
		constexpr float MIN_COORD = -MAX_LENGTH/2;
		if (xWidth == 0 || zDepth == 0)throw Exception("CreateGrid: zero size");
		xWidth *= 2; zDepth *= 2;
		float ratio = xWidth / zDepth;
		float dx, dz;//length of the field
		float MIN_X_COORD, MAX_Z_COORD;
		if (xWidth > zDepth) {
			dx = MAX_LENGTH / xWidth;
			dz = dx / ratio;
			MIN_X_COORD = MIN_COORD;
			MAX_Z_COORD = dz*zDepth/2;
		}
		else {
			dz = MAX_LENGTH / zDepth;
			dx = dz * ratio;
			MAX_Z_COORD = -MIN_COORD;
			MIN_X_COORD = 0 - dx * xWidth / 2;
		}
		//count number of vertex and triangels
		uint32_t triangelsNumber = xWidth*zDepth*2;
		uint32_t vertexCount = (xWidth+1)*(zDepth+1);
		//CREATE BUFFERS
		//std::vector<InputLayoutInfo> inputData = CreateInputLayoutInfoFromMeshInfo(dataInfo, vertexCount, triangelsNumber);
		//Object3D* mesh = new Object3D(adapter, inputData.size(), &inputData[0]);
		//mesh->AllocateMemory();
		unsigned char* vertexData = new unsigned char[vertexCount * dataInfo.vertex.sizeInBytes];
		unsigned char* vertexDataStart = vertexData;
		unsigned char* indexData = new unsigned char[triangelsNumber * 3 * dataInfo.indexes.sizeInBytes];
		unsigned char* indexDataStart = indexData;

		uint32_t vertexSize = dataInfo.vertex.sizeInBytes;
		uint32_t positionOffset = dataInfo.vertexLayout.positionOffsetBytes;
		uint32_t positionSize = dataInfo.vertexLayout.positionSizeBytes;
		uint32_t indexSize = dataInfo.indexes.sizeInBytes;
		float xCoord = MIN_X_COORD;//from top left point
		float zCoord = MAX_Z_COORD;
		DirectX::XMFLOAT4 normal{ 0,1.0,0,0 };
		//cycle from fields in grid
		for (unsigned j = 0; j <= xWidth; ++j) {//first vertex line
			StoreVertexPosition(vertexData + positionOffset, XMVectorSet(xCoord, 0.0f, zCoord, 1), positionSize);
			memcpy(vertexData + dataInfo.vertexLayout.normalOffsetBytes, &normal, dataInfo.vertexLayout.normalSizeBytes);
			vertexData += vertexSize;
			xCoord += dx;
		}
		uint32_t currentIndexNumber = 0;
		uint32_t rowVertexCount = xWidth + 1;
		for (unsigned int i = 0; i < zDepth; ++i) {//z coord
			zCoord -= dz;
			xCoord = MIN_X_COORD;
			for (unsigned j = 0; j < xWidth; ++j) {//x coord
				//vertexes
				StoreVertexPosition(vertexData + positionOffset, XMVectorSet(xCoord, 0.0f, zCoord, 1), positionSize);
				memcpy(vertexData + dataInfo.vertexLayout.normalOffsetBytes, &normal, dataInfo.vertexLayout.normalSizeBytes);

				vertexData += vertexSize;
				xCoord += dx;
				//indexes (throug each row)
				WriteIndexData(indexData, currentIndexNumber, indexSize);//0
				WriteIndexData(indexData, currentIndexNumber + 1, indexSize);//1
				WriteIndexData(indexData, currentIndexNumber + rowVertexCount, indexSize);//7 (for xWidth = 6)
				WriteIndexData(indexData, ++currentIndexNumber, indexSize);//1
				WriteIndexData(indexData, currentIndexNumber + rowVertexCount, indexSize);//8
				WriteIndexData(indexData, currentIndexNumber + rowVertexCount - 1, indexSize);//7
			}
			StoreVertexPosition(vertexData + positionOffset, XMVectorSet(xCoord, 0.0f, zCoord, 1), positionSize);
			memcpy(vertexData + dataInfo.vertexLayout.normalOffsetBytes, &normal, dataInfo.vertexLayout.normalSizeBytes);

			vertexData += vertexSize;
			++currentIndexNumber;
		}
		
		//mesh->InitBuffers({ 0,1 }, { vertexDataStart,indexDataStart });
		triangelsNumber *= 3;
		Object3D* mesh = CreateValidBuffers(adapter, dataInfo, vertexDataStart, vertexCount, indexDataStart, triangelsNumber);

		if (outputInfo != nullptr) {
			outputInfo->vertexCount = vertexCount;
			//outputInfo->indexCount = triangelsNumber * 3;
			outputInfo->indexCount = triangelsNumber;
		}
		delete[] vertexDataStart;
		delete[] indexDataStart;
		return mesh;
	}
}