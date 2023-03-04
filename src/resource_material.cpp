#include "resource.h"

MaterialId LoadMaterial(StringView name)
{
	TextFileHandler handler;
	if (!OpenFileHandler(name, &handler))
	{
		LogWarn("[res] Tried to read material that does not exist!");
		return Null_Material;
	}

	StringView shaderPath;
	MaterialId id = Null_Material;
	Material* material = NULL;
	bool shaderPresent = false;

	RenderSystem* renderSys = &game->renderSystem;

	while (true)
	{
		bool found = false;
		String line = ConsumeNextLine(&handler, &found);
		if (!found)
			break;

		if (line.data[0] == '@')
		{
			shaderPath = Substr(line, 1, line.length);
			shaderPresent = true;

			id = MakeMaterial(renderSys, shaderPath);
			material = LookupMaterial(renderSys, id);
		}
		else
		{
			
			String trimmedLine = EatSpaces(line);
			
			if (trimmedLine.length > 0)
			{
				if (shaderPresent)
				{
					Array<String> parts = BreakString(trimmedLine, ' ');
					
					
					if (parts.size > 0)
					{
						String varName = parts[0];


						// ~Refactor make this a function
						Size offset = 0;
						bool found = false;
						ShaderFieldType type;
						For (material->constantLayout->fields)
						{
							if (strcmp(it->name.data, varName.data) == 0)
							{
								found = true;
								type = it->typeDef.type;
								break;
							}
							offset += UtilShaderFieldElementSize(it->typeDef);
						}

						if (found)
						{
							void* data = NULL;
							switch (type)
							{
								case ShaderField_Float:
								{
									// @@NumberParsing  ~Refactor
									float value = atof(parts[1].data);
									
									*((float*)((u8*)material->memory + offset)) = value;
									break;	
								}
									
								case ShaderField_Float2:
								{
									// @@NumberParsing
									Vec2 value = v2(atof(parts[1].data), atof(parts[2].data));
									
									*((Vec2*)((u8*)material->memory + offset)) = value;
									break;
								}
									
								case ShaderField_Float3:
								{
									// @@NumberParsing
									Vec3 value = v3(atof(parts[1].data), atof(parts[2].data), atof(parts[3].data));
									
									*((Vec3*)((u8*)material->memory + offset)) = value;
									break;
								}
									
								case ShaderField_Float4:
								{
									// @@NumberParsing
									Vec4 value = v4(atof(parts[1].data), atof(parts[2].data), atof(parts[3].data), atof(parts[4].data));
									


									*((Vec4*)((u8*)material->memory + offset)) = value;
									break;
								}
								
								case ShaderField_Int:
								{
									// @@NumberParsing
									int value = atoi(parts[1].data);

									*((int*)((u8*)material->memory) + offset) = value;
									break;
								}

								default:
									LogWarn("[res] Attempted to load shader field '%s' of unsupported type in material (%s)", varName.data, name.data);
									
							}
						}
						else
						{
							bool samplerFound = false;
							int binding = 0;

							// maybe its a sampler?
							For (material->shader->samplerFields)
							{
								if (strcmp(it->name.data, varName.data) == 0)
								{
									samplerFound = true;
									binding = it->binding;
									break;
								}
							}

							if (samplerFound)
							{								
								bool foundSamplerSlot = false;
								MaterialSamplerSlot* samplerSlot = NULL;
								For (material->samplers)
								{
									if (it->binding == binding)
									{
										foundSamplerSlot = true;
										samplerSlot = it;
										break;
									}
								}

								if (foundSamplerSlot)
								{
									// parse the sampler
									
									// load the texture
									TextureResource* tex = LoadTexture(parts[1]);

									if (tex) {
										TextureSampler sampler {};
										sampler.samplerHandle = ProcureSuitableSampler(renderSys->renderer, Filter_Linear, Filter_Linear, SamplerAddress_Repeat, SamplerAddress_Repeat, SamplerAddress_Repeat, SamplerMipmap_Linear);
										sampler.textureHandle = tex->textureHandle;

										samplerSlot->sampler = sampler;
									}
									else
									{
										LogWarn("[res] Failed to load texture '%s' for material sampler '%s' in (%s)", parts[1].data, varName.data, name.data);
									}
								}
								else
								{
									LogWarn("[res] Could not match material sampler '%s' with binding slot %d in material %s", varName.data, binding, name.data);
								}
							}
							else
							{
								LogWarn("[res] unknown variable '%s' while parsing material (%s)", varName.data, name.data);
							}
						}

					}
				}
				else
				{
					LogWarn("[res] No shader specified at (%s) in material (%s)", trimmedLine.data, name.data);
				}
			}

		}
	}

	if (!shaderPresent)
	{
		LogWarn("[res] Failed to load material (%s), no shader specified!", name.data);
		return Null_Material;
	}

	CloseFileHandler(&handler);
	Log("[res] loaded material (%s) shader=%s", name.data, shaderPath.data);


	return id;
}