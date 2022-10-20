#ifndef __CONFIG_SHADER_HEADER__

	#define __CONFIG_SHADER_HEADER__

	#define MAX_SOURCE_LIGHT_NUMBER 3

	#define OBJECT_BUFFER_B_ 1
	#define LIGHT_BUFFER_B_ 2
	#define COMMON_BUFFER_B_ 3

	#define TEXTURE_REG_INDEX 0

	#define GetRegisterName(reg, num) reg##num


	#ifndef __cplusplus
		#include "Structures.hlsli"

		cbuffer ObjectMatrix : register(GetRegisterName(b, OBJECT_BUFFER_B_)) {
			float4x4 objectMatrix;
			Material objectMaterial;
			float3 objectAmbientColor;
		}

		cbuffer FrameInfo : register(GetRegisterName(b, COMMON_BUFFER_B_)) {
			float4x4 viewProjMatrix;
			float4 frameEyePoint;
		}


		cbuffer LightInfo : register(GetRegisterName(b, LIGHT_BUFFER_B_)) {
			uint parallelLightSourceNumber;
			uint dotLightSourceNumber;
			uint cylinderLightSourceNumber;
			uint padding;
			SourceLight lightSources[MAX_SOURCE_LIGHT_NUMBER];
		}

		Texture2D MainObjectTexture : register(GetRegisterName(t, TEXTURE_REG_INDEX));

		SamplerState mainSampler : register(s0);

		/*float4x4 GetNormalsTransformMatrix(float4x4 worldMatrix) {
			float4x4 resMatrix = worldMatrix;
			resMatrix[3] = float4(0, 0, 0, 1);//delete transformation
			//resMatrix = 
		}*/

	#else
		#include "Material.h"
		struct RenderObjectInfo {
			DirectX::XMFLOAT4X4 worldMatrix;//not transponse matrix
			Materials::MaterialHLSL material;
			DirectX::XMFLOAT3 ambientLight;
			BYTE padding[256 - (255 & (sizeof(DirectX::XMFLOAT4X4)+sizeof(Materials::MaterialHLSL)+
				sizeof(DirectX::XMFLOAT3)) ) ];//for constant buffer
		};

		struct FrameRenderingInfo {
			DirectX::XMFLOAT4X4 viewProjMatrix;//transponse matrix
			DirectX::XMFLOAT4 eyePoint;
			BYTE padding[256 - (255 & (sizeof(DirectX::XMFLOAT4X4) + sizeof(DirectX::XMFLOAT4)))];
		};

		#include "Light.h"

		struct LightSourcesInfo {
			uint32_t parallelLightSourceNumber;
			uint32_t dotLightSourceNumber;
			uint32_t cylinderLightSourceNumber;
			uint32_t padding0[1];
			Light::LightHLSL lights[MAX_SOURCE_LIGHT_NUMBER];
			BYTE padding1[256 - (255 & (sizeof(uint32_t)*4 + MAX_SOURCE_LIGHT_NUMBER*sizeof(Light::LightHLSL)) )];
		};

	#endif

#endif
