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

#Constants(animation)
	mat4[128] bones
	int boneCount


#Output
	[0] vec4 color
	[1] vec4 normal

@type VertexShader

layout(location = 0) out vec3 localPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 viewPosition;
layout(location = 3) out vec2 texcoord;

#define MAX_WEIGHTS 4

void main(void)
{
	vec3 skeletalPos = vec3(0);

	for (int i = 0; i < MAX_WEIGHTS; i++)
	{
		mat4 bonePose = animation.bones[int(boneIds[i])];
		vec3 pos = vec3(vec4(inPosition, 1.0) * bonePose);

		skeletalPos += weights[i] * pos;
	}


    gl_Position = ((vec4(skeletalPos, 1) * transformMatrix) * camera.viewMatrix) * camera.projectionMatrix;

	viewPosition = ((vec4(skeletalPos, 1) * transformMatrix) * camera.viewMatrix).xyz;


	

	texcoord = inTexcoord;

	localPosition = skeletalPos;
	worldNormal = normalize((vec4(inNormal, 0) * transformMatrix).xyz);
}

@type PixelShader

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 viewPosition;
layout(location = 3) in vec2 texcoord;

void main(void)
{
	color  = material.color;
	normal = vec4(worldNormal, 1);
}
