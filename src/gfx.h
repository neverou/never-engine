#pragma once

#include "renderer.h"
#include "world.h"
#include "material.h"

struct DrawConstants
{
	Mat4 transformMatrix;
};

struct CameraConstants
{
	Mat4 projectionMatrix;
	Mat4 viewMatrix;
};




// passes
struct GBufferPass : public FgPass
{
	FgDependencyWrite outColor;
	FgDependencyWrite outNormals;
	FgDependencyWrite outDepth;

	// args
	CameraConstants cameraConstants;
	World* world;
	RenderSystem* renderSystem;
};

struct DeferredPass : public FgPass
{
	FgDependencyRead inColor;
	FgDependencyRead inNormals;
	FgDependencyRead inDepth;

	FgDependencyWrite outComposite;
	
	// args
	CameraConstants cameraConstants;
	World* world;
	RenderSystem* renderSystem;

	ShaderResource* deferredShader;
};

struct GizmosPass : public FgPass
{
	FgDependencyRead inComposite;
	FgDependencyWrite outComposite;

	FgDependencyWrite outDepth;

	//
	CameraConstants cameraConstants;
};

struct GuiPass : public FgPass
{
	FgDependencyRead inComposite;

	FgDependencyWrite outComposite;
	FgDependencyWrite outDepth;
};

struct RenderSystem
{
	Renderer* renderer;

	RendererHandle swapchain;
	SArray<RendererHandle> renderTargets;

	// framegraph
	RendererHandle gbufferColor;
	RendererHandle gbufferNormals;
	RendererHandle compositeLightGizmos;
	RendererHandle compositeGizmosGui;
	RendererHandle compositeFinal;
	RendererHandle depthBuffer;
	RendererHandle gizmosDepth;
	RendererHandle guiDepth;

	GBufferPass pass_gbuffer;
	DeferredPass pass_deferred;
	GizmosPass pass_gizmos;
	GuiPass pass_gui;
	FgBuiltGraph graph;

	RendererHandle presentPass;
	//

	RendererHandle cameraConstantsBuffer;

	// materials
	MaterialId nextMaterialId;
	DynArray<MaterialId> materialKeys;
	DynArray<Material> materials;

};

void InitRenderSystem(RenderSystem* renderSystem, Renderer* renderer);
void DeleteRenderSystem(RenderSystem* renderSystem);

void RenderView(RenderSystem* renderSystem, RendererHandle framebuffer, CameraConstants cameraConstants, World* world);
void RenderWorld(RenderSystem* renderSystem, RenderInfo renderInfo, World* world);

void RenderGuiPass(RenderSystem* renderSystem, RenderInfo renderInfo);


// material
Material* LookupMaterial(RenderSystem* renderSystem, MaterialId id);


//


struct Camera : public Entity
{
	Mat4 projectionMatrix;

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};

enum LightType
{
	LightType_Point = 0,
	LightType_Directional = 1,
	LightType_Spot = 2,
};

struct Light : public Entity
{
	BEGIN_DATADESC(Light)
	DEFINE_FIELD(FIELD_INT, lightType)
	DEFINE_FIELD(FIELD_FLOAT, intensity)
	DEFINE_FIELD(FIELD_FLOAT, color.x)
	DEFINE_FIELD(FIELD_FLOAT, color.y)
	DEFINE_FIELD(FIELD_FLOAT, color.z)
	END_DATADESC()

	LightType lightType;
	float intensity;
	Vec3 color;

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};
