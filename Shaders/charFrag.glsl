#version 330 core
uniform sampler2D diffuseTex;
uniform vec4 fogColour;

in Vertex {
vec2 texCoord;
float vis;
} IN;

out vec4 fragColour;
void main(void) 
{
	fragColour = texture(diffuseTex , IN.texCoord );

	float visible = IN.vis;
	fragColour = mix(fogColour, fragColour, visible );
}
