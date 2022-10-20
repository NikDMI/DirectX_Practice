#pragma once
#include "Config.h"
#include "MathFunc.h"

namespace Light {

	//describes data for 3 types of lightning: direct, point, cylinder
	struct LightHLSL {
		DirectX::XMFLOAT3 color;//the "energy" of the light source
		float nearDistance;//distance to which the light energy is constant
		DirectX::XMFLOAT3 centralPoint;//
		float farDistance;//>=nearDistance - from this distance the light energy decresing linearly
		DirectX::XMFLOAT3 direction;// direction for cylinder light
		float radialPower;// power form the cos^m(theta) in the cylinder light
	};

	class SourceLight {
	public:
		enum class LightType { parallel, dot, cylinder };
	protected:
		LightHLSL lightInfo;
		SourceLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 centralPoint, float nearDistance = 0.0f, float farDistance = 0.0f,
			DirectX::XMFLOAT3 direction = { 0,0,0 }, float radialPower = 0.0f);
		LightType type;
	public:
		LightType GetType() { return type; };
		LightHLSL GetLightHLSL() { return lightInfo; };
	};

	class ParallelLight:public SourceLight {
	private:
	public:
		ParallelLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 centralPoint);
	};

}
