#include "gfx.h"


#include "maths.h"
#include "game.h"

#include "allocators.h"

#include "resource.h"

#include "logger.h"
#include "gizmos.h"


#define Camera_Constants_Block_Name    ("camera")
#define Animation_Constants_Block_Name ("animation")

intern RendererHandle quadVbo;
intern ShaderResource* copyShader;

intern void RenderActorsInWorld(RenderSystem* renderSystem, World* world, CameraConstants cameraConstants)
{
	auto* renderer =  renderSystem->renderer;
	ForIt (world->entityPools, poolIt) {
		for (EntityPoolIt actorIt = EntityPoolBegin(*poolIt); EntityPoolItValid(actorIt); actorIt = EntityPoolNext(actorIt)) {
			auto* actor = GetEntityFromPoolIt(actorIt);
            
			if (!actor->mesh) continue;
			if (!actor->material) continue;
		
			// cant do anything with zero vertices
			if (actor->mesh->vertices.size == 0) continue;

			// we need to update the frustum culling hull
			if (actor->mesh->invalidated || !actor->frustumCulling.init)
			{
				Vec3 minPos = actor->mesh->vertices[0].position;
				Vec3 maxPos = actor->mesh->vertices[0].position;
				For (actor->mesh->vertices)
				{
					Vec3 v = it->position;
                    
					minPos.x = Min(minPos.x, v.x);
					minPos.y = Min(minPos.y, v.y);
					minPos.z = Min(minPos.z, v.z);
                    
					maxPos.x = Max(maxPos.x, v.x);
					maxPos.y = Max(maxPos.y, v.y);
					maxPos.z = Max(maxPos.z, v.z);
				}
                
				actor->frustumCulling = {
					.init = true,
					.origin = (maxPos + minPos) / v3(2),
					.radius = Length(maxPos - minPos) / 2
				};
			}
			UpdateGpuMesh(actor->mesh, renderer); // validate the mesh if it needs to be validated
            
			// do frustum culling
			{
				Mat4 matrix = EntityWorldXformMatrix(world, actor);
				Vec3 cullOrigin = Mul(actor->frustumCulling.origin, 1, matrix);
                
				Vec3 viewSpace = Mul(cullOrigin, 1, cameraConstants.viewMatrix);
                
				Vec4 ndcNear = Mul(v4(0, 0, 0, 1), Inverse(cameraConstants.projectionMatrix));
				ndcNear = ndcNear / v4(ndcNear.w);
                
				Vec4 ndcFarCorner = Mul(v4(1, 1, 1, 1), Inverse(cameraConstants.projectionMatrix));
				ndcFarCorner = ndcFarCorner / v4(ndcFarCorner.w);
                
				// primitive near plane check //
				float tNear = -(viewSpace.z + ndcNear.z);
				float tFar = (viewSpace.z - ndcFarCorner.z);
                
				// right plane //
				Vec3 nearRight = v3(ndcFarCorner.x, 0, ndcFarCorner.z) * v3(ndcFarCorner.x / ndcFarCorner.z * ndcNear.z);
				Vec3 farRight  = v3(ndcFarCorner.x, 0, ndcFarCorner.z);
				Vec3 norRight  = Normalize(v3(farRight.z - nearRight.z, 0, nearRight.x - farRight.x));
				float tRight   = Dot(norRight, viewSpace) - Dot(norRight, nearRight);
                
				// left plane //
				Vec3 nearLeft = v3(-ndcFarCorner.x, 0, ndcFarCorner.z) * v3(ndcFarCorner.x / ndcFarCorner.z * ndcNear.z);
				Vec3 farLeft  = v3(-ndcFarCorner.x, 0, ndcFarCorner.z);
				Vec3 norLeft  = Normalize(v3(nearLeft.z - farLeft.z, 0, farLeft.x - nearLeft.x));
				float tLeft   = Dot(norLeft, viewSpace) - Dot(norLeft, nearLeft);
                
				// top plane //
				Vec3 nearTop = v3(0, ndcFarCorner.y, ndcFarCorner.z) * v3(ndcFarCorner.y / ndcFarCorner.z * ndcNear.z);
				Vec3 farTop  = v3(0, ndcFarCorner.y, ndcFarCorner.z);
				Vec3 norTop  = Normalize(v3(0, farTop.z - nearTop.z, nearTop.y - farTop.y));
				float tTop   = Dot(norTop, viewSpace) - Dot(norTop, nearTop);
                
				// bottom plane //
				Vec3 nearBottom = v3(0, -ndcFarCorner.y, ndcFarCorner.z) * v3(ndcFarCorner.y / ndcFarCorner.z * ndcNear.z);
				Vec3 farBottom  = v3(0, -ndcFarCorner.y, ndcFarCorner.z);
				Vec3 norBottom  = Normalize(v3(0, nearBottom.z - farBottom.z, farBottom.y - nearBottom.y));
				float tBottom   = Dot(norBottom, viewSpace) - Dot(norBottom, nearBottom);
                
				float df = Max(Max(Max(tNear, tFar), Max(tRight, tLeft)), Max(tBottom, tTop));
                
				// minkowski sum //
				// apply the highest scale axis to the radius in order to encapsulate the biggest sphere of the actor
				if (df > actor->frustumCulling.radius * Max(actor->xform.scale.x, Max(actor->xform.scale.y, actor->xform.scale.z)) )
					continue;
			}
            
			Material* material = LookupMaterial(renderSystem, actor->material);

			RendererHandle shader = material->shader->handle;
			renderer->CmdSetShader(shader);

			// Camera constants
			{
				void* constants = renderer->MapBuffer(renderSystem->cameraConstantsBuffer); // ~Todo: persistently map buffers
				Memcpy(constants, &cameraConstants, sizeof(CameraConstants));
				renderer->UnmapBuffer(renderSystem->cameraConstantsBuffer);

				int cameraConstantsSlot = ShaderConstantBufferSlot(material->shader, Camera_Constants_Block_Name);
				if (cameraConstantsSlot != -1) {
					FixArray<RendererHandle, 1> cameraConstantBuffers;
					cameraConstantBuffers[0] = renderSystem->cameraConstantsBuffer;
					renderer->CmdSetConstantBuffers(shader, cameraConstantsSlot, &cameraConstantBuffers);
				}
			}
            
            // Material constants
			{
				FixArray<RendererHandle, 1> constantBuffers;
				constantBuffers[0] = material->constantBuffer;
				renderer->CmdSetConstantBuffers(shader, material->constantBinding, &constantBuffers);
			}
            
			// Animation
			if (actor->features & EntityFeature_Animator)
			{
				Animator* animator = &actor->animator;
				UpdateGpuAnimator(animator, renderer);

				Assert(animator->armatureBuffer != RendererHandle_None);
				
				int animationConstantsSlot = ShaderConstantBufferSlot(material->shader, Animation_Constants_Block_Name);
				if (animationConstantsSlot != -1) {
					// ~Refactor Single element array macro
					FixArray<RendererHandle, 1> cb;
					cb[0] = animator->armatureBuffer;
					renderer->CmdSetConstantBuffers(shader, animationConstantsSlot, &cb);
				}
			}

            
			// Sampler bind step
			{
				For (material->samplers)
				{
					FixArray<TextureSampler, 1> samplers;
					samplers[0] = it->sampler;
					if (it->sampler.textureHandle && it->sampler.samplerHandle)
					{
						renderer->CmdSetSamplers(shader, it->binding, &samplers);
					}
				}
			}
            

			// Draw the actor

			DrawConstants drawCallInfo;
			drawCallInfo.transformMatrix = EntityWorldXformMatrix(world, actor);
            
			renderer->CmdPushConstants(shader, 0, sizeof(drawCallInfo), &drawCallInfo);
            
			// ~Todo: Draw mesh function?
			if (actor->mesh->vertexBuffer != 0) {
				FixArray<RendererHandle, 1> buffers;
				buffers[0] = actor->mesh->vertexBuffer;
                
				FixArray<Size, 1> offsets;
				offsets[0] = 0;
                
				renderer->CmdBindVertexBuffers(0, &buffers, &offsets);
				renderer->CmdDrawIndexed(actor->mesh->vertices.size, 0, 0, 1, 0);
			}
		}
	}
}

void InitRenderSystem(RenderSystem* renderSystem, Renderer* renderer)
{
	renderSystem->renderer = renderer;

	// Cameras
	renderSystem->cameraConstantsBuffer = renderer->CreateBuffer(NULL, sizeof(CameraConstants), RENDERBUFFER_FLAGS_CONSTANT | RENDERBUFFER_FLAGS_USAGE_STREAM);
	Log("ccb = %d", renderSystem->cameraConstantsBuffer);

	// Swapchain
	SwapchainDescription swapchainDesc;
	swapchainDesc.format = FORMAT_B8G8R8A8_UNORM_SRGB;
	swapchainDesc.presentMode = PRESENTMODE_TRIPLE_BUFFER_MAILBOX;
	renderSystem->swapchain = renderer->CreateSwapchain(&swapchainDesc);
    
	Swapchain* swapchainPtr = renderer->LookupSwapchain(renderSystem->swapchain);
	if (swapchainPtr) {
		renderSystem->renderTargets = MakeArray<RendererHandle>(swapchainPtr->textures.size);
		ForIdx (renderSystem->renderTargets, idx) {
			RenderTargetDescription rtDesc { };
			rtDesc.width = 1920; // ~Hack super jank
			rtDesc.height = 1080; // ~Hack super jank
            
			// ~Refactor "Render Target Builder", also replace all the renderer create resource functions with either 1 lines or builders
			// eg: AttachToRenderTarget(&rtBuilder, swapchainPtr->textures[idx]);
            
			auto colorAttachments = MakeDynArray<RendererHandle>(0, Frame_Arena);
			ArrayAdd(&colorAttachments, swapchainPtr->textures[idx]);
			rtDesc.colorAttachments = &colorAttachments;
			renderSystem->renderTargets[idx] = renderer->CreateRenderTarget(&rtDesc);
		}
	}
    
    
    
    
    
	// framegraph
	{
		// buffers (~Temp gonna be automated later)
		renderSystem->gbufferColor   = renderer->CreateTexture(1920, 1080, FORMAT_R16G16B16A16_FLOAT, 1, TextureFlags_ColorAttachment | TextureFlags_Sampled);
		renderSystem->gbufferNormals = renderer->CreateTexture(1920, 1080, FORMAT_R16G16B16A16_FLOAT, 1, TextureFlags_ColorAttachment | TextureFlags_Sampled);
		renderSystem->compositeLightGizmos = renderer->CreateTexture(1920, 1080, FORMAT_R16G16B16A16_FLOAT, 1, TextureFlags_ColorAttachment | TextureFlags_Sampled);
		renderSystem->compositeGizmosGui = renderer->CreateTexture(1920, 1080, FORMAT_R16G16B16A16_FLOAT, 1, TextureFlags_ColorAttachment | TextureFlags_Sampled);
		renderSystem->depthBuffer    = renderer->CreateTexture(1920, 1080, FORMAT_D32_FLOAT, 1, TextureFlags_DepthStencilAttachment | TextureFlags_Sampled);
		renderSystem->gizmosDepth = renderer->CreateTexture(1920, 1080, FORMAT_D32_FLOAT, 1, TextureFlags_DepthStencilAttachment);
		renderSystem->guiDepth = renderer->CreateTexture(1920, 1080, FORMAT_D32_FLOAT, 1, TextureFlags_DepthStencilAttachment);
		renderSystem->compositeFinal = renderer->CreateTexture(1920, 1080, FORMAT_R16G16B16A16_FLOAT, 1, TextureFlags_ColorAttachment | TextureFlags_Sampled);
        
		auto gbufferPassFunc = [](Renderer* renderer, FgPass* pass)
		{
			// ~Refactor (...) automate the resolution values (eg, pass in the resolution value as an arg for the textures, but we still make the render area ourselves)
			Rect renderArea = {};
			renderArea.width = 1920;
			renderArea.height = 1080;
			
			// ~CleanUp make this nicer by moving clear colors onto the FgDependencyWrite
			FixArray<Vec4, 2> clearColors;
			clearColors[0] = v4(0.6,0.7,0.9,1);
			clearColors[1] = v4(1);
            
			auto* myPass = (GBufferPass*)pass;
			CameraConstants cameraConstants = myPass->cameraConstants;
			auto* renderSystem = myPass->renderSystem;
			auto* world = myPass->world;
            
			// ~Refactor automate this in the framegraph building
			FixArray<RendererHandle, 2> attachments;
			attachments[0] = myPass->outColor.texture;
			attachments[1] = myPass->outNormals.texture;
			
			RendererHandle target = ProcureSuitableRenderTarget(renderer, RenderTargetDescription {
                                                                    .colorAttachments = &attachments,
                                                                    .depthAttachment = myPass->outDepth.texture,
                                                                    .width = 1920,
                                                                    .height = 1080,
                                                                });
            
			renderer->CmdBeginPass(pass->builtPass, target, renderArea, clearColors, 1);
            
			RenderActorsInWorld(renderSystem, world, cameraConstants);
            
			renderer->CmdEndPass();
		};
		InitFgPass(GBufferPass, &renderSystem->pass_gbuffer, gbufferPassFunc);
		FgBufferWrite(&renderSystem->pass_gbuffer, outColor, renderSystem->gbufferColor);
		FgBufferWrite(&renderSystem->pass_gbuffer, outNormals, renderSystem->gbufferNormals);
		FgBufferWrite(&renderSystem->pass_gbuffer, outDepth, renderSystem->depthBuffer);
        
        
        
        
        
		auto deferredPassFunc = [](Renderer* renderer, FgPass* pass)
		{
			Rect renderArea = {};
			renderArea.width = 1920;
			renderArea.height = 1080;
            
			FixArray<Vec4, 1> clearColors;
			clearColors[0] = v4(0,0,0,1);
            
			auto* myPass = (DeferredPass*)pass;
			CameraConstants cameraConstants = myPass->cameraConstants;
			auto* renderSystem = myPass->renderSystem;
			auto* world = myPass->world;
            
			// ~Refactor automate this in the framegraph building
			FixArray<RendererHandle, 1> attachments;
			attachments[0] = myPass->outComposite.texture;
			RendererHandle target = ProcureSuitableRenderTarget(renderer, RenderTargetDescription {
                                                                    .colorAttachments = &attachments,
                                                                    .width = 1920,
                                                                    .height = 1080,
                                                                });
            
			renderer->CmdBeginPass(pass->builtPass, target, renderArea, clearColors, 1);
            
            
            
            
            
			renderer->CmdSetShader(myPass->deferredShader->handle);
			FixArray<TextureSampler, 3> samplers;
			samplers[0] = TextureSampler {
				.textureHandle = myPass->inColor.input->texture,
				.samplerHandle = ProcureSuitableSampler(renderer, Filter_Nearest, Filter_Nearest, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerMipmap_Linear)
			};
			samplers[1] = TextureSampler {
				.textureHandle = myPass->inNormals.input->texture,
				.samplerHandle = ProcureSuitableSampler(renderer, Filter_Nearest, Filter_Nearest, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerMipmap_Linear)
			};
			samplers[2] = TextureSampler {
				.textureHandle = myPass->inDepth.input->texture,
				.samplerHandle = ProcureSuitableSampler(renderer, Filter_Nearest, Filter_Nearest, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerMipmap_Linear)
			};
			renderer->CmdSetSamplers(myPass->deferredShader->handle, 15, &samplers); // ~Todo use the shader metadata table to get the sampler index
			
            
            
			int cameraConstantsSlot = ShaderConstantBufferSlot(myPass->deferredShader, Camera_Constants_Block_Name);
			if (cameraConstantsSlot != -1)
			{
				FixArray<RendererHandle, 1> a;
				a[0] = renderSystem->cameraConstantsBuffer;
				renderer->CmdSetConstantBuffers(myPass->deferredShader->handle, cameraConstantsSlot, &a);
			}
            
			FixArray<RendererHandle, 1> buffers;
			buffers[0] = quadVbo;
			renderer->CmdBindVertexBuffers(0, &buffers, NULL);
            
            
			// ~Refactor cache the camera and light pools
			EntityRegEntry lightReg;
			GetEntityEntry(Light, &lightReg);
            
			EntityPool lightPool;
			if (GetWorldEntityPool(world, lightReg.typeId, &lightPool))
			{
				for (auto it = EntityPoolBegin(lightPool); EntityPoolItValid(it); it = EntityPoolNext(it))
				{
					auto* light = (Light*)GetEntityFromPoolIt(it);
                    
                    
					// Vec3 lightPos = v3(-969.628235,307.012695,447.588318);
                    
					// set the light data	
					struct {
						Mat4 lightPose;
						Vec3 lightColor;
						float lightIntensity;
						int lightType;
					} lightConstants;
                    
					lightConstants.lightPose = EntityWorldXformMatrix(world, light);
					lightConstants.lightColor = light->color;
					lightConstants.lightIntensity = light->intensity;
					lightConstants.lightType = light->lightType;
					renderer->CmdPushConstants(myPass->deferredShader->handle, 0, sizeof(lightConstants), &lightConstants);
                    
                    
					renderer->CmdDrawIndexed(6, 0, 0, 1, 0);
				}
			}
            
			renderer->CmdEndPass();
		};
		InitFgPass(DeferredPass, &renderSystem->pass_deferred, deferredPassFunc);
		FgBufferRead(&renderSystem->pass_deferred, inColor, TextureLayout_ShaderReadOnlyOptimal);
		FgBufferRead(&renderSystem->pass_deferred, inNormals, TextureLayout_ShaderReadOnlyOptimal);
		FgBufferRead(&renderSystem->pass_deferred, inDepth, TextureLayout_ShaderReadOnlyOptimal);
		FgBufferWrite(&renderSystem->pass_deferred, outComposite, renderSystem->compositeLightGizmos);
		renderSystem->pass_deferred.deferredShader = LoadShader("shaders/deferred.shader");
        
        
        
        
		auto gizmosPassFunc = [](Renderer* renderer, FgPass* pass)
		{
			Rect renderArea = {};
			renderArea.width = 1920;
			renderArea.height = 1080;
            
			FixArray<Vec4, 1> clearColors;
			clearColors[0] = v4(0,0,0,1);
            
			auto* myPass = (GizmosPass*)pass;
            
			// ~Refactor automate this in the framegraph building
			FixArray<RendererHandle, 1> attachments;
			attachments[0] = myPass->outComposite.texture;
			
			RendererHandle target = ProcureSuitableRenderTarget(renderer, RenderTargetDescription {
                                                                    .colorAttachments = &attachments,
                                                                    .depthAttachment = myPass->outDepth.texture,
                                                                    .width = 1920,
                                                                    .height = 1080,
                                                                });
            
			renderer->CmdBeginPass(pass->builtPass, target, renderArea, clearColors, 1);
			
			{ // ~Compress @@CopyImg
				renderer->CmdSetShader(copyShader->handle);
				FixArray<TextureSampler, 1> samplers;
				samplers[0] = TextureSampler {
					.textureHandle = myPass->inComposite.input->texture,
					.samplerHandle = ProcureSuitableSampler(renderer, Filter_Nearest, Filter_Nearest, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerMipmap_Linear)
				};
				renderer->CmdSetSamplers(copyShader->handle, 15, &samplers); // ~Todo use the shader metadata table to get the sampler index
                
				
				FixArray<RendererHandle, 1> buffers;
				buffers[0] = quadVbo;
				renderer->CmdBindVertexBuffers(0, &buffers, NULL);
				renderer->CmdDrawIndexed(6, 0, 0, 1, 0);
			}
            
			DrawGizmos(myPass->cameraConstants.projectionMatrix, myPass->cameraConstants.viewMatrix);
			renderer->CmdEndPass();
		};
		InitFgPass(GizmosPass, &renderSystem->pass_gizmos, gizmosPassFunc);
		FgBufferRead(&renderSystem->pass_gizmos, inComposite, TextureLayout_ShaderReadOnlyOptimal);
		FgBufferWrite(&renderSystem->pass_gizmos, outComposite, renderSystem->compositeGizmosGui);
		FgBufferWrite(&renderSystem->pass_gizmos, outDepth, renderSystem->gizmosDepth);
        
        
        
        
		auto guiPassFunc = [](Renderer* renderer, FgPass* pass)
		{
			Rect renderArea = {};
			renderArea.width = 1920;
			renderArea.height = 1080;
            
			FixArray<Vec4, 1> clearColors;
			clearColors[0] = v4(0,0,0,1);
            
			auto* myPass = (GuiPass*)pass;
            
			// ~Refactor automate this in the framegraph building
			FixArray<RendererHandle, 1> attachments;
			attachments[0] = myPass->outComposite.texture;
			
			RendererHandle target = ProcureSuitableRenderTarget(renderer, RenderTargetDescription {
                                                                    .colorAttachments = &attachments,
                                                                    .depthAttachment = myPass->outDepth.texture,
                                                                    .width = 1920,
                                                                    .height = 1080,
                                                                });
            
			renderer->CmdBeginPass(pass->builtPass, target, renderArea, clearColors, 1);
            
            
			{ // ~Compress @@CopyImg
				renderer->CmdSetShader(copyShader->handle);
				FixArray<TextureSampler, 1> samplers;
				samplers[0] = TextureSampler {
					.textureHandle = myPass->inComposite.input->texture,
					.samplerHandle = ProcureSuitableSampler(renderer, Filter_Nearest, Filter_Nearest, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerMipmap_Linear)
				};
				renderer->CmdSetSamplers(copyShader->handle, 15, &samplers); // ~Todo use the shader metadata table to get the sampler index
                
				
				FixArray<RendererHandle, 1> buffers;
				buffers[0] = quadVbo;
				renderer->CmdBindVertexBuffers(0, &buffers, NULL);
				renderer->CmdDrawIndexed(6, 0, 0, 1, 0);
			}
            
			RenderGui(&game->gui);
            
			renderer->CmdEndPass();
		};
		InitFgPass(GuiPass, &renderSystem->pass_gui, guiPassFunc);
		FgBufferRead(&renderSystem->pass_gui, inComposite, TextureLayout_ShaderReadOnlyOptimal);
		FgBufferWrite(&renderSystem->pass_gui, outComposite, renderSystem->compositeFinal);
		FgBufferWrite(&renderSystem->pass_gui, outDepth, renderSystem->guiDepth);
        
		//
		FgAttach(&renderSystem->pass_gbuffer, outColor, &renderSystem->pass_deferred, inColor);
		FgAttach(&renderSystem->pass_gbuffer, outNormals, &renderSystem->pass_deferred, inNormals);
		FgAttach(&renderSystem->pass_gbuffer, outDepth, &renderSystem->pass_deferred, inDepth);
        
		//
		FgAttach(&renderSystem->pass_deferred, outComposite, &renderSystem->pass_gizmos, inComposite);
        
		//
		FgAttach(&renderSystem->pass_gizmos, outComposite, &renderSystem->pass_gui, inComposite);	
        
		// insert a hook on the last pass to confirm the layout
		renderSystem->pass_gui.outComposite.builtLayout = TextureLayout_ShaderReadOnlyOptimal; // use this to copy the image into the swapchain
        
		auto passes = MakeDynArray<FgPass*>(0, Frame_Arena);
		ArrayAdd(&passes, (FgPass*)&renderSystem->pass_gbuffer);
		ArrayAdd(&passes, (FgPass*)&renderSystem->pass_deferred);
		ArrayAdd(&passes, (FgPass*)&renderSystem->pass_gui);
		ArrayAdd(&passes, (FgPass*)&renderSystem->pass_gizmos);
		renderSystem->graph = FgBuild(renderSystem->renderer, passes);
	}
    
	// utilities
	Vec4 quadVerts[6] {
		v4(-1, -1, 0, 1),
		v4(-1,  1, 0, 0),
		v4( 1,  1, 1, 0),
        
		v4(-1, -1, 0, 1),
		v4( 1,  1, 1, 0),
		v4( 1, -1, 1, 1),
	};
	quadVbo = renderer->CreateBuffer(quadVerts, sizeof(quadVerts), RENDERBUFFER_FLAGS_VERTEX | RENDERBUFFER_FLAGS_USAGE_STATIC);
	copyShader = LoadShader("shaders/copy.shader");
    
	{
		FixArray<RenderPassColorAttachmentDescription, 1> colorAttachments;
		colorAttachments[0] = RenderPassColorAttachmentDescription {
			.format = FORMAT_B8G8R8A8_UNORM_SRGB,
            
			.sampleCount = 1,
            
			.loadOp = RENDERATTACHOP_DONT_CARE,
			.storeOp = RENDERATTACHOP_STORE,
            
			.inLayout = TextureLayout_Undefined,
			.outLayout = TextureLayout_PresentSrc,
		};
		RenderPassDescription desc { colorAttachments };
		renderSystem->presentPass = renderer->CreatePass(&desc);
	}
    
	// materials
	renderSystem->nextMaterialId = 1;
	renderSystem->materialKeys = MakeDynArray<MaterialId>();
	renderSystem->materials = MakeDynArray<Material>();
}

void DeleteRenderSystem(RenderSystem* renderSystem) {
	auto renderer = renderSystem->renderer;
    
	renderer->FreeTexture(renderSystem->depthBuffer);
    
	renderer->FreeBuffer(renderSystem->cameraConstantsBuffer);
    
	renderer->FreeSwapchain(renderSystem->swapchain);
    
	For(renderSystem->renderTargets) {
		renderer->FreeRenderTarget(*it);
	}
    
	// delete materials
	FreeDynArray(&renderSystem->materials);
	FreeDynArray(&renderSystem->materialKeys);
}


void RenderView(RenderSystem* renderSystem, RendererHandle renderTarget, CameraConstants cameraConstants, World* world)
{
	auto renderer = renderSystem->renderer;
    
	renderSystem->pass_gbuffer.renderSystem = renderSystem;
	renderSystem->pass_gbuffer.world = world;
	renderSystem->pass_gbuffer.cameraConstants = cameraConstants;
    
	renderSystem->pass_deferred.renderSystem = renderSystem;
	renderSystem->pass_deferred.world = world;
	renderSystem->pass_deferred.cameraConstants = cameraConstants;
    
	renderSystem->pass_gizmos.cameraConstants = cameraConstants;
    
	FgDo(&renderSystem->graph, renderSystem->renderer);
    
    
	// ~Optimize look into using a vulkan Copy operation to copy the images
	// copy the final rendereed image into the swapchain
    
	// ~CleanUp make the arguments to the process into vars 
	// renderSystem->pass_gui.outComposite.texture,
	// maybe make this a function (not the barrier part)
	auto texBarriers = MakeDynArray<PipelineTextureBarrier>(0, Frame_Arena);
	ArrayAdd(&texBarriers, {
                 .texture = renderSystem->pass_gui.outComposite.texture,
                 .inLayout = TextureLayout_ShaderReadOnlyOptimal,
                 .outLayout = TextureLayout_ShaderReadOnlyOptimal
             });
	PipelineBarrier barrier { texBarriers };
	renderSystem->renderer->CmdPipelineBarrier(barrier);
    
	Rect renderArea = {};
	renderArea.width = 1920;
	renderArea.height = 1080;
	FixArray<Vec4, 1> clearValues;
	renderer->CmdBeginPass(renderSystem->presentPass, renderTarget, renderArea, clearValues, 1);
    
    
	renderer->CmdSetShader(copyShader->handle);
	FixArray<TextureSampler, 1> samplers;
	samplers[0] = TextureSampler {
		.textureHandle = renderSystem->pass_gui.outComposite.texture,
		.samplerHandle = ProcureSuitableSampler(renderer, Filter_Nearest, Filter_Nearest, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerAddress_ClampToEdge, SamplerMipmap_Linear)
	};
	renderer->CmdSetSamplers(copyShader->handle, 15, &samplers); // ~Todo use the shader metadata table to get the sampler index
    
	
	FixArray<RendererHandle, 1> buffers;
	buffers[0] = quadVbo;
	renderer->CmdBindVertexBuffers(0, &buffers, NULL);
	renderer->CmdDrawIndexed(6, 0, 0, 1, 0);
    
    
	renderer->CmdEndPass();
}


// ~Todo remove the RenderWorld and RenderView function and just make a Render(RenderSystem* renderSystem, World* world, CameraConstants cameraConstants)
// 		 also only have 1 active camera at a time
void RenderWorld(RenderSystem* renderSystem, RenderInfo renderInfo, World* world)
{
    RendererHandle swapchainFbo = renderSystem->renderTargets[renderInfo.swapchainImageIndex];
    
	EntityRegEntry cameraReg;
	GetEntityEntry(Camera, &cameraReg);
    
	EntityPool cameraPool;
	if (GetWorldEntityPool(world, cameraReg.typeId, &cameraPool))
	{
		for (auto it = EntityPoolBegin(cameraPool); EntityPoolItValid(it); it = EntityPoolNext(it))
		{
			auto* camera = (Camera*)GetEntityFromPoolIt(it);
			Assert(camera);
            
			CameraConstants cameraConstants;
			cameraConstants.projectionMatrix = camera->projectionMatrix;
			cameraConstants.viewMatrix = Inverse(EntityWorldXformMatrix(world, camera));
            
			RenderView(renderSystem, swapchainFbo, cameraConstants, world);
		}
	}
	else
	{
		LogWarn("[rendersys] Failed to get actor pool for '%s'", cameraReg.name);
	}
}


////// material

Material* LookupMaterial(RenderSystem* renderSystem, MaterialId id)
{
	u32 materialIndex = ArrayFind(&renderSystem->materialKeys, id);
	if (materialIndex == -1)
	{
		LogWarn("[rendersys] Attempted to index material that doesn't exist (id=%u)", id);
		return 0;
	}
	return &renderSystem->materials[materialIndex];
}

//////




////// Cameras

void Camera::Init(World* world) {
	this->projectionMatrix = PerspectiveMatrix(60.0, 16.0 / 9.0, 0.1, 1000.0);
}

void Camera::Destroy(World* world) {
}

void Camera::Start(World* world) {

}

void Camera::Update(World* world) {

}

RegisterEntity(Camera);




/////// Lights ///////

void Light::Init(World* world) {
	this->flags |= Entity_Global; // @Temp
}

void Light::Destroy(World* world) {
}

void Light::Start(World* world) {
}

void Light::Update(World* world) {
}

RegisterEntity(Light);