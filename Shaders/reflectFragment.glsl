#version 330 core

uniform sampler2D diffuseTex ;
uniform samplerCube cubeTex ;
uniform vec3 cameraPos ;
uniform vec4 fogColour;

in Vertex {
vec3 colour ;
vec2 texCoord ;
vec3 normal ;
vec3 worldPos ;
float vis;
} IN ;

out vec4 fragColour ;

void main ( void ) 
{
vec4 diffuse = texture( diffuseTex, IN.texCoord );
vec3 viewDir = normalize( cameraPos - IN.worldPos );


vec3 reflectDir = reflect( -viewDir , normalize(IN.normal ));

vec4 reflectTex = texture( cubeTex , reflectDir );
	

fragColour = reflectTex + ( diffuse * 0.25f );

float visible = IN.vis;
fragColour = mix(fogColour, fragColour, visible );
}
