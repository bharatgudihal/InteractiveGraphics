#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat3 cameraMatrix;
uniform mat3 normalMatrix;

out vec3 o_normal;
out vec3 o_position;

void main(){
	gl_Position = mvp * vec4(position, 1.0);
	o_normal = normalMatrix * normal;
	o_position = cameraMatrix * position;
}