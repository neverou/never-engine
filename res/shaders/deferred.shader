#PushConstants
	mat4 lightPose
	vec3 lightColor
	float lightIntensity
	int lightType

#VertexLayout
	vec2 inPosition
	vec2 inTexcoord

#Samplers
	sampler2D albedo
	sampler2D normals
	sampler2D depth

#Output
	[0] vec4 outColor


#Config
	blendEnable true
	blendSrcColor BlendFactor_One
	blendSrcAlpha BlendFactor_One

	blendDestColor BlendFactor_One
	blendDestAlpha BlendFactor_One

	blendOpColor BlendOp_Add
	blendOpAlpha BlendOp_Add


#Constants(camera)
	mat4 projectionMatrix
	mat4 viewMatrix

@type VertexShader

layout(location = 0) out vec2 vTexcoord;

void main(void) {
    gl_Position = vec4(inPosition, 0, 1);
	vTexcoord = inTexcoord;
}

@type PixelShader


#define LightType_Point 0
#define LightType_Directional 1
#define LightType_Spot 2

layout(location = 0) in vec2 vTexcoord;

float LinearizeDepth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}


void main(void)
{
	vec4 baseColor = texture(albedo, vTexcoord);
	vec3 normal = texture(normals, vTexcoord).xyz;

	float depthV = LinearizeDepth(texture(depth, vTexcoord).r, 0.1, 1000.0); // ~Todo put near and far planes in the camera

	vec3 viewFar = (inverse(transpose(camera.projectionMatrix)) * vec4((vTexcoord * 2 - 1) * vec2(1,-1), 1, 1)).xyz;
	vec4 worldPos = (inverse(transpose(camera.viewMatrix)) * vec4(viewFar * depthV, 1));

	vec3 lightPos = (vec4(0,0,0,1) * lightPose).xyz;
	vec3 lightFacing = (vec4(0,0,1,0) * lightPose).xyz;	
	

	float coefficient = 0;	
	float dotProd     = 0;
	if (lightType == LightType_Point)
	{		
		vec3 lDir = lightPos - worldPos.xyz;
		coefficient = lightIntensity / dot(lDir, lDir);
		dotProd     = max(dot(normal, normalize(lDir) ), 0.0);
	}
	else if (lightType == LightType_Directional)
	{		
		coefficient = lightIntensity;
		dotProd     = max(dot(lightFacing, -normal), 0.0);
	}

	vec3 color = baseColor.xyz * lightColor * coefficient * dotProd; // * smoothstep(0.3, 0.5, dotProd);

	outColor = vec4(color, 1.0);
}
