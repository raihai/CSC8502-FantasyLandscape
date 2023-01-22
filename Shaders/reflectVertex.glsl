#version 330 core

uniform mat4 modelMatrix ;
uniform mat4 viewMatrix ;
uniform mat4 projMatrix ;
uniform mat4 textureMatrix ;

const float density = 0.0005;
const float gradient = 20;

in vec3 position ;
in vec3 normal ;
in vec2 texCoord ;

out Vertex {
vec3 colour ;
vec2 texCoord ;
vec3 normal ;
vec3 worldPos ;
float vis;
} OUT ;

void main ( void ) {
mat3 normalMatrix = transpose(inverse ( mat3 ( modelMatrix )));

OUT.texCoord = ( textureMatrix * vec4 ( texCoord , 0.0 , 1.0)).xy ;

OUT.normal = normalize(normalMatrix * normalize(normal ));

vec4 worldPos = ( modelMatrix * vec4 ( position,1));

OUT.worldPos = worldPos.xyz ;

vec4 worldPosition = modelMatrix * vec4(position, 1.0);
vec4 positionRelativeToCam = viewMatrix * worldPosition;

float distance = length(positionRelativeToCam.xyz);
float visib = exp(-pow((distance * density), gradient));
visib = clamp(visib, 0.0, 1.0);
	
OUT.vis = visib;

gl_Position = ( projMatrix * viewMatrix ) * worldPos ;
 }