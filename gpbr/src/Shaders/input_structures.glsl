precision mediump float;
layout(set = 0, binding = 0) uniform  SceneData{   

	mat4 view;
	mat4 proj;
	mat4 view_proj;
	vec4 ambient_color;
	vec4 sunlight_direction; //w for sun power
	vec4 sunlight_color;
} sceneData;

layout(set = 1, binding = 0) uniform GLTFMaterialData{   

	vec4 color_factors;
	vec4 metal_rough_factors;
	vec4 alpha_cutoff; // only x
	vec4 transmission_factor; // only x
	vec4 attenuation_color; // xyz
	vec4 attentuation_distance; // only x
	vec4 thickness_factor; // only x
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
