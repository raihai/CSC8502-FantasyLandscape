#version 330 core
uniform samplerCube cubeTex ;

uniform vec4 fogColour;

const float lowerLimit = 0.0;
const float upperLimit = 0.1;

in Vertex {
vec3 viewDir ;
} IN ;

out vec4 fragColour ;

void main ( void ) 
{
	
	vec4 finalColour = texture( cubeTex, normalize(IN.viewDir));
	
	vec3 texCoord = IN.viewDir;
	float factor = (texCoord.y - lowerLimit ) / (upperLimit - lowerLimit);
	factor = clamp(factor, 0.0, 1.0);

	fragColour = mix(fogColour, finalColour, factor);
}