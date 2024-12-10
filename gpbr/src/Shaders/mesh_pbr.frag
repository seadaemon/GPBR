#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define USE_BINDLESS
#include "input_structures.glsl"
#include "pbr.glsl"
#include "blinn_phong.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPos;
layout (location = 4) in vec3 inLightPos;
layout (location = 5) in vec3 inCameraPos;

layout (location = 0) out vec4 outFragColor;

struct SHCoefficients {
    vec3 l00, l1m1, l10, l11, l2m2, l2m1, l20, l21, l22;
}; 

const SHCoefficients grace = SHCoefficients(
    vec3( 0.3623915,  0.2624130,  0.2326261 ),
    vec3( 0.1759131,  0.1436266,  0.1260569 ),
    vec3(-0.0247311, -0.0101254, -0.0010745 ),
    vec3( 0.0346500,  0.0223184,  0.0101350 ),
    vec3( 0.0198140,  0.0144073,  0.0043987 ),
    vec3(-0.0469596, -0.0254485, -0.0117786 ),
    vec3(-0.0898667, -0.0760911, -0.0740964 ),
    vec3( 0.0050194,  0.0038841,  0.0001374 ),
    vec3(-0.0818750, -0.0321501,  0.0033399 )
);

vec3 calcIrradiance(vec3 nor) {
    const SHCoefficients c = grace;
    const float c1 = 0.429043;
    const float c2 = 0.511664;
    const float c3 = 0.743125;
    const float c4 = 0.886227;
    const float c5 = 0.247708;
    return (
        c1 * c.l22 * (nor.x * nor.x - nor.y * nor.y) +
        c3 * c.l20 * nor.z * nor.z +
        c4 * c.l00 -
        c5 * c.l20 +
        2.0 * c1 * c.l2m2 * nor.x * nor.y +
        2.0 * c1 * c.l21  * nor.x * nor.z +
        2.0 * c1 * c.l2m1 * nor.y * nor.z +
        2.0 * c2 * c.l11  * nor.x +
        2.0 * c2 * c.l1m1 * nor.y +
        2.0 * c2 * c.l10  * nor.z
    );
}

void main() 
{
    // light, normal, view, half
    vec3 lv = normalize(inLightPos - inPos);
    vec3 nv = normalize(inNormal);
    vec3 vv = normalize(inCameraPos - inPos); 
    vec3 hv = normalize(lv + vv);

    vec3 irradiance = calcIrradiance(nv);

    // Fetch color and metallic-roughness textures
    int colorID = materialData.color_texture_ID;
    vec4 base_color = inColor * texture( nonuniformEXT(allTextures[colorID]), inUV);
    int metallic_rough_ID = materialData.metal_rough_texture_ID;
    vec4 metallic_roughness = texture(allTextures[metallic_rough_ID], inUV);    
 
    // Calculate perceptual roughness/metallic
    float roughness = max(materialData.roughness_factor * metallic_roughness.g,  1e-2);
    float metallic = materialData.metallic_factor * metallic_roughness.b;
    metallic = metallic * metallic;
    float LoN = clamp(dot(lv, nv), 0.0f, 1.f); 
     
    // Calculate f0 and diffuse_color 
    vec3 diffuse_color = (1.0 - metallic) * base_color.rgb; 
    vec3 dielectric_specular = vec3(0.04);    
    vec3 f0 = mix(dielectric_specular, base_color.rgb, metallic);  
     
    vec3 pbr = pbr_BRDF(diffuse_color, metallic, roughness, f0, nv, vv, lv);  

    outFragColor = vec4(pbr * LoN + base_color.rgb*irradiance.x * vec3(0.2f), base_color.a);
}