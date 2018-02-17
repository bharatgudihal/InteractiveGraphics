#version 330 core

in vec3 o_normal;
in vec3 o_position;
in vec2 o_uv;

uniform vec3 lightPosition;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;
uniform vec3 cameraPosition;
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

out vec4 FragColor;

void main(){
	vec3 diffuseColor = vec3(texture(diffuseMap, o_uv));
	vec3 ambientColor = diffuseColor;
	vec3 specularColor = vec3(texture(specularMap, o_uv));
	vec3 lightColor = vec3(0.8);
	vec3 ambientLightColor = vec3(0.2);
	vec3 normal = normalize(o_normal);
	vec3 lightDirection = normalize(lightPosition - o_position);
	float inclination = clamp(dot(lightDirection, normal), 0, 1);
	vec3 cameraDirection = normalize(o_position - cameraPosition);
	vec3 halfVector = (cameraDirection + lightDirection);
	halfVector = halfVector / length(halfVector);
	halfVector = normalize(halfVector);
	float specularity = clamp(dot(halfVector, normal), 0, 1);
	vec3 outputColor = lightColor * (diffuseColor * inclination + specularColor * pow(specularity, shininess)) + (ambientLightColor * ambientColor);	
	FragColor = vec4(outputColor, 1.0);
}