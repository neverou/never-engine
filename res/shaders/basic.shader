#PushConstants
	mat4 transformMatrix

#VertexLayout
	vec3 inPosition
	vec3 inNormal
	vec2 inTexcoord

	vec4 weights
	vec4 boneIds

#Constants(material)
	vec4 color

#Constants(camera)
	mat4 projectionMatrix
	mat4 viewMatrix


#Samplers
	sampler2D mainTex

#Output
	[0] vec4 color
	[1] vec4 normal

@type VertexShader

layout(location = 0) out vec3 localPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 viewPosition;
layout(location = 3) out vec2 texcoord;

void main(void) {
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

layout(location = 4) in vec4 weighh;

void main(void)
{
	color = texture(mainTex, texcoord) * material.color;
	normal = vec4(worldNormal, 1);
}
