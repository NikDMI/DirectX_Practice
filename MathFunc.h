#pragma once
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

namespace Math3D {

	inline DirectX::XMVECTOR GetTriangleNormal(DirectX::FXMVECTOR vertex1, DirectX::FXMVECTOR vertex2, DirectX::FXMVECTOR vertex3) {
		DirectX::XMVECTOR vec1 = DirectX::operator-(vertex2, vertex1);
		DirectX::XMVECTOR vec2 = DirectX::operator-(vertex3, vertex1);
		return DirectX::XMVector3Normalize(DirectX::XMVector3Cross(vec1, vec2));
	}
}