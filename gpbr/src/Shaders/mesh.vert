#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outPosition;
layout (location = 4) out vec3 outLightPos; 
layout (location = 5) out vec3 outCameraPos;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertex_buffer;
} PushConstants;

void main() 
{
	Vertex v = PushConstants.vertex_buffer.vertices[gl_VertexIndex];

	vec4 position = vec4(v.position, 1.0f);

	gl_Position =  sceneData.view_proj * PushConstants.render_matrix * position;

	mat4 view_mat = sceneData.view;

	// Apply the normal matrix; needed for non-uniform scale
	outNormal = mat3(transpose(inverse(view_mat * PushConstants.render_matrix))) *  v.normal;
	
	// View space 
	outPosition = (view_mat * PushConstants.render_matrix * position).xyz;
	outLightPos = (view_mat * vec4(lightData.position,1.0)).xyz;
	outCameraPos = (view_mat * vec4(sceneData.camera_pos, 1.0)).xyz;

	outColor = v.color.rgba * materialData.base_color_factor.rgba;
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
}