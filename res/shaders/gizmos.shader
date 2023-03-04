#VertexLayout
	vec3 inPosition
	vec3 inColor

#Constants(gizmos)
	mat4 projectionMatrix
	mat4 viewMatrix

#Output
	[0] vec4 color




@type VertexShader

layout(location = 0) out vec4 color;


void main(void)
{
	gl_Position = (vec4(inPosition, 1) * gizmos.viewMatrix) * gizmos.projectionMatrix;
	color = vec4(inColor, 1.0);
}




@type PixelShader

layout(location = 0) in vec4 inColor;

void main(void)
{
	color = inColor;
}
