#version 330 core

in vec3 modelSpaceVertexPosition;

uniform mat4 virtualMVP;
uniform sampler2D reflectionMap;
uniform vec2 screenSize;

out vec4 FragColor;

void main(){
	
	vec4 virtualClipSpacePosition = virtualMVP * vec4(modelSpaceVertexPosition,1.0);
	float u = virtualClipSpacePosition.x/virtualClipSpacePosition.w * 0.5 + 0.5;
	u = gl_FragCoord.x / screenSize.x;
	float v = virtualClipSpacePosition.y/virtualClipSpacePosition.w * 0.5 + 0.5;
	v = gl_FragCoord.y / screenSize.y;
	vec2 uv = vec2(u, 1-v);	
	FragColor = texture(reflectionMap, uv);
	
}