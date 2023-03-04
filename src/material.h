#pragma once

#include "renderer.h"
#include "maths.h"

struct ShaderResource;
struct RenderSystem;
struct ConstantBufferLayout;


typedef u32 MaterialId;

constexpr MaterialId Null_Material = 0;

struct Material;


struct MaterialSamplerSlot
{
	int binding;
	TextureSampler sampler;
};

struct Material 
{
	ShaderResource* shader;
	
	ConstantBufferLayout* constantLayout;
	RendererHandle constantBuffer;
	u32 constantBinding;

	SArray<MaterialSamplerSlot> samplers;

	void* memory;
};

MaterialId MakeMaterial(RenderSystem* renderSys, StringView shader);
void FreeMaterial(RenderSystem* renderSys, MaterialId material);

void SetFloat (MaterialId material, StringView field, float value);
void SetFloat2(MaterialId material, StringView field, Vec2 value);
void SetFloat3(MaterialId material, StringView field, Vec3 value);
void SetFloat4(MaterialId material, StringView field, Vec4 value);
void SetMat4  (MaterialId material, StringView field, Mat4 value);
void SetInt   (MaterialId material, StringView field, int value);

void SetSampler(MaterialId material, StringView samplerName, TextureSampler sampler);