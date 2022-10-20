#include "ConfigShaders.h"

void main( float3 pos : POSITION, float3 normal: NORMAL,float2 texCoord:TEXCOORD,
	out float4 posOut: SV_POSITION, out float4 normalOut: NORMAL,out float4 ambientColor: A_COLOR,
	out float2 texCoordOut: TEXCOORD)
{
	//pos.xy += 0.5f * sin(pos.x) * sin(3.0f * currentTime);
	//pos.z *= 0.6f + 0.4f * sin(2.0f * currentTime);
	//pos.y = sin(3*pos.x);
	//pos.y = 0.3f * (pos.z * sin(20.0f * pos.x)+0.5*cos(10*pos.x)) + pos.x * cos(pos.z * 9.0f);
	float4x4 objectMatrixT = transpose(objectMatrix);

	normalOut.xyz = normal.xyz; normalOut.w = 0;
	normalOut = normalize(mul(normalOut, objectMatrixT));

	objectMatrixT = mul(objectMatrixT, viewProjMatrix);
	posOut.xyz = pos.xyz; posOut.w = 1;
	posOut = mul(posOut, objectMatrixT);
	
	ambientColor = float4(objectMaterial.albedo * objectAmbientColor, 1);//можно вынести в CPU

	texCoordOut = texCoord;
}

