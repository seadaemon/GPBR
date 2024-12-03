precision highp float;
layout(set = 0, binding = 0) uniform  SceneData {
	mat4 view;
	mat4 proj;
	mat4 view_proj;
	vec3 camera_pos;
	vec4 ambient_color;
	vec4 sunlight_direction; //w for sun power
	vec4 sunlight_color;
} sceneData;

layout(set = 0, binding = 1) uniform LightData {
    vec3 position;
    vec3 color;
    float intensity;
    int type;
    vec4 extra[14];
} lightData;

#ifdef USE_BINDLESS

layout(set = 0, binding = 2) uniform sampler2D allTextures[];
#else
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
#endif

layout(set = 1, binding = 0) uniform GLTFMaterialData {
	vec4 base_color_factor;
	float metallic_factor;
	float roughness_factor;
	int color_texture_ID;
	int metal_rough_texture_ID;
	vec4 alpha_cutoff; // only x
} materialData;