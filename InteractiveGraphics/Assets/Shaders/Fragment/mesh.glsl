#version 330 core

in vec3 o_normal;

uniform vec3 lightPosition;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

out vec4 FragColor;

void main(){
	vec3 outputColor = o_normal;
	FragColor = vec4(o_normal, 1.0);
}