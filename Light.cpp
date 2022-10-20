#include "Light.h"

namespace Light {
	using namespace DirectX;

	SourceLight::SourceLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 centralPoint, float nearDistance, float farDistance,
		DirectX::XMFLOAT3 direction, float radialPower){
		lightInfo.color = color;
		lightInfo.centralPoint = centralPoint;
		lightInfo.direction = direction;
		lightInfo.farDistance = farDistance;
		lightInfo.nearDistance = nearDistance;
		lightInfo.radialPower = radialPower;
	}


	ParallelLight::ParallelLight(DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 centralPoint):
	SourceLight(color,centralPoint)
	{
		type = LightType::parallel;
	}
}