#include "material.h"

#include "gfx.h"
#include "resource.h"



#include <string.h>

#include "game.h"

#define Material_Constants_Name "material"

MaterialId MakeMaterial(RenderSystem* renderSys, StringView shaderPath)
{
	ShaderResource* shader = LoadShader(shaderPath);

	Material material { };
	material.shader = shader;

	// look for a material constant buffer
	ConstantBufferLayout* layout = NULL;

	// ~Refactor ~Todo stick this in a function and use it in gfx.cpp instead of hardcoded constant buffer idx
	material.constantBinding = ShaderConstantBufferSlot(shader, Material_Constants_Name);
	if (material.constantBinding == -1)
	{
		LogWarn("[material] Failed to find material constant in shader '%s'", shaderPath.data);
		return Null_Material;
	}

	layout = &shader->constantBufferLayouts[material.constantBinding];
	Assert(layout != NULL);

	Size constantsSize = 0;
	For (layout->fields)
	{
		constantsSize += UtilShaderFieldElementSize(it->typeDef);
	}


	// samplers
	if (shader->samplerFields.size != 0)
	{
		material.samplers = MakeArray<MaterialSamplerSlot>(shader->samplerFields.size);
		int idx = 0;
		For (shader->samplerFields)
		{
			MaterialSamplerSlot slot { };
			slot.binding = it->binding;
			material.samplers[idx++] = slot;
		}
	}



	// ~Todo make this DYNAMIC not STREAM after @@FixMapMemory
	material.constantBuffer = renderSys->renderer->CreateBuffer(NULL, constantsSize, RENDERBUFFER_FLAGS_CONSTANT | RENDERBUFFER_FLAGS_USAGE_STREAM);
	material.constantLayout = layout;
	material.memory = renderSys->renderer->MapBuffer(material.constantBuffer);

	MaterialId id = renderSys->nextMaterialId++;
	ArrayAdd(&renderSys->materialKeys, id);
	ArrayAdd(&renderSys->materials, material);
	return id;
}

void FreeMaterial(RenderSystem* renderSys, MaterialId material)
{
	Size index = ArrayFind(&renderSys->materialKeys, material);
	if (index == -1)
	{
		LogWarn("[rendersys] Attempted to remove material that doesn't exist (id=%u)", material);
		return;
	}

	Material* mat = &renderSys->materials[index];

	

	renderSys->renderer->UnmapBuffer(mat->constantBuffer);
	renderSys->renderer->FreeBuffer(mat->constantBuffer);

	ArrayRemoveAt(&renderSys->materialKeys, index);
	ArrayRemoveAt(&renderSys->materials, index);

	FreeArray(&mat->samplers);
}




intern void WriteConstantBuffer(MaterialId material, StringView field, void* data, u32 size)
{
	auto* renderSys = &game->renderSystem;
	Material* mat = LookupMaterial(renderSys, material);

	Size offset = 0;
	bool found = false;
	For (mat->constantLayout->fields)
	{
		if (Equals(it->name, field))
		{
			found = true;
			break;
		}
		offset += UtilShaderFieldElementSize(it->typeDef);
	}

	if (!found)
	{
		LogWarn("[material] Unknown field in material '%s' of shader '%s'", field.data, mat->shader->name.data);
		return;
	}

	Memcpy(((u8*)mat->memory) + offset, data, size);
}

void SetFloat (MaterialId material, StringView field, float value) 	{ WriteConstantBuffer(material, field, &value, sizeof(float)); }
void SetFloat2(MaterialId material, StringView field, Vec2 value)	{ WriteConstantBuffer(material, field, &value, sizeof(Vec2)); }
void SetFloat3(MaterialId material, StringView field, Vec3 value)	{ WriteConstantBuffer(material, field, &value, sizeof(Vec3)); }
void SetFloat4(MaterialId material, StringView field, Vec4 value)	{ WriteConstantBuffer(material, field, &value, sizeof(Vec4)); }
void SetMat4  (MaterialId material, StringView field, Mat4 value)	{ WriteConstantBuffer(material, field, &value, sizeof(Mat4)); }
void SetInt   (MaterialId material, StringView field, int value)	{ WriteConstantBuffer(material, field, &value, sizeof(int)); }

void SetSampler(MaterialId material, StringView samplerName, TextureSampler sampler)
{
	auto* renderSys = &game->renderSystem;
	Material* mat = LookupMaterial(renderSys, material);

	int binding = 0;

	bool found = false;
	For (mat->shader->samplerFields)
	{
		if (strcmp(it->name.data, samplerName.data) == 0)
		{
			found = true;
			binding = it->binding;
			break;
		}
	}

	if (!found)
	{
		LogWarn("[material] Unknown sampler in material '%s' of shader '%s'", samplerName.data, mat->shader->name.data);
		return;
	}

	bool foundSamplerSlot = false;
	MaterialSamplerSlot* samplerSlot = NULL;
	For (mat->samplers)
	{
		if (it->binding == binding)
		{
			foundSamplerSlot = true;
			samplerSlot = it;
			break;
		}
	}

	if (!foundSamplerSlot)
	{
		LogWarn("[material] Could not match material sampler '%s' with binding slot %d, shader '%s'", samplerName.data, binding, mat->shader->name.data);
		return;
	}

	samplerSlot->sampler = sampler;
}