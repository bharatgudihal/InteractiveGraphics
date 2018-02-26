#version 330 core

in vec3 modelSpaceVertexPosition;

uniform mat4 virtualMVP;
uniform sampler2D reflectionMap;

out vec4 FragColor;

void main(){
	
	vec4 virtualClipSpacePosition = virtualMVP * vec4(modelSpaceVertexPosition,1.0);
	float u = virtualClipSpacePosition.x/virtualClipSpacePosition.w * 0.5 + 0.5;
	u = clamp(u,0,1);
	float v = virtualClipSpacePosition.y/virtualClipSpacePosition.w * 0.5 + 0.5;
	v = clamp(v,0,1);
	vec2 uv = vec2(u, v);	
	FragColor = texture(reflectionMap, uv);
	
}