#include "renderer.h"

#include "logger.h"

#if defined(RENDERER_IMPL_D3D11)
#include "d3d11/d3d11_renderer.h"
#endif

#if defined(RENDERER_IMPL_OPENGL)
#include "opengl_renderer.h"
#endif

#if defined(RENDERER_IMPL_VULKAN)
#include "vulkan_renderer.h"
#endif

#include "surface.h"


#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) // SDL platforms

#include <sdl/SDL.h>

// TODO:
// eventually remove this and create the vulkan surface ourselves,
// rather than SDL do it for us
#if defined(RENDERER_IMPL_VULKAN)
#include <sdl/SDL_vulkan.h>
#endif

#endif  // SDL platforms

#include "allocators.h"

RendererSpawner MakeRendererSpawner(Surface* surface) {
    RendererSpawner spawner = {};
    spawner.flags = RENDERER_SPAWNER_FLAG_NONE;
    spawner.surface = surface;
    
	spawner.graphicsProcessors = MakeDynArray<GraphicsProcessor>();
    
	NativeSurfaceInfo native = spawner.surface->GetNativeInfo();
	
	switch (spawner.surface->rendererType) {
        
#if defined(RENDERER_IMPL_D3D11)
		// NOTE: two ifdefs because maybe Xbox support in future, etc

#if defined(PLATFORM_WINDOWS) // windows initialisation
    	case RENDERER_D3D11: {
			// spawner.flags |= RENDERER_SPAWNER_FLAG_SUPPORTS_GRAPHICS_PROCESSOR_AFFINITY;
            
			// TODO: this literally does nothing rn lol,
			// we need to make a directx renderer spawner
            
			// NOTE: spawner.platform_api_instance is gonna be DXGIFactory
			
			break;
		}
#endif
        
#endif // RENDERER_IMPL_D3D11
        
        
        
        
        
#if defined(RENDERER_IMPL_OPENGL)
        
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) // SDL platforms
		case RENDERER_OPENGL: {
			Assert(native.sdlWindow && (native.flags & NATIVE_SURFACE_IS_SDL_BACKEND));
            // ("How do you even have a non-SDL window on windows and linux");
            
			// OpenGL doesnt need to do any preinitialization stuff to init the API because creating the context IS initing the api so yea
			spawner.platformApiInstance = NULL;
			
			break;
		}
#endif
        
#endif // RENDERER_IMPL_OPENGL
        
#if defined(RENDERER_IMPL_VULKAN)
        
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) // SDL platforms
		case RENDERER_VULKAN: {
			Assert(native.sdlWindow && (native.flags & NATIVE_SURFACE_IS_SDL_BACKEND));
            // ("really how did u mess this up to the point where you dont have an SDL window lol");
            
			
			u32 instanceExtensionCount;
			// Query extension count and allocate space for the extension list
			SDL_Vulkan_GetInstanceExtensions(native.sdlWindow, &instanceExtensionCount, NULL);
            
			auto instanceExtensions = MakeDynArray<const char*>(instanceExtensionCount, Frame_Arena);
			defer(FreeDynArray(&instanceExtensions));
            
			// Get the extensions
			SDL_Vulkan_GetInstanceExtensions(native.sdlWindow, &instanceExtensionCount, instanceExtensions.data);
            
		    PopulateRendererSpawnerForVulkan(&spawner, instanceExtensions);
			
            break;
		}
        
#endif
        
#endif
        
		default: {
		    FatalError("Renderer type unsupported on your platform!");
			break;
		}
	}
    
	return spawner;
}

void DestroyRendererSpawner(RendererSpawner* spawner) {
	FreeDynArray(&spawner->graphicsProcessors);
}

Renderer* SpawnRenderer(RendererSpawnInfo* spawnInfo, Allocator* allocator) {
	Renderer* renderer;
	
	NativeSurfaceInfo native = spawnInfo->spawner.surface->GetNativeInfo();
	
	switch (spawnInfo->spawner.surface->rendererType) {
		
#if defined(RENDERER_IMPL_D3D11)
        
		// NOTE: two ifdefs because maybe Xbox support in future, etc
		
#if defined(PLATFORM_WINDOWS) // windows initialisation
    	
        case RENDERER_D3D11: {
			renderer = SpawnD3D11Renderer(native.hwnd);
			break;
		}
        
#endif
#endif // RENDERER_IMPL_D3D11
        
        
        
#if defined(RENDERER_IMPL_OPENGL)
        
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) // SDL platforms
		case RENDERER_OPENGL: {
			Assert(native.sdlWindow && (native.flags & NATIVE_SURFACE_IS_SDL_BACKEND));
            // ("How do you even have a non-SDL window on windows and linux");
            
            // TODO:
            // maybe change the layer thing to a struct with
            // function pointers and stuff instead of what it is rn
            renderer = SpawnOpenGLRenderer(SpawnOpenGLLayerSDL(native.sdlWindow), spawnInfo);
			break;
		}
#endif
        
#endif // RENDERER_IMPL_OPENGL
        
#if defined(RENDERER_IMPL_VULKAN)
        
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) // SDL platforms
		case RENDERER_VULKAN: {
			// TODO:
			// eventually remove this and create the vulkan surface ourselves,
			// rather than SDL do it for us,
			// especially here, so we dont have to deal with the appending instance extensions and bare minimum stuff,
			// instead we just pass the window data
			
			Assert(native.sdlWindow && (native.flags & NATIVE_SURFACE_IS_SDL_BACKEND));
            
            VulkanRendererSpawnInfo vkSpawnInfo = {};
            
			vkSpawnInfo.rendererSpawnInfo = *spawnInfo;
			Assert(SDL_Vulkan_CreateSurface(native.sdlWindow, cast(VkInstance, spawnInfo->spawner.platformApiInstance), &vkSpawnInfo.vkSurface));
			
			renderer = SpawnVulkanRenderer(&vkSpawnInfo, allocator);

            break;
		}
        
#endif
        
#endif

		default: {
		    LogError("[gfx] Tried to create a renderer of unknown type!");
			Assert(false);

            break;
		}
	}
    
	DestroyRendererSpawner(&spawnInfo->spawner);

	// common renderer init
	renderer->samplerCache = MakeDynArray<RendererHandle>();
	
	renderer->renderTargetCacheKeys   = MakeDynArray<RenderTargetCacheKey>();
	renderer->renderTargetCacheValues = MakeDynArray<RendererHandle>();

	return renderer;
}

void DeleteRenderer(Renderer* renderer) {
    // common renderer delete

	// delete sampler cache
	For (renderer->samplerCache)
		renderer->FreeSampler(*it);
	FreeDynArray<RendererHandle>(&renderer->samplerCache);

	For (renderer->renderTargetCacheKeys)
		FreeArray((SArray<RenderTargetDescription>*)&it->colorAttachments);
	For (renderer->renderTargetCacheValues)
		renderer->FreeRenderTarget(*it);
	FreeDynArray(&renderer->renderTargetCacheKeys);
	FreeDynArray(&renderer->renderTargetCacheValues);



	
	switch(renderer->type) {
#if defined(RENDERER_IMPL_D3D11)
		case RENDERER_D3D11: {
			DeleteD3D11Renderer(cast(D3D11Renderer*, renderer));
			break;
		}
#endif // RENDERER_IMPL_D3D11
        
        
#if defined(RENDERER_IMPL_OPENGL)
		case RENDERER_OPENGL: {
			DeleteOpenGLRenderer(cast(OpenGLRenderer*, renderer));
			break;
		}
#endif // RENDERER_IMPL_OPENGL
        
#if defined(RENDERER_IMPL_VULKAN)
		case RENDERER_VULKAN: {
			DeleteVulkanRenderer(cast(VulkanRenderer*, renderer));
			break;
		}
#endif // RENDERER_IMPL_VULKAN
        
        
		default: {
            LogError("[gfx] Tried to delete renderer of unknown type!");
            Assert(false);
            break;
        }
	}
}


// RendererSystem MakeRendererSystem(Surface* surface, Allocator* allocator) {
//     RendererSystem rendererSystem;
//     rendererSystem.surface = surface;
    
//     RendererSpawnInfo spawnInfo;
//     spawnInfo.spawner = MakeRendererSpawner(surface);
    
//     // cant set the gpu on opengl (or d3d11 yet) so this code is needed to not mess everything up lol
//     if (spawnInfo.spawner.flags & RENDERER_SPAWNER_FLAG_SUPPORTS_GRAPHICS_PROCESSOR_AFFINITY)
//         spawnInfo.chosenGraphicsProcessor = spawnInfo.spawner.graphicsProcessors.data[0];
    
//     Log("[gfx] Spawner Flags: %lu", spawnInfo.spawner.flags);
    
//     for (size_t it = 0; it < spawnInfo.spawner.graphicsProcessors.size; it++) {
//         GraphicsProcessor proc = spawnInfo.spawner.graphicsProcessors.data[it];
//         Log("[gfx]    Device Found: %s", proc.deviceName);
//     }
    
//     rendererSystem.renderer = SpawnRenderer(&spawnInfo, allocator);
    
// 	return rendererSystem;
// }

// void DestroyRendererSystem(RendererSystem* rendererSystem) {
// 	DeleteRenderer(rendererSystem->renderer);
// }




size_t RenderFormatBytesPerPixel(RenderFormat format)
{
	switch (format)
	{
		case FORMAT_R8_SNORM:
		case FORMAT_R8_UNORM:
		case FORMAT_R8_SINT:
		case FORMAT_R8_UINT:				return 1;

		case FORMAT_R16_SNORM:
		case FORMAT_R16_UNORM:
		case FORMAT_R16_SINT:
		case FORMAT_R16_UINT:
		case FORMAT_R16_FLOAT:				return 2;
		
		case FORMAT_R32_SINT:
		case FORMAT_R32_UINT:
		case FORMAT_R32_FLOAT:				return 4;
		
		case FORMAT_R8G8_SNORM:
		case FORMAT_R8G8_UNORM:
		case FORMAT_R8G8_SINT:
		case FORMAT_R8G8_UINT:				return 2;
		
		case FORMAT_R16G16_SNORM:
		case FORMAT_R16G16_UNORM:
		case FORMAT_R16G16_SINT:
		case FORMAT_R16G16_UINT:
		case FORMAT_R16G16_FLOAT:			return 2 * 2;
		
		case FORMAT_R32G32_SINT:
		case FORMAT_R32G32_UINT:
		case FORMAT_R32G32_FLOAT:			return 2 * 4;

		case FORMAT_R32G32B32_SINT:
		case FORMAT_R32G32B32_UINT:
		case FORMAT_R32G32B32_FLOAT:		return 3 * 4;

		case FORMAT_R8G8B8A8_SNORM:
		case FORMAT_R8G8B8A8_UNORM:
		case FORMAT_R8G8B8A8_SINT:
		case FORMAT_R8G8B8A8_UINT:
		case FORMAT_R8G8B8A8_UNORM_SRGB:	return 4 * 1;

		case FORMAT_R16G16B16A16_SNORM:
		case FORMAT_R16G16B16A16_UNORM:
		case FORMAT_R16G16B16A16_SINT:
		case FORMAT_R16G16B16A16_UINT:
		case FORMAT_R16G16B16A16_FLOAT:		return 4 * 2;

		case FORMAT_R32G32B32A32_SINT:
		case FORMAT_R32G32B32A32_UINT:
		case FORMAT_R32G32B32A32_FLOAT:		return 4 * 4;

		case FORMAT_B8G8R8A8_SNORM:
		case FORMAT_B8G8R8A8_UNORM:
		case FORMAT_B8G8R8A8_SINT:
		case FORMAT_B8G8R8A8_UINT:
		case FORMAT_B8G8R8A8_UNORM_SRGB: 	return 4;
		
		case FORMAT_D32_FLOAT:
		case FORMAT_D32_FLOAT_S8_UINT:
		case FORMAT_D24_UNORM_S8_UINT:		return 4;
	}

	Assert(false);
	return 0;
}


bool FormatHasDepth(RenderFormat format)
{
	return 	(format == FORMAT_D32_FLOAT)  		 ||
    		(format == FORMAT_D32_FLOAT_S8_UINT) ||
    		(format == FORMAT_D24_UNORM_S8_UINT);
}
    

// framegraphs

void _InitFgPass(FgPass* pass, StringView name, PassRenderFunc renderFunc)
{
	Assert(pass != NULL);

	pass->_name = CopyString(name);
	pass->_renderFunc = renderFunc;

	pass->reads  = MakeDynArray<FgDependencyRead*>();
	pass->writes = MakeDynArray<FgDependencyWrite*>();

	pass->dependencies = MakeDynArray<FgPass*>();
}

void _DestroyFgPass(FgPass* pass, String name)
{
	Assert(pass != NULL);

	FreeString(&pass->_name);

	FreeDynArray(&pass->reads);
	FreeDynArray(&pass->writes);

	FreeDynArray(&pass->dependencies);
}

void _FgBufferRead(FgPass* pass, FgDependencyRead* read, TextureLayout layout)
{
	Assert(pass != NULL);
	Assert(read != NULL);

	*read = FgDependencyRead {
		.layout = layout
	};
	ArrayAdd(&pass->reads, read);
}

void _FgBufferWrite(FgPass* pass, FgDependencyWrite* write, RendererHandle texture)
{
	Assert(pass != NULL);
	Assert(write != NULL);
	Assert(texture != RendererHandle_None);

	*write = FgDependencyWrite {
		.texture = texture
	};
	ArrayAdd(&pass->writes, write);
}

void _FgAttach(FgPass* from, FgDependencyWrite* fromWrite, FgPass* to, FgDependencyRead* toRead)
{
	Assert(from != NULL);
	Assert(fromWrite != NULL);
	Assert(to != NULL);
	Assert(toRead != NULL);

	// if "from" isnt already a dependency of "to" then add it
	if (ArrayFind( &to->dependencies, from ) == -1)
		ArrayAdd( &to->dependencies, from );

	toRead->input = fromWrite;
}

FgBuiltGraph FgBuild(Renderer* renderer, Array<FgPass*> passes)
{
	struct LayoutTable
	{
		DynArray<FgDependencyWrite*> writes; // keys 
		DynArray<TextureLayout> layouts;     // values
	};

	struct PassLayoutTableTuple
	{
		FgPass* pass;
		LayoutTable layoutTable;
	};

	auto remaining = MakeDynArray<FgPass*>(passes.size, Frame_Arena);
	auto resolved  = MakeDynArray<FgPass*>(0, Frame_Arena);
	Memcpy(remaining.data, passes.data, remaining.size * sizeof(FgPass*)); // ~Refactor @@ArrayCopy

	auto commands = MakeDynArray<FgGraphCommand>(0, Frame_Arena);

	while (remaining.size)
	{
		Log("[fg] todo: %d", remaining.size);
		auto stagePasses = MakeDynArray<PassLayoutTableTuple>(0, Frame_Arena);

		// Double buffer resolved so the following loop has to wait an iteration before resolving a dependency
		auto prevResolved  = MakeArray<FgPass*>(resolved.size, Frame_Arena);
		Memcpy(prevResolved.data, resolved.data, resolved.size * sizeof(FgPass*)); // ~Refactor @@ArrayCopy

		ForIdx (remaining, idx)
		{
			FgPass* pass = remaining[idx];

			u32 dependencyCount = 0;
			For (pass->dependencies)
			{
				if (ArrayFind( &prevResolved, *it ) == -1)
					dependencyCount++;
			}

			if (dependencyCount == 0)
			{
				ArrayAdd(&resolved, pass);
				ArrayRemoveAt(&remaining, idx--);

				LayoutTable table {
					.writes = MakeDynArray<FgDependencyWrite*>(0, Frame_Arena),
					.layouts = MakeDynArray<TextureLayout>(0, Frame_Arena),
				};

				For (pass->reads)
				{
					FgDependencyRead* read = *it;
					
					Size existingWrite = ArrayFind( &table.writes, read->input );
					if (existingWrite == -1)
					{
						ArrayAdd(&table.writes, read->input);
						ArrayAdd(&table.layouts, read->layout);
					}
					else
					{
						// Cannot read from the same output with different layouts on the same pass
						Assert(table.layouts[existingWrite] == read->layout);
					}
				}

				ArrayAdd(&stagePasses, PassLayoutTableTuple {
					.pass = pass,
					.layoutTable = table
				});

				Log("[fg] resolve --> %s", pass->_name.data);
			}
		}

		LayoutTable builderLayout { 
			.writes = MakeDynArray<FgDependencyWrite*>(0, Frame_Arena),
			.layouts = MakeDynArray<TextureLayout>(0, Frame_Arena),
		};

		LayoutTable initialLayout { 
			.writes = MakeDynArray<FgDependencyWrite*>(0, Frame_Arena),
			.layouts = MakeDynArray<TextureLayout>(0, Frame_Arena),
		};

		while (stagePasses.size)
		{
			LayoutTable* nextLayout = NULL;

			ForIdx (stagePasses, idx)
			{
				auto* it = &stagePasses[idx];
				bool layoutCompatible = true;

				ForIdx (it->layoutTable.writes, layoutIdx)
				{
					Size builderWriteIdx = ArrayFind(&builderLayout.writes, it->layoutTable.writes[layoutIdx]);

					if (builderWriteIdx == -1)
					{
						layoutCompatible = false;
						nextLayout = &it->layoutTable;
						break;
					}
					else
					{
						if (builderLayout.layouts[builderWriteIdx] != it->layoutTable.layouts[layoutIdx])
						{
							layoutCompatible = false;
							nextLayout = &it->layoutTable;
							break;
						}
					}
				}

				if (layoutCompatible)
				{
					Log("[fg] %s is compatible!", it->pass->_name.data);
					ArrayAdd(&commands, FgGraphCommand {
						.type = FgGraphCommand_ExecutePass,
						.pass = it->pass,
					});
					ArrayRemoveAt(&stagePasses, idx--);
				}
			}

			Assert((stagePasses.size > 0) == (nextLayout != NULL));

			// apply the next layout
			if (nextLayout)
			{
				auto texBarrier = MakeDynArray<PipelineTextureBarrier>();
				// ~Todo @@Memory free this

				ForIdx (nextLayout->writes, layoutIdx)
				{
					Size builderWriteIdx = ArrayFind(&builderLayout.writes, nextLayout->writes[layoutIdx]);

					if (builderWriteIdx == -1)
					{
						ArrayAdd(&builderLayout.writes, nextLayout->writes[layoutIdx]);
						ArrayAdd(&builderLayout.layouts, nextLayout->layouts[layoutIdx]);
						
						ArrayAdd(&initialLayout.writes, nextLayout->writes[layoutIdx]);
						ArrayAdd(&initialLayout.layouts, nextLayout->layouts[layoutIdx]);

						ArrayAdd(&texBarrier, PipelineTextureBarrier {
							.texture = nextLayout->writes[layoutIdx]->texture,
							.inLayout = nextLayout->layouts[layoutIdx],
							.outLayout = nextLayout->layouts[layoutIdx],
						});
					}
					else
					{
						if (builderLayout.layouts[builderWriteIdx] != nextLayout->layouts[layoutIdx])
						{
							ArrayAdd(&texBarrier, PipelineTextureBarrier {
								.texture = nextLayout->writes[layoutIdx]->texture,
								.inLayout = builderLayout.layouts[builderWriteIdx],
								.outLayout = nextLayout->layouts[layoutIdx],
							});

							builderLayout.layouts[builderWriteIdx] = nextLayout->layouts[layoutIdx];
						}
					}
				}

				ArrayAdd(&commands, FgGraphCommand {
					.type = FgGraphCommand_PipelineBarrier,
					.barrier = {
						.textureBarriers = texBarrier,
					},
				});

				Log("[fg] Layout change!");
			}
		}

		ForIdx (initialLayout.writes, idx)
			initialLayout.writes[idx]->builtLayout = initialLayout.layouts[idx];
	}

	For (passes)
	{
		FgPass* fgPass = *it;

		Log("[fg] Building pass -> %s (reads: %lu , writes: %lu)", fgPass->_name.data, fgPass->reads.size, fgPass->writes.size);
	

		bool hasDepthAttachment = false;
		RenderPassDepthAttachmentDescription depthAttachment { };
		
		auto colorAttachments = MakeDynArray<RenderPassColorAttachmentDescription>(0, Frame_Arena);
		For (fgPass->writes)
		{
			FgDependencyWrite* write = *it;

			// ~Refactor (...) determine whether we want this allowed
			TextureLayout inputLayout = TextureLayout_Undefined;
			TextureLayout outputLayout = write->builtLayout;

			ForIt (fgPass->reads, readIt)
			{
				FgDependencyRead* read = *readIt;
				if (read->input->texture != write->texture) continue;

				Log("[fg] ... Write has a read it depends on!");

				inputLayout = read->layout;
			}

			Texture* texture = renderer->LookupTexture(write->texture);
			Assert((texture->flags & TextureFlags_ColorAttachment) != 0 || (texture->flags & TextureFlags_DepthStencilAttachment) != 0);

			if (texture->flags & TextureFlags_DepthStencilAttachment)
			{
				Assert(!hasDepthAttachment);

				hasDepthAttachment = true;
				depthAttachment = {
					.format = texture->format,
					.sampleCount = 1,

					.loadOp = (inputLayout == TextureLayout_Undefined ? RENDERATTACHOP_CLEAR : RENDERATTACHOP_LOAD), // ~Todo (...) replace this and put some flags on the output to say what load op we want on the existing texture,
					.storeOp = RENDERATTACHOP_STORE,

					.inLayout = inputLayout,
					.outLayout = outputLayout,
				};
			}
			else if (texture->flags & TextureFlags_ColorAttachment)
			{
				ArrayAdd(&colorAttachments, {
					.format = texture->format,
					.sampleCount = 1,

					.loadOp    = (inputLayout == TextureLayout_Undefined ? RENDERATTACHOP_CLEAR : RENDERATTACHOP_LOAD), // ~Todo (...) replace this and put some flags on the output to say what load op we want on the existing texture
					.storeOp   = RENDERATTACHOP_STORE,

					.inLayout  = inputLayout,
					.outLayout = outputLayout,
				});					
			}
		}

		RenderPassDescription passDesc {
			.colorAttachments = colorAttachments,
			.hasDepthAttachment = hasDepthAttachment,
			.depthAttachment = depthAttachment,
		};

		fgPass->builtPass = renderer->CreatePass(&passDesc);
	}

	FgBuiltGraph graph { };
	graph.commands = MakeArray<FgGraphCommand>(commands.size);
	Memcpy(graph.commands.data, commands.data, commands.size * sizeof(FgGraphCommand));
	return graph;
}


void FgFree(FgBuiltGraph* graph)
{
	FreeArray(&graph->commands);
}

void FgDo(FgBuiltGraph* graph, Renderer* renderer)
{
	For (graph->commands)
	{
		FgGraphCommand cmd = *it;

		switch (cmd.type)
		{
			case FgGraphCommand_ExecutePass: {
				cmd.pass->_renderFunc(renderer, cmd.pass);
				break;
			}
			case FgGraphCommand_PipelineBarrier: {
				renderer->CmdPipelineBarrier(cmd.barrier);
				break;
			}
		}
	}
}


// sampler cache
RendererHandle ProcureSuitableSampler(
	Renderer* renderer, 
	FilterMode magFilter, 
	FilterMode minFilter, 
	SamplerAddressMode addressModeU, 
	SamplerAddressMode addressModeV, 
	SamplerAddressMode addressModeW, 
	SamplerMipmapMode mipmapMode)
{
	For (renderer->samplerCache)
	{
		Sampler* sampler = renderer->LookupSampler(*it);

		if (sampler->magFilter    == magFilter || 
			sampler->minFilter    == minFilter || 
			sampler->addressModeU == addressModeU || 
			sampler->addressModeV == addressModeV || 
			sampler->addressModeW == addressModeW || 
			sampler->mipmapMode   == mipmapMode)
			return *it;
	}


	// make a new one
	RendererHandle handle = renderer->CreateSampler(magFilter, minFilter, addressModeU, addressModeV, addressModeW, mipmapMode);
	Log("[renderer] creating new cached sampler (%lu)", handle);
	ArrayAdd(&renderer->samplerCache, handle);
	return handle;
}


// render target cache
RendererHandle ProcureSuitableRenderTarget(Renderer* renderer, RenderTargetDescription desc)
{
	ForIdx (renderer->renderTargetCacheKeys, i)
	{
		RenderTargetCacheKey key = renderer->renderTargetCacheKeys[i];
		if (key.depthAttachment == desc.depthAttachment 				&& 
			key.colorAttachments.size == desc.colorAttachments->size	&&
			key.width  == desc.width &&
			key.height == desc.height)
		{
			bool failed = false;
			ForIdx (key.colorAttachments, j)
				failed |= (key.colorAttachments[j] != (*desc.colorAttachments)[j]);
			if (failed) continue;

			return renderer->renderTargetCacheValues[i];
		}
	}
	
	RendererHandle handle = renderer->CreateRenderTarget(&desc);
	Log("[renderer] creating new cached render target (%lu)", handle);

	RenderTargetCacheKey persistentDesc = { 
		.colorAttachments = MakeArray<RendererHandle>(desc.colorAttachments->size),
		.depthAttachment = desc.depthAttachment,
		.width = desc.width,
		.height = desc.height
	};

	Memcpy(persistentDesc.colorAttachments.data, desc.colorAttachments->data, sizeof(RendererHandle) * persistentDesc.colorAttachments.size);

	ArrayAdd(&renderer->renderTargetCacheKeys, persistentDesc);
	ArrayAdd(&renderer->renderTargetCacheValues, handle);

	return handle;
}