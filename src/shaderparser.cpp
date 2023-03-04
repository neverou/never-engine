#include "shaderparser.h"

#include "allocators.h"
#include "renderer.h"

#include <string.h> 

enum ShaderParserMode
{
	ShaderParserMode_Metadata,
	ShaderParserMode_VertexShader,
	ShaderParserMode_PixelShader,
};

enum ShaderMetadataParseMode
{
	ShaderMetadataParse_None,
	ShaderMetadataParse_PushConstants,
	ShaderMetadataParse_VertexLayout,
	ShaderMetadataParse_Constants,
	ShaderMetadataParse_Samplers,
	ShaderMetadataParse_Output,
	ShaderMetadataParse_Config,
};

struct ShaderVarDecl
{
	String name;
	String type;

	bool isArray;
	int elementCount;
};

// Returns true if parsing was successful, otherwise returns false!
intern bool ParseVarDecl(StringView input, ShaderVarDecl* varOut)
{
	ShaderVarDecl var = {};
	
	// Make sure names and types are alphanumeric
	Array<String> parts = BreakString(input, ' ');
	if (parts.size != 2) return false;

	var.type = TempString();


	int arrayBracketAt = 0;

	for (int i = 0; i < parts[0].length; i++)
	{
		if (parts[0].data[i] == '[')
		{
			var.isArray = true;
			arrayBracketAt = i;
			break;
		}

		var.type.Concat(CharStr(&parts[0].data[i])); // Append each character of the type
	}

	if (var.isArray)
	{
		String arrayElementCountStr;
		bool foundClosingBracket = false;
		for (int i = arrayBracketAt + 1; i < parts[0].length; i++)
		{
			// Read until a closing bracket
			if (parts[0].data[i] == ']')
			{
				foundClosingBracket = true;
				arrayElementCountStr = Substr(parts[0], arrayBracketAt + 1, i);

				if (i != parts[0].length - 1)
				LogWarn("[shaderparser] Garbage following the array brackets will not be considered! <<%s>>", input.data);
				break;
			}
		}

		if (!foundClosingBracket)
		{
			LogWarn("[shaderparser] Failed to parse variable declartion, missing closing bracket in array definition! <<%s>>", input.data);
			return false;
		}

		var.elementCount = atoi(arrayElementCountStr.data);
	}

	var.name = parts[1];
	
	*varOut = var;
	return true;
}

intern ShaderFieldType NameToShaderFieldType(StringView name)
{
	if (strcmp(name.data, "float") == 0) 	return ShaderField_Float;
	if (strcmp(name.data, "vec2") == 0) 	return ShaderField_Float2;
	if (strcmp(name.data, "vec3") == 0) 	return ShaderField_Float3;
	if (strcmp(name.data, "vec4") == 0) 	return ShaderField_Float4;
	if (strcmp(name.data, "mat4") == 0) 	return ShaderField_Mat4;
	if (strcmp(name.data, "int") == 0) 		return ShaderField_Int;
	else
		LogWarn("[shaderparser] Failed to parse shader field type name! '%s'", name.data);

	return ShaderField_None;
}

intern ShaderFieldTypeDef VarDeclToShaderFieldTypeDef(ShaderVarDecl var)
{
	ShaderFieldTypeDef typeDef;
	typeDef.type         = NameToShaderFieldType(var.type);
	typeDef.isArray      = var.isArray;
	typeDef.elementCount = var.elementCount;	
	return typeDef;
}


intern ShaderSamplerType NameToShaderSamplerType(StringView name)
{
	if (strcmp(name.data, "sampler1D") == 0) 	return ShaderSampler_Sampler1D;
	if (strcmp(name.data, "sampler2D") == 0) 	return ShaderSampler_Sampler2D;
	if (strcmp(name.data, "sampler3D") == 0) 	return ShaderSampler_Sampler3D;
	else 
		LogWarn("[shaderparser] Failed to parse shader sampler type name! '%s'", name.data);

	return ShaderSampler_None;
}



#define EnumParse(enumValue, thing) if (strcmp(value.data, #thing) == 0) { enumValue = thing; }



intern bool ParseBlendOp(StringView value, BlendOp* blendOpPtr)
{
	BlendOp blendOp;
	EnumParse(blendOp, BlendOp_Add)
	else EnumParse(blendOp, BlendOp_Subtract)
	else EnumParse(blendOp, BlendOp_Reverse_Subtract)
	else EnumParse(blendOp, BlendOp_Min)
	else EnumParse(blendOp, BlendOp_Max)
	else return false;

	*blendOpPtr = blendOp;
	return true;
}

intern bool ParseBlendFactor(StringView value, BlendFactor* blendFactorPtr)
{
	BlendFactor blendFactor;
	
	EnumParse(blendFactor, BlendFactor_Zero)
    else EnumParse(blendFactor, BlendFactor_One)
    else EnumParse(blendFactor, BlendFactor_Src_Color)
    else EnumParse(blendFactor, BlendFactor_One_Minus_Src_Color)
    else EnumParse(blendFactor, BlendFactor_Dst_Color)
    else EnumParse(blendFactor, BlendFactor_One_Minus_Dst_Color)
    else EnumParse(blendFactor, BlendFactor_Src_Alpha)
    else EnumParse(blendFactor, BlendFactor_One_Minus_Src_Alpha)
    else EnumParse(blendFactor, BlendFactor_Dst_Alpha)
    else EnumParse(blendFactor, BlendFactor_One_Minus_Dst_Alpha)
    else EnumParse(blendFactor, BlendFactor_Src_Alpha_Saturate)
	else return false;

	*blendFactorPtr = blendFactor;
	return true;
}


bool ParseShader(StringView shaderPath, 
				 DynArray<ShaderField>* pushConstants, 
				 DynArray<ShaderField>* vertexLayout, 
				 DynArray<ConstantBufferLayout>* constantBufferLayouts,
				 DynArray<ShaderSamplerField>* samplerFields,
				 DynArray<ShaderOutputField>* outputFields,
				 ShaderConfigInfo* shaderConfig,
				 String* vertexSource,
				 String* pixelSource)
{
	TextFileHandler handler;
	if (!OpenFileHandler(shaderPath, &handler))
	{
		LogWarn("[shaderparser] Unable to open shader file '%s'!", shaderPath.data);
		return false;
	}

	*shaderConfig = { };

	// put some reasonable defaults
	shaderConfig->depthWriteEnable = true;
	shaderConfig->depthTestEnable = true;
	shaderConfig->depthCompareOp = CompareOp_Less_Or_Equal;


	ShaderParserMode mode = ShaderParserMode_Metadata;

	ShaderMetadataParseMode metadataMode = ShaderMetadataParse_None;
	String metaBlockName;

	vertexSource->Resize(0);
	pixelSource->Resize(0);

	// ~Todo figure out how to deal with bindings programatically
	// (and across apis as well, cause we generate them so its up to our code, so we can do cool things!)

	const int First_Sampler_Binding = 15; // ~Hack ~Refactor hardcoded in vulkan renderer currently, use an accessable global

	while (true)
	{
		bool found = false;
		String line = ConsumeNextLine(&handler, &found);
		if (!found) break;

		String trimmed = EatSpaces(line);

		if (trimmed.data[0] == '@')
		{
			String act = Substr(trimmed, 1, trimmed.length);
			if (strcmp(Substr(act, 0, 4).data, "type") == 0)
			{
				String modeName = Substr(act, 5, act.length);	

				if (strcmp(modeName.data, "VertexShader") == 0)
					mode = ShaderParserMode_VertexShader;
				else if (strcmp(modeName.data, "PixelShader") == 0)
					mode = ShaderParserMode_PixelShader;
				else
					LogWarn("[shaderparser] Unknown mode '%s' in file %s", modeName.data, shaderPath.data);
			}
		}
		else
			switch (mode)
			{
				case ShaderParserMode_Metadata:
				{
					if (trimmed.data[0] == '#')
					{
						// open paren
						int paren = FindStringFromLeft(trimmed, '(');
						bool hasParen = paren != -1;
						if (paren == -1) paren = trimmed.length;

						String metaModeName = Substr(trimmed, 1, paren);

						if (hasParen)
						{
							metaBlockName = Substr(trimmed, paren + 1, trimmed.length - 1);
						}

						if (strcmp(metaModeName.data, "PushConstants") == 0) 		metadataMode = ShaderMetadataParse_PushConstants;
						else if (strcmp(metaModeName.data, "VertexLayout") == 0) 	metadataMode = ShaderMetadataParse_VertexLayout;
						else if (strcmp(metaModeName.data, "Constants") == 0) 		metadataMode = ShaderMetadataParse_Constants;
						else if (strcmp(metaModeName.data, "Samplers") == 0) 		metadataMode = ShaderMetadataParse_Samplers;
						else if (strcmp(metaModeName.data, "Output") == 0) 			metadataMode = ShaderMetadataParse_Output;
						else if (strcmp(metaModeName.data, "Config") == 0)			metadataMode = ShaderMetadataParse_Config;
						else LogWarn("[shaderparser] unknown shader metadata block '%s'", metaModeName.data);
					}
					else
					{
						if (trimmed.length > 0)
							switch (metadataMode)
							{
								case ShaderMetadataParse_PushConstants:
								{
									ShaderVarDecl var;
									if (ParseVarDecl(trimmed, &var))
									{
										ShaderFieldTypeDef typeDef = VarDeclToShaderFieldTypeDef(var);

										ShaderField field {};

										// Name is a temporaary string, so its copied so its persistent in memory (not dealloacted at the frame end)
										// But type is not a string so it does not need to copied
										field.name = CopyString(var.name); 
										field.typeDef = typeDef;
										ArrayAdd(pushConstants, field);
									}
									else
										LogWarn("[shaderparser] Failed to parse Push Constant! <<%s>> in shader '%s'", trimmed.data, shaderPath.data);

									break;
								}
								case ShaderMetadataParse_VertexLayout:
								{
									ShaderVarDecl var;
									if (ParseVarDecl(trimmed, &var))
									{
										Assert(!var.isArray); // Not supported, might be possible though!

										ShaderFieldTypeDef typeDef = VarDeclToShaderFieldTypeDef(var);

										ShaderField field {};

										// Name is a temporaary string, so its copied so its persistent in memory (not dealloacted at the frame end)
										// But type is not a string so it does not need to copied
										field.name = CopyString(var.name); 
										field.typeDef = typeDef;
										ArrayAdd(vertexLayout, field);
									}
									else
										LogWarn("[shaderparser] Failed to parse Vertex Layout! <<%s>> in shader '%s'", trimmed.data, shaderPath.data);

									break;
								}
								case ShaderMetadataParse_Constants:
								{
									ShaderVarDecl var;
									if (ParseVarDecl(trimmed, &var))
									{
										ShaderFieldTypeDef typeDef = VarDeclToShaderFieldTypeDef(var);

										ConstantBufferLayout* layout = NULL;
										For (*constantBufferLayouts)
										{
											if (Equals(it->name, metaBlockName))
											{
												layout = it;
												break;
											}
										}

										if (!layout)
										{
											ConstantBufferLayout nyLayout = {};
											nyLayout.name = CopyString(metaBlockName);
											nyLayout.fields = MakeDynArray<ShaderField>();
											layout = ArrayAdd(constantBufferLayouts, nyLayout);
										}

										ShaderField field {};
										field.name = CopyString(var.name);
										field.typeDef = typeDef;
										ArrayAdd(&layout->fields, field);
									}
									break;
								}
								case ShaderMetadataParse_Samplers:
								{
									ShaderVarDecl var;
									ParseVarDecl(trimmed, &var);
									Assert(!var.isArray); // Not supported, might be possible though!

									ShaderSamplerType type = NameToShaderSamplerType(var.type);

									ShaderSamplerField samplerField {};
									samplerField.name = CopyString(var.name);
									samplerField.type = type;
									samplerField.binding = samplerFields->size + First_Sampler_Binding;
									ArrayAdd(samplerFields, samplerField);
									break;
								}
								case ShaderMetadataParse_Output:
								{
									int openBracket = FindStringFromLeft(trimmed, '[');
									int closeBracket = FindStringFromLeft(trimmed, ']');

									if (openBracket == -1 || closeBracket == -1)
									{
										LogWarn("Failed to parse Output entry '%s' in shader '%s'", trimmed.data, shaderPath.data);
										break;
									}

									String indexStr = Substr(trimmed, openBracket + 1, closeBracket);
									// ~Refactor custom ToInt function in strings
									int index = atoi(indexStr.data);

									String varDec = EatSpaces(Substr(trimmed, closeBracket + 1, trimmed.length));

									ShaderVarDecl var;
									if (ParseVarDecl(varDec, &var))
									{
										ShaderFieldTypeDef typeDef = VarDeclToShaderFieldTypeDef(var);

										ShaderOutputField field {};
										field.index = index;
										field.name = CopyString(var.name);
										field.typeDef = typeDef;
										ArrayAdd(outputFields, field);
									}
									else
										LogWarn("[shaderparser] Failed to parse Output Field! <<%s>> in shader '%s'", trimmed.data, shaderPath.data);
									break;
								}
								case ShaderMetadataParse_Config:
								{
									Array<String> str = BreakString(trimmed, ' ');
									if (str.size != 2) {
										LogWarn("[shaderparser] Failed to parse Config entry '%s' in shader '%s'", trimmed.data, shaderPath.data);
										break;
									}
									
									String field = str[0];
									String value = str[1];

									if (strcmp(field.data, "fillMode") == 0)
									{
										PolygonFillMode fillMode;
										
										EnumParse(fillMode, PolygonMode_Fill)
										else EnumParse(fillMode, PolygonMode_Line)
										else EnumParse(fillMode, PolygonMode_Point)
										else {
											LogWarn("[shaderparser] Unknown polygon fill mode: '%s'", value.data);
											break;
										}
										shaderConfig->fillMode = fillMode;
									}
									else if (strcmp(field.data, "frontFace") == 0)
									{
										FrontFace frontFace;

										EnumParse(frontFace, FrontFace_CW)
										else EnumParse(frontFace, FrontFace_CCW)
										else {
											LogWarn("[shaderparser] Unknown front face mode: '%s'", value.data);
											break;
										}
										shaderConfig->frontFace = frontFace;
									}
									else if (strcmp(field.data, "lineWidth") == 0)
									{
										shaderConfig->lineWidth = atof(field.data); // ~Custom number parsing
									}
									else if (strcmp(field.data, "cullMode") == 0)
									{
										CullMode cullMode;
										EnumParse(cullMode, Cull_None)
										else EnumParse(cullMode, Cull_Front)
										else EnumParse(cullMode, Cull_Back)
										else EnumParse(cullMode, Cull_FrontAndBack)
										else {
											LogWarn("[shaderparser] Unknown cull mode: '%s'", value.data);
											break;
										}
										shaderConfig->cullMode = cullMode;
									}
									else if (strcmp(field.data, "blendEnable") == 0)
									{
										bool blend = false;
										if (strcmp(value.data, "true") == 0) blend = true;
										else if (strcmp(value.data, "false") == 0) blend = false;
										else {
											LogWarn("[shaderparser] Unknown blend enable mode: '%s'", value.data);
											break;
										}
										shaderConfig->blendEnable = blend;
									}
									else if (strcmp(field.data, "blendSrcColor") == 0)
									{
										if (!ParseBlendFactor(value, &shaderConfig->blendSrcColor)) {
											LogWarn("[shaderparser] Unknown blend factor for blendSrcColor: '%s'", value.data);
											break;
										}
									}
									else if (strcmp(field.data, "blendDestColor") == 0)
									{
										if (!ParseBlendFactor(value, &shaderConfig->blendDestColor)) {
											LogWarn("[shaderparser] Unknown blend factor for blendDestColor: '%s'", value.data);
											break;
										}
									}
									else if (strcmp(field.data, "blendOpColor") == 0)
									{
										if (!ParseBlendOp(value, &shaderConfig->blendOpColor)) {
											LogWarn("[shaderparser] Unknown blend op for blendOpColor: '%s'", value.data);
											break;
										}
									}
									else if (strcmp(field.data, "blendSrcAlpha") == 0)
									{
										if (!ParseBlendFactor(value, &shaderConfig->blendSrcAlpha)) {
											LogWarn("[shaderparser] Unknown blend factor for blendSrcAlpha: '%s'", value.data);
											break;
										}
									}
									else if (strcmp(field.data, "blendDestAlpha") == 0)
									{
										if (!ParseBlendFactor(value, &shaderConfig->blendDestAlpha)) {
											LogWarn("[shaderparser] Unknown blend factor for blendDestAlpha: '%s'", value.data);
											break;
										}
									}
									else if (strcmp(field.data, "blendOpAlpha") == 0)
									{
										if (!ParseBlendOp(value, &shaderConfig->blendOpAlpha)) {
											LogWarn("[shaderparser] Unknown blend op for blendOpAlpha: '%s'", value.data);
											break;
										}
									}
									else if (strcmp(field.data, "depthTestEnable") == 0)
									{
										bool depthTestEnable = false;
										if (strcmp(value.data, "true") == 0) depthTestEnable = true;
										else if (strcmp(value.data, "false") == 0) depthTestEnable = false;
										else {
											LogWarn("[shaderparser] Unknown depth test enable mode: '%s'", value.data);
											break;
										}
										shaderConfig->depthTestEnable = depthTestEnable;
									}
									else if (strcmp(field.data, "depthWriteEnable") == 0)
									{
										bool depthWriteEnable = false;
										if (strcmp(value.data, "true") == 0) depthWriteEnable = true;
										else if (strcmp(value.data, "false") == 0) depthWriteEnable = false;
										else {
											LogWarn("[shaderparser] Unknown depth write enable mode: '%s'", value.data);
											break;
										}
										shaderConfig->depthWriteEnable = depthWriteEnable;
									}
									else if (strcmp(field.data, "depthCompareOp") == 0)
									{
										CompareOp depthCompareOp;
										EnumParse(depthCompareOp, CompareOp_Never)
										else EnumParse(depthCompareOp, CompareOp_Less)
										else EnumParse(depthCompareOp, CompareOp_Equal)
										else EnumParse(depthCompareOp, CompareOp_Less_Or_Equal)
										else EnumParse(depthCompareOp, CompareOp_Greater)
										else EnumParse(depthCompareOp, CompareOp_Not_Equal)
										else EnumParse(depthCompareOp, CompareOp_Greater_Or_Equal)
										else EnumParse(depthCompareOp, CompareOp_Always)
										else {
											LogWarn("[shaderparser] Unknown depth compare op: '%s'", value.data);
											break;
										}
										shaderConfig->depthCompareOp = depthCompareOp;
									}
									else {
										LogWarn("[shaderparser] Unknown field name '%s'", field.data);
										break;
									}

									
									break;
								}
							}
					}

					break;
				}

				case ShaderParserMode_VertexShader:
				{
					vertexSource->Concat(TPrint("%s\n", line.data));
					break;
				}

				case ShaderParserMode_PixelShader:
				{
					pixelSource->Concat(TPrint("%s\n", line.data));
					break;
				}
			}
	}

	CloseFileHandler(&handler);
	return true;
}