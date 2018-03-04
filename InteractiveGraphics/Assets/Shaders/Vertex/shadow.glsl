#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

out vec3 o_normal;
out vec3 o_position;
out vec4 shadowCoord;

void main(){
	gl_Position = projection * view * model * vec4(position, 1.0);
	o_normal = transpose(inverse(mat3(model))) * normal;
	o_position = vec3(model * vec4(position, 1.0));
	shadowCoord = lightSpaceMatrix * model * vec4(position, 1.0);
}