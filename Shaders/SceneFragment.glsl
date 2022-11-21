#version 330 core

uniform sampler2D diffuseTex;
uniform int useTexture;

uniform vec4 fogColour;

in Vertex{
vec2 texCoord;
vec4 colour;
vec4 worldPos;
float vis;
}IN;

out vec4 fragColour;

void main (void) {

	
	if (texture(diffuseTex, IN.texCoord).a < 0.5)
	{
		discard;
	}

	fragColour = IN.colour;

	if(useTexture > 0 ) 
	{
		fragColour *= texture(diffuseTex, IN.texCoord);

		
		float visible = IN.vis;
		fragColour =  mix(fogColour, fragColour, visible );
	}

}