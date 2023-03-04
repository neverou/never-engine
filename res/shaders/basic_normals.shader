#PushConstants
	mat4 transformMatrix

#VertexLayout
	vec3 inPosition
	vec3 inNormal
	vec2 inTexcoord


#Constants(material)
	vec4 color

#Constants(camera)
	mat4 projectionMatrix
	mat4 viewMatrix


#Samplers
	sampler2D mainTex
	sampler2D normalTex

#Output
	[0] vec4 color
	[1] vec4 normal

@type VertexShader

layout(location = 0) out vec3 localPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 viewPosition;
layout(location = 3) out vec2 texcoord;

void main(void)
{
    gl_Position = ((vec4(inPosition, 1) * transformMatrix) * camera.viewMatrix) * camera.projectionMatrix;

	viewPosition = ((vec4(inPosition, 1) * transformMatrix) * camera.viewMatrix).xyz;

	texcoord = inTexcoord;

	localPosition = inPosition;
	worldNormal = normalize((vec4(inNormal, 0) * transformMatrix).xyz);
}



@type PixelShader

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 viewPosition;
layout(location = 3) in vec2 texcoord;

void main(void)
{
	// vec3 normalMap = normalize(texture(normalTex, texcoord).rgb * 2. - 1.);
	
	color = texture(mainTex, texcoord) * material.color;

	normal = vec4(worldNormal, 1);
}
