#version 330 core

in vec2 o_uv;

uniform sampler2D diffuseMap;

out vec4 FragColor;

void main(){
	vec3 diffuseColor = vec3(texture(diffuseMap, o_uv));
	vec3 black = vec3(0);
	vec3 defaultColor = vec3(0.2);
	if(diffuseColor == black){
		diffuseColor = defaultColor;
	}
	FragColor = vec4(diffuseColor, 1.0);
}