#version 330 core

out vec4 FragColor;

in vec3 texCoords;

uniform samplerCube cubeMap;

void main(){
	FragColor = texture(cubeMap, texCoords);
}