#version 450

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	float light_value = max(dot(inNormal, sceneData.sunlight_direction.xyz), 0.1f);

	vec3 color = inColor * texture(colorTex,inUV).xyz;
	vec3 ambient = color *  sceneData.ambient_color.xyz;

	outFragColor = vec4(color * light_value * sceneData.sunlight_color.w + ambient, 1.0f);
}
