#version 330 core

in vec3 cameraSpaceVertexPosition;
in vec3 cameraSpaceVertexNormal;
in vec3 cameraSpaceLightPosition;

uniform mat4 inverseViewMatrix;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform samplerCube cubeMap;

out vec4 FragColor;

void main(){
	
	vec3 incidentEye = normalize(cameraSpaceVertexPosition);
	vec3 normal = normalize(cameraSpaceVertexNormal);

	vec3 reflected = reflect(incidentEye, normal);
		
	reflected = vec3(inverseViewMatrix * vec4(reflected, 0.0));

	vec4 reflectedFragmentColor = texture(cubeMap, reflected);

	vec3 lightColor = vec3(1.0);
	vec3 ambientLightColor = vec3(0.2);

	vec3 lightDirection =  normalize(cameraSpaceLightPosition - cameraSpaceVertexPosition);
	float diffuseIntensity = clamp(dot(lightDirection, normal), 0, 1);

	vec3 halfVector = normalize(cameraSpaceVertexPosition + cameraSpaceLightPosition);

	float specularity = clamp(dot(halfVector, normal), 0, 1);

	vec3 blinnColor = lightColor * (diffuse * diffuseIntensity + specular * pow(specularity, shininess)) + (ambientLightColor * ambient);
	
	FragColor = reflectedFragmentColor * vec4(blinnColor, 1.0);	
}