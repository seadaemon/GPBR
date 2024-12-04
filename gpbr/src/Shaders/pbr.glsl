precision highp float;
#define PI 3.1415926535897932384626433832795

// Adopted from: https://learnopengl.com/PBR/Theory

// Normal distribution function (specular D): approximates the surface area
// of microfacets exactly aligned to the halfway vector h.

// Trowbridge-Reitz GGX Distribution Function (specular D)
float D_GGX(float NoH, float roughness)
{
    float a = NoH * roughness;
    float k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / 4.0);
}

// Geometry function (specular G): approximates the relative surface area where microfacets
// overshadow one another.

// Adopted from: https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf
// Schlick-GGX Geometry Function
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

// Fresnel (specular F): describes the ratio of reflected light to refracted light.

// Adopted from: https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf
// Fresnel-Schlick approximation (scalar optimization)
vec3 F_Schlick(float HoV, vec3 f0)
{
	return f0 + (vec3(1.0) - f0) * pow(1.0 - HoV, 5.0);
}

// Fresnel-Schlick (4.5 Listing 8)
float F_Schlick(float u, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

// Lambertian diffuse BRDF
float Fd_Lambert()
{
	return 1.f / PI;
}

// Burley diffuse BRDF
float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
	//float f90 = clamp(50.0 * 1.0, 0.0, 1.0);
    float lightScatter = F_Schlick(NoL, 1.0, f90);
    float viewScatter = F_Schlick(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

// Adapted from: https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf
// Bidirectional Radiance Distribution Function
vec3 pbr_BRDF(vec3 diffuse, float metallic, float roughness, vec3 f0, vec3 n, vec3 v, vec3 l)
{
	vec3 h = normalize(v + l);

	float NoV = abs(dot(n,v)) + 1e-5;
	float NoL = clamp(dot(n, l), 0.0, 1.0);
	float NoH = clamp(dot(n, h), 0.0, 1.0);
	float HoV = clamp(dot(h, v), 0.0, 1.0);
	float LoH = clamp(dot(l,h), 0.0, 1.0);

	// perceptually linear roughness
	float rough = max(roughness * roughness, 1e-2);

	float D = D_GGX(NoH, rough);
	vec3  F = F_Schlick(LoH, f0);
	float V = V_SmithGGXCorrelated(NoV, NoL, rough); // G

	// specular BRDF
	vec3 Fr = (D * V) * F;
	
	vec3 energyCompensation = 1.0 + f0 * (1.0 / Fr.g - 1.0);

	Fr *= energyCompensation;

	// diffuse BRDF
	//vec3 Fd = diffuse * Fd_Lambert();

	vec3 Fd = diffuse * Fd_Burley(NoV, NoL, LoH, rough);

	return (Fd + Fr) * NoL;
}