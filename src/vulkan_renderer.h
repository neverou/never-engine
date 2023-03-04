#pragma once

#include "renderer.h"

#if defined(RENDERER_IMPL_VULKAN)
#include "core.h"

#include <vulkan/vulkan.h>


struct VulkanBuffer : public Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

	VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
};

struct VulkanPass : public RenderPass
{
    VkRenderPass renderPass;
};

struct VulkanRenderTarget : public RenderTarget
{
    SArray<RendererHandle> attachments;
};

struct VulkanSampler : public Sampler
{
	VkSampler sampler;
};

enum VulkanTextureFlags
{
	VulkanTextureFlag_None 					= 0x0,
	VulkanTextureFlag_OnlyOwnTextureView 	= 0x1, // Meaning we don't own the texture, just the view, but there might be one (so don't deallocate it)
};

struct VulkanTexture : public Texture {
	u64 vkFlags;

	VkImage texture;
	VkDeviceMemory memory;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	VkImageView textureView;
};

struct VulkanSwapchain : public Swapchain {
    VkSwapchainKHR swapchain;
    u32 activeImageIndex;
};


struct VulkanShaderStage {
    ShaderType type;
    VkShaderModule shader;
};

struct VulkanShader : public Shader {
    SArray<VulkanShaderStage> stages;
};

enum VulkanMemoryType {
	VulkanMemory_DeviceLocal = 0,
	VulkanMemory_HostMappable,
	VulkanMemory_DeviceLocalHostMappable,
	// VulkanMemory_HostMappableCached,

	VulkanMemory_Count
};


struct VulkanCachePipeline {
    RendererHandle linkedPass;
    RendererHandle linkedShader;

    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct VulkanCacheFramebuffer {
 	RendererHandle linkedPass;
    RendererHandle linkedRenderTarget;

	VkFramebuffer framebuffer;	
};



// ~Todo: query physical device limits for these
constexpr u32 Vulkan_UniformDescriptorCount = 15; 
constexpr u32 Vulkan_SamplerDescriptorCount = 15;
constexpr u32 Vulkan_DescriptorSetCount 	= 1024;


enum VulkanDescriptorSlotType 
{
	VulkanDescriptorSlot_None = 0,
	VulkanDescriptorSlot_Uniform,
	VulkanDescriptorSlot_Sampler,
};

struct VulkanDescriptorSlot
{
	VulkanDescriptorSlotType type;

	union 
	{
		struct
		{
			RendererHandle bufferHandle;	
		} uniform;
		TextureSampler sampler;
	};
};

struct VulkanRenderer : public Renderer
{
    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    SArray<VkDescriptorSet> descriptorSets;
    u32 allocatedDescriptorSets;


    VkCommandPool commandPool;
    VkCommandBuffer mainCommandList;
    VkSemaphore presentSemaphore;
    VkSemaphore renderSemaphore;
    VkFence renderFence;

    u32 graphicsQueueFamily;
    u32 presentQueueFamily;
    u32 transferQueueFamily;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;

    VkDebugUtilsMessengerEXT debugUtilsMessenger;

    // Resources //
    RendererHandle nextHandle;

    DynArray<RendererHandle> 	bufferHandleKeys;
    DynArray<VulkanBuffer> 		buffers;

    DynArray<RendererHandle> 	passHandleKeys;
    DynArray<VulkanPass> 		passes;

    DynArray<RendererHandle> 	textureHandleKeys;
    DynArray<VulkanTexture> 	textures;

    DynArray<RendererHandle> 	swapchainHandleKeys;
    DynArray<VulkanSwapchain> 	swapchains;

    DynArray<RendererHandle> 	shaderHandleKeys;
    DynArray<VulkanShader> 		shaders;

    DynArray<RendererHandle> 	renderTargetHandleKeys;
    DynArray<VulkanRenderTarget> renderTargets;

	DynArray<RendererHandle> 	samplerHandleKeys;
	DynArray<VulkanSampler> 	samplers;
    //

    // Resource management //
    RendererHandle CreateBuffer(void* data, Size size, u32 flags) override;
    void FreeBuffer(RendererHandle handle) override;
    Buffer* LookupBuffer(RendererHandle handle) override;

    RendererHandle CreatePass(const RenderPassDescription* desc) override;
    void FreePass(RendererHandle handle) override;
    RenderPass* LookupPass(RendererHandle handle) override;

    RendererHandle CreateRenderTarget(const RenderTargetDescription* desc) override;
    void FreeRenderTarget(RendererHandle handle) override;
    RenderTarget* LookupRenderTarget(RendererHandle handle) override;

    RendererHandle CreateTexture(u32 width, u32 height, RenderFormat format, u32 mipmaps, u64 flags) override;
    void FreeTexture(RendererHandle handle) override;
    Texture* LookupTexture(RendererHandle handle) override;

    RendererHandle CreateSwapchain(const SwapchainDescription* desc) override;
    void FreeSwapchain(RendererHandle handle) override;
    Swapchain* LookupSwapchain(RendererHandle handle) override;

    RendererHandle CreateShader(const ShaderDescription* desc) override;
    void FreeShader(RendererHandle handle) override;
    Shader* LookupShader(RendererHandle handle) override;

    RendererHandle CreateSampler(FilterMode magFilter, FilterMode minFilter, SamplerAddressMode addressModeU, SamplerAddressMode addressModeV, SamplerAddressMode addressModeW,SamplerMipmapMode mipmapMode) override;
    void FreeSampler(RendererHandle handle) override;
    Sampler* LookupSampler(RendererHandle handle) override;
	//

	//
    void* MapBuffer(RendererHandle handle) override;
    void UnmapBuffer(RendererHandle handle) override;

	void UploadTextureData(RendererHandle handle, Size size, void* data) override;
    //

    // State //
    RendererHandle activePass;
    RendererHandle activeShader;

	FixArray<VulkanDescriptorSlot, Vulkan_UniformDescriptorCount + Vulkan_SamplerDescriptorCount> descriptorSlots;
    //

    // Pipeline stuff or something idk lol //
    DynArray<VulkanCachePipeline> cachePipelines;

	DynArray<VulkanCacheFramebuffer> cacheFramebuffers;
    //

    // Commands //
    RenderInfo BeginRender(RendererHandle swapchain) override;
    void EndRender(RendererHandle swapchain) override;

    void CmdBeginPass(RendererHandle pass, RendererHandle renderTarget, Rect renderArea, Array<Vec4> clearColors, float clearDepth) override;
    void CmdEndPass() override;

    void CmdSetShader(RendererHandle handle) override;

    void CmdBindVertexBuffers(u32 firstBinding, Array<RendererHandle>* buffers, Array<Size>* offsets) override;
    void CmdDrawIndexed(u32 indexCount, u32 firstIndex, u32 vertexOffset, u32 instanceCount, u32 firstInstance) override;

    void CmdSetConstantBuffers(RendererHandle shaderHandle, u32 startSlot, Array<RendererHandle>* constantBuffers) override;
	void CmdSetSamplers(RendererHandle shaderHandle, u32 startSlot, Array<TextureSampler>* textureSamplers) override;

	void CmdUpdateBuffer(RendererHandle buffer, Size start, Size size, void* data) override;
	void CmdPushConstants(RendererHandle shader, Size start, Size size, void* data) override;

	void CmdPipelineBarrier(PipelineBarrier barrier) override; 
    //

};

struct VulkanRendererSpawnInfo {
    RendererSpawnInfo rendererSpawnInfo;
    VkSurfaceKHR vkSurface;
};

void PopulateRendererSpawnerForVulkan(RendererSpawner* spawner, Array<const char*> extensions);

VulkanRenderer* SpawnVulkanRenderer(const VulkanRendererSpawnInfo* info, Allocator* allocator);
void DeleteVulkanRenderer(VulkanRenderer* renderer);

#endif
