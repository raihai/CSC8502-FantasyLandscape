#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform vec4 nodeColour;

const float density = 0.0005;
const float gradient = 15;


in vec3 position;
in vec2 texCoord;


out Vertex {
	vec2 texCoord;
	vec4 colour;
	vec4 worldPos;
	float vis;
} OUT;


void main(void) {
	
	mat4 mvMatrix = viewMatrix * modelMatrix;
	mat4 mvpMatrix = projMatrix * mvMatrix;

	vec4 worldPosition = modelMatrix * vec4(position, 1.0);
	vec4 positionRelativeToCam = viewMatrix * worldPosition;
	
	float distance = length(positionRelativeToCam.xyz);
	float visib = exp(-pow((distance * density), gradient));
	visib = clamp(visib, 0.0, 1.0);
	
	OUT.vis = visib;


	gl_Position = mvpMatrix * vec4(position, 1.0);
	
	vec4 worldPos = ( mvMatrix * vec4 ( position,1));
	
	OUT.worldPos = worldPos;

	OUT.texCoord = texCoord;
	OUT.colour = nodeColour;

}