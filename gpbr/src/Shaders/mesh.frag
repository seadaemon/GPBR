#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	float light_value = max(dot(inNormal, sceneData.sunlight_direction.xyz), 0.1f);

	vec4 color = inColor.rgba * texture(colorTex,inUV).rgba;
	vec3 ambient = color.xyz *  sceneData.ambient_color.xyz;

	outFragColor = vec4(color.xyz * light_value * sceneData.sunlight_color.w + ambient, color.a);
	
	//if(outFragColor.a < materialData.alpha_cutoff.x)
	//{
	//	discard;
	//}
}
