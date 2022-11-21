#version 330 core

uniform mat4 modelMatrix ;
uniform mat4 viewMatrix ;
uniform mat4 projMatrix ;

in vec3 position ;
in vec3 normal ;
in vec2 texCoord;

out Vertex {
vec2 texCoord ;
vec3 normal ;
vec4 worldPos ;
} OUT ;

void main ( void ) {
	
	mat4 mvMatrix = viewMatrix * modelMatrix;
	mat4 mvpMatrix = projMatrix * mvMatrix;
	gl_Position = mvpMatrix * vec4(position, 1.0);
	
	// Output all out variables
	OUT.texCoord = texCoord;
	OUT.normal =   normal;

	vec4 worldPos = ( mvMatrix * vec4 ( position,1));
	OUT.worldPos = worldPos;

}