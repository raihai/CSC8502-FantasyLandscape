#version 330 core
uniform mat4 modelMatrix ;
uniform mat4 viewMatrix ;
uniform mat4 projMatrix ;


const float density = 0.0004;
const float gradient = 10;

in vec3 position ;
in vec4 colour ;
in vec3 normal ;
in vec4 tangent ;
in vec2 texCoord;

out Vertex {
vec4 colour ;
vec2 texCoord ;
vec3 normal ;
vec3 tangent ; 
vec3 binormal ; 
vec3 worldPos ;
float vis;
} OUT ;

void main ( void ) {
	OUT.colour = colour ;
	OUT.texCoord = texCoord ;

	mat3 normalMatrix = transpose(inverse(mat3(modelMatrix )));

	vec3 wNormal = normalize(normalMatrix * normalize(normal));
	vec3 wTangent = normalize(normalMatrix * normalize(tangent.xyz ));

	OUT.normal = wNormal ;
	OUT.tangent = wTangent ;
	OUT.binormal = cross( wTangent, wNormal ) * tangent.w ;

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