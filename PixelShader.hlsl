#include "ConfigShaders.h"


float3 GetReflectedLight(float3 lightColor,float cosTheta) {//count the color of reflected light
	//cosTheta >0 and <=1
	float k = 1 - cosTheta;
	float3 rFrensel = objectMaterial.rFrensel + (float3(1, 1, 1) - objectMaterial.rFrensel)*k*k*k*k*k ;
	return rFrensel * lightColor;
}

float3 GetRougnessLightCoeff(float3 vectorL, float3 normal) {
	float3 sVector = normalize(vectorL + normalize(frameEyePoint));//new "normal" of the surface
	float cosPsi = dot(normal, sVector);//amplitude of this vector from real normal
	cosPsi = clamp(cosPsi, 0, 1);
	const float mRougness = 0.3;//coeff, when surface is the most roughness
	const float mShineness = 18.0;//when surface is shine
	float mRougnessCoeff = mShineness - (mShineness - mRougness) * (1-objectMaterial.shininess);//ÝÒÎ ÌÎÆÍÎ ÂÛÍÅÑÒÈ Â CPU
	float coeff = pow(cosPsi, mRougnessCoeff);
	return float3(coeff, coeff, coeff);
}

float4 GetParallelLightColor(float4 normal) {
	float4 resultColor = (0, 0, 0, 0);
	for (uint i = 0; i < parallelLightSourceNumber && i < MAX_SOURCE_LIGHT_NUMBER; ++i) {
		float cosTheta = dot(normal, normalize(lightSources[i].centralPoint));
		float cosThetaN = cosTheta;
		cosTheta = clamp(cosTheta, 0, 1);
		//if (cosTheta == 0) return float4(0, 0, 0, 0);
		resultColor += float4(lightSources[i].color*cosTheta*objectMaterial.albedo, 1);//diffuse light
		if (cosThetaN >= 0) {
			float3 reflectedLight = GetReflectedLight(lightSources[i].color, cosTheta);
			float3 rougnessCoeff = GetRougnessLightCoeff(normalize(lightSources[i].centralPoint), normal);
			reflectedLight = reflectedLight * rougnessCoeff;
			resultColor += float4(reflectedLight * objectMaterial.albedo, 0);//specular light
		}
	}
	return resultColor;
}

float4 main(float4 pos: SV_POSITION, float4 normal : NORMAL, float4 ambientColor : A_COLOR,
	float2 texCoord : TEXCOORD) : SV_TARGET
{
	float4 parallelLight = GetParallelLightColor(normal);
	float4 totalColor = ambientColor + parallelLight;
	totalColor = MainObjectTexture.Sample(mainSampler, texCoord) * totalColor;
	return totalColor;
}
