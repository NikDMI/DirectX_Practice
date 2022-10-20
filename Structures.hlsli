
struct SourceLight {
	float3 color;//the "energy" of the light source
	float nearDistance;//distance to which the light energy is constant
	float3 centralPoint;//
	float farDiatance;//>=nearDistance - from this distance the light energy decresing linearly
	float3 direction;// direction for cylinder light
	float radialPower;// power form the cos^m(theta) in the cylinder light
};


struct Material {
	float3 albedo; //amount of ligth, that can be reflected
	float shininess;//0..1, 1 - all specular light only in one direction, 0 - specular light diffuses
	float3 rFrensel; //amount of specular light when see to the normal of the surface at 0 deegres
	float padding;
};