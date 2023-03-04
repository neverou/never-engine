#VertexLayout
	vec2 inPosition
	vec2 inTexcoord
	
#Samplers
	sampler2D mainTex

#Output
	[0] vec4 color

@type VertexShader

layout(location = 0) out vec2 vTexcoord;

void main(void) {
    gl_Position = vec4(inPosition, 0, 1);
	vTexcoord = inTexcoord;
}

@type PixelShader

layout(location = 0) in vec2 vTexcoord;

void main(void)
{
	vec4 baseColor = texture(mainTex, vTexcoord);
	color = baseColor;
	gl_FragDepth = 1; // ~Todo use shader parser seting to just not do z-write!
}
