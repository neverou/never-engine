#pragma once

#include "resource.h"

struct ShaderConfigInfo
{
	// Shader raster info
    PolygonFillMode fillMode;
    FrontFace frontFace;
    float lineWidth;
    CullMode cullMode;
    
    // Shader blend info
    bool blendEnable;
    BlendFactor blendSrcColor;
    BlendFactor blendDestColor;
    BlendOp blendOpColor;
    BlendFactor blendSrcAlpha;
    BlendFactor blendDestAlpha;
    BlendOp blendOpAlpha;

	// Shader depth info
	bool depthTestEnable;
	bool depthWriteEnable;
	CompareOp depthCompareOp;
};


// Not using the shader resource directly because we want to use this in a tool
// that parses shader metadata during the compile step (shader compile step)
bool ParseShader(StringView shaderPath, 
				 DynArray<ShaderField>* pushConstants, 
				 DynArray<ShaderField>* vertexLayout, 
				 DynArray<ConstantBufferLayout>* constantBufferLayouts,
				 DynArray<ShaderSamplerField>* samplerFields,
				 DynArray<ShaderOutputField>* outputFields,
				 ShaderConfigInfo* shaderConfig,
				 String* vertexSource,
				 String* pixelSource);

// ~Todo replace the last 2 with a shader stage array for when we do other types of shaders
