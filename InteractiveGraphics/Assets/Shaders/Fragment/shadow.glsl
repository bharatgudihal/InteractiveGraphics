#version 330 core

in vec3 o_normal;
in vec3 o_position;
in vec2 o_uv;
in vec4 shadowCoord;

uniform vec3 lightPosition;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform vec3 cameraPosition;
uniform sampler2DShadow shadowMap;

out vec4 FragColor;

void main(){

	//Check shadow map	
	vec3 projectionCoords = shadowCoord.xyz/shadowCoord.w;
	projectionCoords = projectionCoords * 0.5 + 0.5;
	float closestDepth = texture(shadowMap, projectionCoords.xyz);
	float currentDepth = shadowCoord.z;
	float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

	vec3 diffuseColor = (1 - shadow) * diffuse;
	vec3 ambientColor = ambient;
	vec3 specularColor = (1 - shadow) * specular;

	vec3 lightColor = vec3(0.8);
	vec3 ambientLightColor = vec3(0.2);
	
	vec3 normal = normalize(o_normal);
	
	vec3 lightDirection = normalize(lightPosition - o_position);
	float inclination = clamp(dot(lightDirection, normal), 0, 1);
	
	vec3 cameraDirection = normalize(o_position - cameraPosition);
	vec3 halfVector = normalize(cameraDirection + lightDirection);
	
	float specularity = clamp(dot(halfVector, normal), 0, 1);
	
	vec3 outputColor = lightColor * (diffuseColor * inclination + specularColor * pow(specularity, shininess)) + (ambientLightColor * ambientColor);	

	FragColor = vec4(outputColor, 1.0);
}