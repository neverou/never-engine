#VertexLayout
	vec3 inPosition
	vec3 inColor
	vec2 inTexcoord


#PushConstants
	int guiMode

#Constants(gui)
	mat4 projectionMatrix

#Samplers
	sampler2D mainTex

#Output
	[0] vec4 color

@type VertexShader

layout(location = 0) out vec2 texcoord;
layout(location = 1) out vec4 color;

void main(void)
{
    gl_Position = vec4(inPosition, 1) * gui.projectionMatrix;
	texcoord = inTexcoord;
	color = vec4(inColor, 1.0);
}




@type PixelShader

layout(location = 0) in vec2 inTexcoord;
layout(location = 1) in vec4 inColor;

#define GuiMode_Color 0
#define GuiMode_Image 1

float rand(vec2 n)
{ 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

void main(void)
{
	if (guiMode == GuiMode_Color)
	{
		color = inColor;
	}
	else if (guiMode == GuiMode_Image)
	{
		color = inColor * texture(mainTex, inTexcoord);
	}

	if (color.a < 0.5) discard;
}
