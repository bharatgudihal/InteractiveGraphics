#version 330 core

in vec3 cameraSpaceVertexPosition;

uniform mat4 inverseViewMatrix;

uniform samplerCube cubeMap;

out vec4 FragColor;

void main(){
	
	vec3 incidentEye = normalize(cameraSpaceVertexPosition);
	
	//convert from camera to world space
	incidentEye = -vec3(inverseViewMatrix * vec4(incidentEye, 0.0));
	
	FragColor = texture(cubeMap, incidentEye);	
}