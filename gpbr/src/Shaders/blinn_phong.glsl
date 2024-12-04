precision highp float;

vec3 blinnPhongBRDF(vec3 diffuse_color, vec3 n, vec3 v, vec3 l, vec3 h)
{
	vec3 Fa = vec3(0.1f);

	float lambertian = max(dot(l,n), 0.f);

	vec3 Fd = diffuse_color * lambertian;

	vec3 specular_color = diffuse_color * 0.5f;
	float NoH = clamp(dot(n,h), 0.f, 1.f);

	float shininess = 32.f;

	vec3 Fr = specular_color * pow(NoH, shininess); 

	return Fa + Fd + Fr;
}