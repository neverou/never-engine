#pragma once

#include "str.h"
#include "array.h"

constexpr u32 Resource_Bucket_Size = 16;

#include "renderer.h"
#include "mesh.h"
#include "material.h"
#include "audio.h"

struct Resource {
	String name;
};



struct ShaderFieldTypeDef
{
	ShaderFieldType type;
	bool isArray;
	int elementCount;
};

struct ShaderField
{
	String name;
	ShaderFieldTypeDef typeDef;
};

struct ShaderOutputField
{
	int index;
	String name;
	ShaderFieldTypeDef typeDef;
};


enum ShaderSamplerType
{
	ShaderSampler_None = 0,
	ShaderSampler_Sampler1D,
	ShaderSampler_Sampler2D,
	ShaderSampler_Sampler3D,
};

struct ShaderSamplerField
{
	int binding;
	String name;
	ShaderSamplerType type;
};

struct ConstantBufferLayout
{
	String name;
	DynArray<ShaderField> fields;
};

struct ShaderResource : public Resource {
	RendererHandle handle;

	// Push Constants
	DynArray<ShaderField> pushConstants;

	// Vertex Layout
	DynArray<ShaderField> vertexLayout;
	
	// Constant Buffers
	DynArray<ConstantBufferLayout> constantBufferLayouts;

	// Samplers
	DynArray<ShaderSamplerField> samplerFields;

	// Output
	DynArray<ShaderOutputField> outputFields;
};




struct ModelResource : public Resource {
	TriangleMesh mesh;
};

struct TextureResource : public Resource {
	RendererHandle textureHandle;
};

struct AudioResource : public Resource {
	AudioBuffer buffer;
};

#include "anim.h"

struct AnimResource : public Resource {
	Animation animation;
};

struct ResourceManager {
	BucketArray<Resource_Bucket_Size, ShaderResource>  shaders;
	BucketArray<Resource_Bucket_Size, ModelResource>   models;
	BucketArray<Resource_Bucket_Size, TextureResource> textures;
	BucketArray<Resource_Bucket_Size, AudioResource>   audios;
	BucketArray<Resource_Bucket_Size, AnimResource>    anims;
};

void InitResourceManager();
void DestroyResourceManager();
ResourceManager* GetResourceManager();

ShaderResource* LoadShader(StringView name);
ModelResource* LoadModel(StringView name);
TextureResource* LoadTexture(StringView name);
MaterialId LoadMaterial(StringView name); // technically not a "resource" per se
AudioResource* LoadAudio(StringView name);
AnimResource* LoadAnim(StringView name);


Array<String> AllModelNames();




Size UtilShaderFieldElementSize(ShaderFieldTypeDef type);
int ShaderConstantBufferSlot(ShaderResource* shader, StringView name);
int ShaderSamplerSlot(ShaderResource* shader, StringView name);

// TODO(...); Freeing resources