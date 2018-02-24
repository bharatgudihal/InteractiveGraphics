#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 mvNormal;
uniform vec3 lightPosition;

out vec3 cameraSpaceVertexPosition;
out vec3 cameraSpaceVertexNormal;
out vec3 cameraSpaceLightPosition;

void main(){
	gl_Position = mvp * vec4(position, 1.0);
	cameraSpaceVertexPosition = vec3(mv * vec4(position, 1.0));
	cameraSpaceVertexNormal = mvNormal * normal;
	cameraSpaceLightPosition = lightPosition;
}