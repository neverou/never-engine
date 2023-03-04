#pragma once


// ~Todo: add back renderer implementations when they fully implement Renderer,
// because everything other than Vulkan doesnt work right now, they dont have buffers

#if defined(PLATFORM_WINDOWS)

// #define RENDERER_IMPL_D3D11
// #define RENDERER_IMPL_OPENGL
#define RENDERER_IMPL_VULKAN

#endif

#if defined(PLATFORM_LINUX)

// #define RENDERER_IMPL_OPENGL
#define RENDERER_IMPL_VULKAN

#endif

#include "std.h"
#include "maths.h"

struct GraphicsProcessor {
    char deviceName[256]; // maybe come up with a better solution??
    u32 vendorID;
    u32 deviceID;
    void* platformHandle;
};



enum RendererType {
    RENDERER_VULKAN,
    RENDERER_D3D11,
    RENDERER_OPENGL,
};


enum RendererFlags {
    RENDERER_FLAG_NONE = 0x0,
    RENDERER_FLAG_SUPPORTS_GRAPHICS_PROCESSOR_AFFINITY = 0x1,
};


enum RenderFormat {
    FORMAT_NONE = 0,
	// R
    FORMAT_R8_SNORM,
    FORMAT_R8_UNORM,
    FORMAT_R8_SINT,
    FORMAT_R8_UINT,
    
    FORMAT_R16_SNORM,
    FORMAT_R16_UNORM,
    FORMAT_R16_SINT,
    FORMAT_R16_UINT,
    FORMAT_R16_FLOAT,
    
    FORMAT_R32_SINT,
    FORMAT_R32_UINT,
    FORMAT_R32_FLOAT,
    
    
    // RG
    FORMAT_R8G8_SNORM,
    FORMAT_R8G8_UNORM,
    FORMAT_R8G8_SINT,
    FORMAT_R8G8_UINT,
    
    FORMAT_R16G16_SNORM,
    FORMAT_R16G16_UNORM,
    FORMAT_R16G16_SINT,
    FORMAT_R16G16_UINT,
    FORMAT_R16G16_FLOAT,
    
    FORMAT_R32G32_SINT,
    FORMAT_R32G32_UINT,
    FORMAT_R32G32_FLOAT,
    
    
    // RGB
    FORMAT_R32G32B32_SINT,
    FORMAT_R32G32B32_UINT,
    FORMAT_R32G32B32_FLOAT,
    
    
    // RGBA
    FORMAT_R8G8B8A8_SNORM,
    FORMAT_R8G8B8A8_UNORM,
    FORMAT_R8G8B8A8_SINT,
    FORMAT_R8G8B8A8_UINT,
    FORMAT_R8G8B8A8_UNORM_SRGB,
    
    FORMAT_R16G16B16A16_SNORM,
    FORMAT_R16G16B16A16_UNORM,
    FORMAT_R16G16B16A16_SINT,
    FORMAT_R16G16B16A16_UINT,
    FORMAT_R16G16B16A16_FLOAT,
    
    FORMAT_R32G32B32A32_SINT,
    FORMAT_R32G32B32A32_UINT,
    FORMAT_R32G32B32A32_FLOAT,
    
    
    // BGRA
    FORMAT_B8G8R8A8_SNORM,
    FORMAT_B8G8R8A8_UNORM,
    FORMAT_B8G8R8A8_SINT,
    FORMAT_B8G8R8A8_UINT,
    FORMAT_B8G8R8A8_UNORM_SRGB,

    // Depth
    FORMAT_D32_FLOAT,
    FORMAT_D32_FLOAT_S8_UINT,
    FORMAT_D24_UNORM_S8_UINT,
};


size_t RenderFormatBytesPerPixel(RenderFormat format);


bool FormatHasDepth(RenderFormat format);

enum PresentMode {
	PRESENTMODE_DOUBLE_BUFFER_IMMEDIATE = 0x00,
	PRESENTMODE_DOUBLE_BUFFER_FIFO 		= 0x01,
	PRESENTMODE_TRIPLE_BUFFER_FIFO 		= 0x02,
	PRESENTMODE_TRIPLE_BUFFER_MAILBOX	= 0x03,
};

//

constexpr u32 RendererHandle_None = 0;
typedef u32 RendererHandle;

enum RenderBufferFlags {
    RENDERBUFFER_FLAGS_NONE 		 = 0x00,
    RENDERBUFFER_FLAGS_VERTEX 		 = 0x01,
    RENDERBUFFER_FLAGS_INDEX 		 = 0x02,
    RENDERBUFFER_FLAGS_CONSTANT 	 = 0x04,

	RENDERBUFFER_FLAGS_USAGE_STATIC  = 0x08,
	RENDERBUFFER_FLAGS_USAGE_DYNAMIC = 0x10,
	RENDERBUFFER_FLAGS_USAGE_STREAM  = 0x20,
};


// ~Refactor ~CleanUp:
// put all the resources together in the file
// possibly make a base resource struct and stick resource flags in there?

// ~Note: 
// decide if we want to do structs for resource creation

struct Buffer {
    u32 flags;
    u64 size;
};

enum TextureLayout
{
	TextureLayout_Undefined = 0,
    TextureLayout_General,
    TextureLayout_ColorAttachmentOptimal,
    TextureLayout_DepthStencilAttachmentOptimal,
    TextureLayout_DepthStencilReadOnlyOptimal,
    TextureLayout_ShaderReadOnlyOptimal,
    TextureLayout_TransferSrcOptimal,
    TextureLayout_TransferDstOptimal,
    TextureLayout_Preinitialised,
	TextureLayout_PresentSrc,
};


enum RenderAttachOp {
    RENDERATTACHOP_DONT_CARE = 0,
    RENDERATTACHOP_LOAD,
    RENDERATTACHOP_STORE,
    RENDERATTACHOP_CLEAR,
};

struct RenderPassColorAttachmentDescription {
	RenderFormat format;
	u8 sampleCount;
    RenderAttachOp loadOp;
    RenderAttachOp storeOp;

	TextureLayout inLayout;
	TextureLayout outLayout;
};

struct RenderPassDepthAttachmentDescription {
	RenderFormat format;
	u8 sampleCount; // ~Todo does this need to exist?
	RenderAttachOp loadOp;
	RenderAttachOp storeOp;

	TextureLayout inLayout;
	TextureLayout outLayout;
};

struct RenderPassDescription {
	Array<RenderPassColorAttachmentDescription> colorAttachments;

	bool hasDepthAttachment;
	RenderPassDepthAttachmentDescription depthAttachment;
};

struct RenderPass {
	u32 colorAttachmentCount;
	bool hasDepthAttachment;
};



struct RenderInfo {
    u32 swapchainImageIndex;
    bool swapchainValid;
};


enum PrimitiveType {
    Primative_Triangle_List,
	Primative_Line_List,
};

enum PolygonFillMode {
    PolygonMode_Fill,
    PolygonMode_Line,
    PolygonMode_Point,
};

enum CullMode {
    Cull_None = 0,
    Cull_Front = 1,
    Cull_Back = 2,
    Cull_FrontAndBack = Cull_Front | Cull_Back
};

enum FrontFace {
    FrontFace_CW,
    FrontFace_CCW,
};

enum BlendFactor {
    BlendFactor_Zero,
    BlendFactor_One,
    BlendFactor_Src_Color,
    BlendFactor_One_Minus_Src_Color,
    BlendFactor_Dst_Color,
    BlendFactor_One_Minus_Dst_Color,
    BlendFactor_Src_Alpha,
    BlendFactor_One_Minus_Src_Alpha,
    BlendFactor_Dst_Alpha,
    BlendFactor_One_Minus_Dst_Alpha,
    // BlendFactor_CONSTANT_COLOR = 10,
    // BlendFactor_One_MINUS_CONSTANT_COLOR = 11,
    // BlendFactor_CONSTANT_ALPHA = 12,
    // BlendFactor_One_MINUS_CONSTANT_ALPHA = 13,
    BlendFactor_Src_Alpha_Saturate,
    // BlendFactor_SRC1_COLOR = 15,
    // BlendFactor_One_MINUS_SRC1_COLOR = 16,
    // BlendFactor_SRC1_ALPHA = 17,
    // BlendFactor_One_MINUS_SRC1_ALPHA = 18,
};

enum BlendOp {
    BlendOp_Add = 0,
    BlendOp_Subtract = 1,
    BlendOp_Reverse_Subtract = 2,
    BlendOp_Min = 3,
    BlendOp_Max = 4,
};

enum CompareOp
{
    CompareOp_Never = 0,
    CompareOp_Less = 1,
    CompareOp_Equal = 2,
    CompareOp_Less_Or_Equal = 3,
    CompareOp_Greater = 4,
    CompareOp_Not_Equal = 5,
    CompareOp_Greater_Or_Equal = 6,
    CompareOp_Always = 7,
};


// ~Todo change this to a BufferElements and BufferLayout thing instead of the hack

enum VertexInputRate {
    VERTEX_INPUT_RATE_PER_VERTEX = 0x0,
    VERTEX_INPUT_RATE_PER_INSTANCE = 0x1,
};

struct VertexInputElement {
    u32 location;
    u32 offset;
    RenderFormat format;
};

struct VertexInputLayout {
    u32 binding;
    u32 stride;
    VertexInputRate inputRate;
    
    // ~Hack HANK! 
    VertexInputElement elements[8];
    u32 elementCount;
};



enum ShaderType
{
    SHADER_VERTEX,
    SHADER_PIXEL,
};


enum ShaderFieldType
{
	ShaderField_None,
	ShaderField_Float,
	ShaderField_Float2,
	ShaderField_Float3,
	ShaderField_Float4,
	ShaderField_Mat4,
	ShaderField_Int,
};

struct ShaderStageDescription {
    ShaderType type;
    Size size;
    void* data;
};

struct ShaderDescription {
    // ~Todo move these into vertex buffers
    Array<VertexInputLayout>* vertexInputLayouts;
    PrimitiveType primitiveType;
    //
    

    Array<ShaderStageDescription>* stages;


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


struct Shader
{
    // ~Todo move these into vertex buffers
    SArray<VertexInputLayout> vertexInputLayouts;
    PrimitiveType primitiveType;
    //

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

	// ~Todo: Shader metadata?
};



struct RenderTargetDescription {
    Array<RendererHandle>* colorAttachments;
    RendererHandle depthAttachment;
    u32 width;
    u32 height;
};

struct RenderTarget {
    u32 width;
    u32 height;
};


enum FilterMode
{
	Filter_Nearest	= 0,
	Filter_Linear,
};

enum SamplerAddressMode
{
	SamplerAddress_Repeat 			= 0,
    SamplerAddress_MirroredRepeat,
    SamplerAddress_ClampToEdge,
    SamplerAddress_ClampToBorder,
};

enum SamplerMipmapMode
{
    SamplerMipmap_Nearest = 0,
    SamplerMipmap_Linear,
};



struct Sampler
{
	FilterMode magFilter;
	FilterMode minFilter;
	SamplerAddressMode addressModeU;
	SamplerAddressMode addressModeV;
	SamplerAddressMode addressModeW;
	SamplerMipmapMode mipmapMode;
};


enum TextureFlags
{
	TextureFlags_None			= 0x0,

	TextureFlags_Sampled					= 0x1,
	TextureFlags_DepthStencilAttachment 	= 0x2,
	TextureFlags_ColorAttachment			= 0x4,
};


struct Texture
{
	u32 flags;
	u32 width;
	u32 height;
	RenderFormat format;
	u32 mipmaps;
};

//


struct SwapchainDescription {
    RenderFormat format;
    PresentMode presentMode;    
};

struct Swapchain {
    SArray<RendererHandle> textures;
};


struct TextureSampler
{
	RendererHandle textureHandle;
	RendererHandle samplerHandle;
};



struct PipelineTextureBarrier
{
	RendererHandle texture;
	TextureLayout inLayout;
	TextureLayout outLayout;
};

struct PipelineBarrier
{
	Array<PipelineTextureBarrier> textureBarriers;
};


struct RenderTargetCacheKey
{
	SArray<RendererHandle> colorAttachments;
    RendererHandle depthAttachment;
    u32 width;
    u32 height;
};

struct Renderer {
    Allocator* allocator;
	RendererType type;

	DynArray<RendererHandle> samplerCache;
	
	DynArray<RenderTargetCacheKey> renderTargetCacheKeys;
	DynArray<RendererHandle> renderTargetCacheValues;

    // Resource Management //
    virtual RendererHandle CreateBuffer(void* data, Size size, u32 flags) = 0;
    virtual void FreeBuffer(RendererHandle handle) = 0;
    virtual Buffer* LookupBuffer(RendererHandle handle) = 0;

    virtual RendererHandle CreatePass(const RenderPassDescription* desc) = 0;
    virtual void FreePass(RendererHandle handle) = 0;
    virtual RenderPass* LookupPass(RendererHandle handle) = 0;
        
    virtual RendererHandle CreateRenderTarget(const RenderTargetDescription* desc) = 0;
    virtual void FreeRenderTarget(RendererHandle handle) = 0;
    virtual RenderTarget* LookupRenderTarget(RendererHandle handle) = 0;
    
    virtual RendererHandle CreateTexture(u32 width, u32 height, RenderFormat format, u32 mipmaps, u64 flags) = 0;
    virtual void FreeTexture(RendererHandle handle) = 0;
    virtual Texture* LookupTexture(RendererHandle handle) = 0;

    virtual RendererHandle CreateSwapchain(const SwapchainDescription* desc) = 0;
    virtual void FreeSwapchain(RendererHandle handle) = 0;
    virtual Swapchain* LookupSwapchain(RendererHandle handle) = 0;
    
    virtual RendererHandle CreateShader(const ShaderDescription* desc) = 0;
    virtual void FreeShader(RendererHandle handle) = 0;
    virtual Shader* LookupShader(RendererHandle handle) = 0;
    
	virtual RendererHandle CreateSampler(FilterMode magFilter, FilterMode minFilter, SamplerAddressMode addressModeU, SamplerAddressMode addressModeV, SamplerAddressMode addressModeW, SamplerMipmapMode mipmapMode) = 0;
    virtual void FreeSampler(RendererHandle handle) = 0;
    virtual Sampler* LookupSampler(RendererHandle handle) = 0;
    
    virtual void* MapBuffer(RendererHandle handle) = 0;
    virtual void UnmapBuffer(RendererHandle handle) = 0; 

	// ~Todo make UploadBufferData() and make MapBuffer() persistent (only for STREAMING buffers)

	virtual void UploadTextureData(RendererHandle handle, Size size, void* data) = 0;
    //
    
    // Commands //
    virtual RenderInfo BeginRender(RendererHandle swapchain) = 0;
    virtual void EndRender(RendererHandle swapchain) = 0;
    
    virtual void CmdBeginPass(RendererHandle pass, RendererHandle renderTarget, Rect renderArea, Array<Vec4> clearColors, float clearDepth) = 0;
    virtual void CmdEndPass() = 0;
    
    virtual void CmdSetShader(RendererHandle handle) = 0;
	// ~Refactor make the array not a pointer
    virtual void CmdBindVertexBuffers(u32 firstBinding, Array<RendererHandle>* buffers, Array<Size>* offsets) = 0;
    virtual void CmdDrawIndexed(u32 indexCount, u32 firstIndex, u32 vertexOffset, u32 instanceCount, u32 firstInstance) = 0;
    
    virtual void CmdSetConstantBuffers(RendererHandle shaderHandle, u32 startSlot, Array<RendererHandle>* constantBuffers) = 0;
	virtual void CmdSetSamplers(RendererHandle shaderHandle, u32 startSlot, Array<TextureSampler>* textureSamplers) = 0;

	virtual void CmdUpdateBuffer(RendererHandle buffer, Size start, Size size, void* data) = 0;
	virtual void CmdPushConstants(RendererHandle shader, Size start, Size size, void* data) = 0;

	// ~Todo @@RendererUpgrade come up with a better way to handle placing barriers (provide information to the renderer backend through higher level data structures)
	virtual void CmdPipelineBarrier(PipelineBarrier barrier) = 0; 
    //
};



struct Surface;

enum RendererSpawnerFlags {
	RENDERER_SPAWNER_FLAG_NONE = 0x0,
	RENDERER_SPAWNER_FLAG_SUPPORTS_GRAPHICS_PROCESSOR_AFFINITY = 0x1,
};

struct RendererSpawner {
    DynArray<GraphicsProcessor> graphicsProcessors;
    
    Surface* surface;
    void* platformApiInstance;
    u64 flags;
};

struct RendererSpawnInfo {
	GraphicsProcessor chosenGraphicsProcessor;
	RendererSpawner spawner;
};


RendererSpawner MakeRendererSpawner(Surface* surface);
void DestroyRendererSpawner(RendererSpawner* spawner);

Renderer* SpawnRenderer(RendererSpawnInfo* spawnInfo, Allocator* allocator);
void DeleteRenderer(Renderer* renderer);


// struct RendererSystem {
//     Renderer* renderer;
//     Surface* surface;
// };

// RendererSystem MakeRendererSystem(Surface* surface, Allocator* allocator);
// void DestroyRendererSystem(RendererSystem* rendererSystem);




// Frame Graph

struct FgDependencyWrite
{
	RendererHandle texture; // ~Todo remove this when we do transient texture allocation

	// written into after framegraph is built
	TextureLayout builtLayout = TextureLayout_General;
};

struct FgDependencyRead
{
	FgDependencyWrite* input;
	TextureLayout layout;
};

struct FgPass;
typedef void (*PassRenderFunc)(Renderer* renderer, FgPass* pass);
struct FgPass
{
	String _name;
	PassRenderFunc _renderFunc;

	DynArray<FgDependencyRead*> reads;
	DynArray<FgDependencyWrite*> writes;

	DynArray<FgPass*> dependencies;

	// written into after framegraph is built
	RendererHandle builtPass;
};


enum FgGraphCommandType
{
	FgGraphCommand_ExecutePass,
	FgGraphCommand_PipelineBarrier,
};

struct FgGraphCommand
{
	FgGraphCommandType type;
	union
	{
		PipelineBarrier barrier;
		FgPass* pass;
	};
};

struct FgBuiltGraph
{
	SArray<FgGraphCommand> commands;
};

#define InitFgPass(X, pass, renderFunc) _InitFgPass((X*)pass, #X, renderFunc)
void _InitFgPass(FgPass* pass, StringView name, PassRenderFunc renderFunc);
void DestroyFgPass(FgPass* pass);

#define FgBufferRead(pass, read, layout) _FgBufferRead(pass, &(pass)->read, layout)
#define FgBufferWrite(pass, write, texture) _FgBufferWrite(pass, &(pass)->write, texture)
#define FgAttach(from, fromWrite, to, toRead) _FgAttach(from, &(from)->fromWrite, to, &(to)->toRead)

void _FgBufferRead(FgPass* pass, FgDependencyRead* read, TextureLayout layout);
void _FgBufferWrite(FgPass* pass, FgDependencyWrite* write, RendererHandle texture);
void _FgAttach(FgPass* from, FgDependencyWrite* fromWrite, FgPass* to, FgDependencyRead* toRead);

FgBuiltGraph FgBuild(Renderer* renderer, Array<FgPass*> passes);
void FgFree(FgBuiltGraph* graph);

void FgDo(FgBuiltGraph* graph, Renderer* renderer);

// sampler cache
RendererHandle ProcureSuitableSampler(
	Renderer* renderer, 
	FilterMode magFilter, 
	FilterMode minFilter, 
	SamplerAddressMode addressModeU, 
	SamplerAddressMode addressModeV, 
	SamplerAddressMode addressModeW, 
	SamplerMipmapMode mipmapMode);

// render target cache
RendererHandle ProcureSuitableRenderTarget(Renderer* renderer, RenderTargetDescription desc);