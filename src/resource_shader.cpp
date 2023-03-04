#include "resource.h"
#include "shaderparser.h"

#include "shaderc/shaderc.hpp"

intern String ShaderFieldTypeToString(ShaderFieldType type)
{
	if (type == ShaderField_Float) return TCopyString("float");
	if (type == ShaderField_Float2) return TCopyString("vec2");
	if (type == ShaderField_Float3) return TCopyString("vec3");
	if (type == ShaderField_Float4) return TCopyString("vec4");
	if (type == ShaderField_Mat4) return TCopyString("mat4");
	if (type == ShaderField_Int) return TCopyString("int");
	
	LogWarn("[res] Failed to convert shader field type to string (%d)", type);
	return TCopyString("<unknown>");
}

intern String ShaderSamplerTypeToString(ShaderSamplerType type)
{
	// ~Refactor ~Rename TCopyString() -> TCopy()
	if (type == ShaderSampler_Sampler1D) 	return TCopyString("sampler1D");
	if (type == ShaderSampler_Sampler2D) 	return TCopyString("sampler2D");
	if (type == ShaderSampler_Sampler3D) 	return TCopyString("sampler3D");
	
	LogWarn("[res] Failed to convert shader sampler type to string (%d)", type);
	return TCopyString("<unknown>");
}

u64 UtilShaderFieldTypeElementSize(ShaderFieldType type)
{
	switch (type)
	{
		case ShaderField_Float:  return 1 * sizeof(float);
		case ShaderField_Float2: return 2 * sizeof(float);
		case ShaderField_Float3: return 3 * sizeof(float);
		case ShaderField_Float4: return 4 * sizeof(float);
		case ShaderField_Mat4: 	 return 4 * 4 *  sizeof(float);
		case ShaderField_Int: 	 return sizeof(int);

		default: Assert(false);
	}
	return 0;
}

u64 UtilShaderFieldElementSize(ShaderFieldTypeDef typeDef)
{
	u64 size = UtilShaderFieldTypeElementSize(typeDef.type);
	if (typeDef.isArray)
		size *= typeDef.elementCount;
	return size;
}

intern String GenerateVarDeclCode(StringView name, ShaderFieldTypeDef typeDef)
{
	if (typeDef.isArray)
		return TPrint("%s %s[%d]", ShaderFieldTypeToString(typeDef.type).data, name.data, typeDef.elementCount);
	else
		return TPrint("%s %s", ShaderFieldTypeToString(typeDef.type).data, name.data);
}

ShaderResource* LoadShader(StringView name)
{
	{
		auto* existing = SearchResource(&resourceManager.shaders, name);
		if (existing)
			return existing;
	}


	ShaderResource shader;
	shader.pushConstants = MakeDynArray<ShaderField>();
	shader.vertexLayout = MakeDynArray<ShaderField>();
	shader.constantBufferLayouts = MakeDynArray<ConstantBufferLayout>();
	shader.samplerFields = MakeDynArray<ShaderSamplerField>();
	shader.outputFields = MakeDynArray<ShaderOutputField>();

	String rawVertexSrc = TempString();
	String rawPixelSrc = TempString();

	// ~Refactor @@MemoryLeak destroy these if we exit without success!
	ShaderConfigInfo shaderConfig { };
	if (!ParseShader(name,
		&shader.pushConstants,
		&shader.vertexLayout,
		&shader.constantBufferLayouts,
		&shader.samplerFields,
		&shader.outputFields,
		&shaderConfig,
		&rawVertexSrc,
		&rawPixelSrc))
	{
		LogWarn("[res] Parsing %s failed!", name.data);
		return NULL;
	}


	// Generate the code constructs from the metadata:
	int genConstructIdx = 0;
	String adjVertexSrc = TempString();
	String adjPixelSrc  = TempString();

	StringView versionHeader = "#version 450\n";

	// ~Hack hardcoded
	adjVertexSrc.Concat(versionHeader);
	adjPixelSrc.Concat(versionHeader);

	// Push constants
	if (shader.pushConstants.size > 0)
	{
		String buffer = TempString();

		buffer.Concat("layout(push_constant) uniform ");
		buffer.Concat(TPrint("_GENERATED_%d", genConstructIdx++));
		buffer.Concat(" {\n");

		For (shader.pushConstants)
		{
			buffer.Concat("\t");
			buffer.Concat(GenerateVarDeclCode(it->name, it->typeDef));
			buffer.Concat(";\n");
		}

		buffer.Concat("};\n");

		adjVertexSrc.Concat(buffer);
		adjPixelSrc.Concat(buffer);
	}

	// Vertex Layout
	if (shader.vertexLayout.size > 0)
	{ 
		String buffer = TempString();

		int idx = 0;
		For (shader.vertexLayout)
		{
			buffer.Concat(TPrint("layout(location=%d) in ", idx));
			buffer.Concat(GenerateVarDeclCode(it->name, it->typeDef));
			buffer.Concat(";\n");
			idx++;
		}

		adjVertexSrc.Concat(buffer);
	}

	// Constant Buffers
	if (shader.constantBufferLayouts.size > 0) 
	{
		String buffer = TempString();

		int idx = 0;
		For (shader.constantBufferLayouts)
		{
			buffer.Concat(TPrint("layout (binding=%d) uniform _GENERATED_%d {\n", idx, genConstructIdx++));

			ForIt (it->fields, it2)
			{
				buffer.Concat("\t");
				buffer.Concat(GenerateVarDeclCode(it2->name, it2->typeDef));
				buffer.Concat(";\n");
			}

			buffer.Concat("} ");
			buffer.Concat(it->name);
			buffer.Concat(";\n");
			idx++;
		}

		adjVertexSrc.Concat(buffer);
		adjPixelSrc.Concat(buffer);
	}

	if (shader.samplerFields.size > 0) {
		String buffer = TempString();

		For (shader.samplerFields)
		{
			buffer.Concat(TPrint("layout(binding=%d) uniform ", it->binding));
			buffer.Concat(ShaderSamplerTypeToString(it->type));
			buffer.Concat(" ");
			buffer.Concat(it->name);
			buffer.Concat(";\n");	
		}

		adjVertexSrc.Concat(buffer);
		adjPixelSrc.Concat(buffer);		
	}


	if (shader.outputFields.size > 0) { // Output Fields
		String buffer = TempString();

		For (shader.outputFields)
		{
			buffer.Concat(TPrint("layout(location=%d) out ", it->index));
			buffer.Concat(GenerateVarDeclCode(it->name, it->typeDef));
			buffer.Concat(";\n");
		}

		adjPixelSrc.Concat(buffer);
	}

	adjVertexSrc.Concat(rawVertexSrc);
	adjPixelSrc.Concat(rawPixelSrc);

	// Log(adjVertexSrc);
	// Log("=============");
	// Log(adjPixelSrc);

	// Compile the shader
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	// options.AddMacroDefinition("MY_DEFINE", "1");
	// if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

	shaderc::SpvCompilationResult vertexBinary =
		compiler.CompileGlslToSpv(adjVertexSrc.data, shaderc_glsl_vertex_shader, name.data, options);
	
	shaderc::SpvCompilationResult pixelBinary =
		compiler.CompileGlslToSpv(adjPixelSrc.data, shaderc_glsl_fragment_shader, name.data, options);

	if (vertexBinary.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		LogWarn("[shader catalog] Failed to compile Vertex Shader '%s': %s", name.data, vertexBinary.GetErrorMessage().c_str());
		
		// We ran into an error with the vertex shader, therefore print the shader code
		Log("=== %s (vertex) ===", name.data);
		String vertexSourceWithLineNumbers = TempString();
		Array<String> lines = BreakString(adjVertexSrc, '\n');
		int line = 1;
		For (lines)
		{
			vertexSourceWithLineNumbers.Concat(TPrint("% 4d || ", line));
			vertexSourceWithLineNumbers.Concat(*it);
			vertexSourceWithLineNumbers.Concat("\n");
			line++;
		}
		Log(vertexSourceWithLineNumbers.data);
		Log("====================");
		return NULL;
	}


	if (pixelBinary.GetCompilationStatus() != shaderc_compilation_status_success) {
		LogWarn("[shader catalog] Failed to compile Pixel Shader '%s': %s", name.data, pixelBinary.GetErrorMessage().c_str());

		// We ran into an error with the pixel shader, therefore print the shader code
		Log("=== %s (pixel) ===", name.data);
		
		String pixelSourceWithLineNumbers = TempString();
		Array<String> lines = BreakString(adjPixelSrc, '\n');
		int line = 1;
		For (lines)
		{
			pixelSourceWithLineNumbers.Concat(TPrint("% 4d || ", line));
			pixelSourceWithLineNumbers.Concat(*it);
			pixelSourceWithLineNumbers.Concat("\n");
			line++;
		}
		Log(pixelSourceWithLineNumbers.data);

		Log("====================");
		return NULL;
	}
	// 





	ShaderStageDescription vertShaderDesc = {};
	{
		vertShaderDesc.data = (u32*)vertexBinary.cbegin();
		vertShaderDesc.size = (Size)((u32*)vertexBinary.cend() - (u32*)vertexBinary.cbegin()) * sizeof(u32);
		vertShaderDesc.type = SHADER_VERTEX;
	}

	ShaderStageDescription pixShaderDesc = {};
	{
		pixShaderDesc.data = (u32*)pixelBinary.cbegin();
		pixShaderDesc.size = (Size)((u32*)pixelBinary.cend() - (u32*)pixelBinary.cbegin()) * sizeof(u32);
		pixShaderDesc.type = SHADER_PIXEL;
	}


	




	ShaderDescription shaderDesc = {};

	shaderDesc.fillMode 		= shaderConfig.fillMode;
    shaderDesc.frontFace 		= shaderConfig.frontFace;
    shaderDesc.lineWidth 		= shaderConfig.lineWidth;
    shaderDesc.cullMode 		= shaderConfig.cullMode;
    shaderDesc.blendEnable 		= shaderConfig.blendEnable;
    shaderDesc.blendSrcColor 	= shaderConfig.blendSrcColor;
    shaderDesc.blendDestColor 	= shaderConfig.blendDestColor;
    shaderDesc.blendOpColor 	= shaderConfig.blendOpColor;
    shaderDesc.blendSrcAlpha 	= shaderConfig.blendSrcAlpha;
    shaderDesc.blendDestAlpha 	= shaderConfig.blendDestAlpha;
    shaderDesc.blendOpAlpha 	= shaderConfig.blendOpAlpha;
	shaderDesc.depthTestEnable 	= shaderConfig.depthTestEnable;
	shaderDesc.depthWriteEnable = shaderConfig.depthWriteEnable;
	shaderDesc.depthCompareOp 	= shaderConfig.depthCompareOp;

	
	FixArray<ShaderStageDescription, 2> shaders;
	shaders[0] = vertShaderDesc;
	shaders[1] = pixShaderDesc;
	shaderDesc.stages = &shaders;

	shaderDesc.lineWidth = 1.0;
	shaderDesc.frontFace = FrontFace_CW;
	shaderDesc.cullMode = Cull_Back;
	

	FixArray<VertexInputLayout, 1> inputLayouts;
	VertexInputLayout layout;
	layout.binding = 0;
	layout.inputRate = VERTEX_INPUT_RATE_PER_VERTEX; // ~Todo figure out instancing

	int i = 0;
	Size offset = 0;
	For (shader.vertexLayout)
	{
		VertexInputElement inputElement;
		inputElement.location = i;
		inputElement.offset = offset;

		RenderFormat vertexFormat = FORMAT_NONE;
		
		// ~Todo possibly allow choosing the format instead of the shader field type in the shader metadata
		// eg instead of just "vec3" -> "r32g32b32_float"
		switch (it->typeDef.type)
		{
			case ShaderField_Float:  vertexFormat = FORMAT_R32_FLOAT; break;
			case ShaderField_Float2: vertexFormat = FORMAT_R32G32_FLOAT; break;
			case ShaderField_Float3: vertexFormat = FORMAT_R32G32B32_FLOAT; break;
			case ShaderField_Float4: vertexFormat = FORMAT_R32G32B32A32_FLOAT; break;
			// je suis lazy
			case ShaderField_Mat4: Assert(false); break;
			case ShaderField_Int: Assert(false); break;

			default: Assert(false); break;
		}

		inputElement.format = vertexFormat;

		offset += UtilShaderFieldElementSize(it->typeDef);

		layout.elements[i] = inputElement;
		i++;
	}

	layout.stride = offset;
	layout.elementCount = shader.vertexLayout.size;

	inputLayouts[0] = layout;
	shaderDesc.vertexInputLayouts = &inputLayouts;

	RendererHandle handle = game->renderer->CreateShader(&shaderDesc);

	shader.name = CopyString(name);
	shader.handle = handle;

	Log("[res] loaded shader (%s)", name.data);

	// ~Hack ~FixMe do something if this errors probably
	return BucketArrayAdd(&resourceManager.shaders, shader);
}



int ShaderConstantBufferSlot(ShaderResource* shader, StringView name)
{
	ForIdx (shader->constantBufferLayouts, idx)
	{
		auto* it = &shader->constantBufferLayouts[idx];
		if (strcmp(it->name.data, name.data) == 0)
		{
			return idx;
		}
	}
	return -1;
}

int ShaderSamplerSlot(ShaderResource* shader, StringView name)
{
	ForIdx (shader->samplerFields, idx)
	{
		auto* it = &shader->samplerFields[idx];
		if (strcmp(it->name.data, name.data) == 0)
		{
			return idx;
		}
	}
	return -1;
}
