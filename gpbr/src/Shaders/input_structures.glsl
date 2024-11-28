precision highp float;
layout(set = 0, binding = 0) uniform  SceneData {
	mat4 view;
	mat4 proj;
	mat4 view_proj;
	vec4 ambient_color;
	vec4 sunlight_direction; //w for sun power
	vec4 sunlight_color;
} sceneData;

#ifdef USE_BINDLESS
layout(set = 0, binding = 1) uniform sampler2D allTextures[];
#else
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
#endif

layout(set = 1, binding = 0) uniform GLTFMaterialData {
	vec4 color_factors;
	vec4 metal_rough_factors;
	int color_texture_ID;
	int metal_rough_texture_ID;
	vec4 alpha_cutoff; // only x
} materialData;

layout(set = 2, binding = 0) uniform LightData {
	vec3 position;
    vec3 color;
    float intensity;
    int type;
    vec4 extra[14];
} lightData;