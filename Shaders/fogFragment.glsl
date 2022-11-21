#version 330 core

uniform vec4 fogColour;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;

uniform vec3 cameraPos ;
uniform vec4 lightColour ;
uniform vec3 lightPos ;
uniform float lightRadius ;

in Vertex {
vec2 texCoord ;
vec3 normal ;
vec4 worldPos ;
} IN ;

out vec4 fragColour;

void main(void) 
{

	vec3 incident = normalize ( lightPos - IN.worldPos.xyz );
	vec3 viewDir = normalize ( cameraPos - IN.worldPos.xyz );
	vec3 halfDir = normalize ( incident + viewDir );

	vec4 diffuse = vec4(0.0);

	float lambert = max(dot(incident, IN.normal), 0.0f);
	float distance = length ( lightPos - IN.worldPos.xyz );
	float attenuation = 1.0f - clamp ( distance / lightRadius ,0.0 ,1.0);
	float specFactor = clamp( dot( halfDir , IN.normal),0.0 ,1.0);
	specFactor = pow( specFactor , 15.0 );

	vec3 surface = ( diffuse.rgb * lightColour.rgb );
	fragColour.rgb = surface * lambert * attenuation ;
	fragColour.rgb += ( lightColour.rgb * specFactor )* attenuation * 0.2;
	fragColour.rgb += surface * 0.3f;
	fragColour.a = diffuse.a;


	// Add fog
	float fogCoord = abs(IN.worldPos.z/IN.worldPos.w);
	float fogFactor = 1.0 - clamp( exp(-fogDensity * fogCoord), 0.0, 1.0);
	float result = exp(-fogDensity * fogCoord);

	fragColour =  mix(fragColour, fogColour, result);


}