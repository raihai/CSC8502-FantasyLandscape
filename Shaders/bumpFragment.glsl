#version 330 core

uniform sampler2D ground; 
uniform sampler2D grass;
uniform sampler2D rocks;
uniform sampler2D rockBump;

uniform vec3 cameraPos ;
uniform vec4 lightColour ;
uniform vec3 lightPos ;
uniform float lightRadius ;
uniform float vHeight;
uniform vec4 fogColour;
in Vertex {
vec4 colour ;
vec2 texCoord ;
vec3 normal ;
vec3 tangent ; 
vec3 binormal ; 
vec3 worldPos ;
float vis;
} IN ;

out vec4 fragColour ;

void main ( void ) {

	vec3 incident = normalize( lightPos - IN.worldPos );
	vec3 viewDir = normalize( cameraPos - IN.worldPos );
	vec3 halfDir = normalize( incident + viewDir);

	mat3 TBN = mat3(normalize( IN.tangent ) ,normalize( IN.binormal ), normalize(IN.normal));
	
	vec3 bumpNormal = texture(rockBump, IN.texCoord).rgb ;
	bumpNormal = normalize(bumpNormal * 2.0 - 1.0);
	bumpNormal = normalize( TBN * bumpNormal  );	

	vec4 diffuse = vec4(0.0);
	float fScale = IN.worldPos.y/vHeight;

	const float fRange1 = 0.15f;
	const float fRange2 = 0.3f;
	const float fRange3 = 0.65f;
  	const float fRange4 = 0.85f;

	if(fScale >= 0.0 && fScale <= fRange1)
		diffuse  = texture(ground, IN.texCoord);
	else if (fScale <= fRange2)
	{
		fScale -= fRange1;
		fScale /= (fRange2-fRange1);
		
		float fScale2 = fScale;
		fScale = 1.0-fScale; 
		
		diffuse += texture(ground, IN.texCoord)*fScale;
		diffuse += texture(grass, IN.texCoord)*fScale2;
	}
	else if(fScale <= fRange3)
		diffuse = texture(grass, IN.texCoord);
	else if(fScale <= fRange4)
	{
		fScale -= fRange3;
		fScale /= (fRange4-fRange3);
		
		float fScale2 = fScale;
		fScale = 1.0-fScale; 
		
		diffuse += texture(grass, IN.texCoord)*fScale;
		diffuse += texture(rocks, IN.texCoord)*fScale2;		
	}
	else diffuse = texture(rocks, IN.texCoord);


	float lambert = max(dot(incident, bumpNormal ), 0.0f);
	float distance = length ( lightPos - IN.worldPos );
	float attenuation = 1.0f - clamp ( distance / lightRadius ,0.0 ,1.0);

	float specFactor = clamp( dot( halfDir , bumpNormal ),0.0 ,1.0);
	specFactor = pow( specFactor , 15.0 );
	vec3 surface = ( diffuse.rgb * lightColour.rgb );
	fragColour.rgb = surface * lambert * attenuation ;
	fragColour.rgb += ( lightColour.rgb * specFactor )* attenuation * 0.2;
	fragColour.rgb += surface * 0.3f;
	fragColour.a = diffuse.a;

	float visible = IN.vis;
	fragColour = mix(fogColour, fragColour, visible );
 }
