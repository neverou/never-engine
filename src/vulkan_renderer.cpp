#include "vulkan_renderer.h"


#if defined(RENDERER_IMPL_VULKAN)
#include <string.h>

#include "std.h"
#include "array.h"
#include "logger.h"
#include "maths.h"
#include "allocators.h"

// ~Improve ~Refactor: this is super dumb because it exits the program if a Vulkan command fails in any way
#define DOVK(statement, msg) { auto __ = statement; if (__ != VK_SUCCESS) { LogError("[vk] %s returned \"%s\"", #statement, GetVulkanErrorTextFromResult(__).data); Assert(false); } }

intern StringView GetVulkanErrorTextFromResult(VkResult errCode) {
	switch (errCode) {
		case VK_SUCCESS: 											return "VK_SUCCESS";
		case VK_NOT_READY: 											return "VK_NOT_READY";
		case VK_TIMEOUT: 											return "VK_TIMEOUT";
		case VK_EVENT_SET: 											return "VK_EVENT_SET";
		case VK_EVENT_RESET: 										return "VK_EVENT_RESET";
		case VK_INCOMPLETE: 										return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: 							return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: 						return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: 						return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: 									return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: 							return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: 							return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: 						return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: 							return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: 							return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: 							return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: 						return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: 								return "VK_ERROR_FRAGMENTED_POOL";
	    case VK_ERROR_UNKNOWN: 										return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY: 							return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE: 						return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION: 								return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: 				return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR: 							return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: 					return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR: 									return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: 								return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: 					return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT: 						return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV: 							return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT: 							return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: 			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR: 									return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR: 									return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR: 							return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR: 						return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT: 						return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
		//case VK_ERROR_OUT_OF_POOL_MEMORY_KHR: 						return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
		//case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR: 					return "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR";
		//case VK_ERROR_FRAGMENTATION_EXT: 							return "VK_ERROR_FRAGMENTATION_EXT";
		//case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: 					return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
		//case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR: 			return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR";
		//case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT: 				return "VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT";
	}
	return "undefined";
}




intern VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return VK_FALSE;

    Log("[vk] validation layer: %s", pCallbackData->pMessage);

    //if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) Assert(false);
    return VK_FALSE;
}

VkResult VkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}






intern VkMemoryPropertyFlags VulkanMemoryPropertyFlagsFromMemoryType(VulkanMemoryType memoryType)
{
	VkMemoryPropertyFlags memoryPropFlags = 0;
	switch (memoryType)
	{
		case VulkanMemory_DeviceLocal: 				memoryPropFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; break;
		case VulkanMemory_HostMappable: 			memoryPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; break;
		case VulkanMemory_DeviceLocalHostMappable: 	memoryPropFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; break;
	}
	return memoryPropFlags;
}



// ~Todo write a better memory allocator
intern VkDeviceMemory AllocateGpuMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryRequirements memReq, VulkanMemoryType memoryType)
{
	// we do a little ~~trolling~~ memory allocation
	VkMemoryPropertyFlags memoryPropFlags = VulkanMemoryPropertyFlagsFromMemoryType(memoryType);
	
	if (memoryPropFlags == 0) 
	{
		LogError("[vk] tried to allocate an invalid memory type!");
		Assert(false);
	}

    // find memory type
    VkPhysicalDeviceMemoryProperties memProperties; // ~Optimize cache this in the renderer?
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    u32 memoryTypeIndex;
    {
        bool foundMemory = false;
        for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
		{
            if ((memReq.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memoryPropFlags) == memoryPropFlags) 
			{
                if (memoryType == VulkanMemory_HostMappable || memoryType == VulkanMemory_DeviceLocalHostMappable)
                {
                    if (memProperties.memoryHeaps[memProperties.memoryTypes[i].heapIndex].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) continue;
                }

                foundMemory = true;
                memoryTypeIndex = i;
				break;
            }
        }

        if (!foundMemory)
            FatalError("[vk] Unable to find valid memory type on GPU!"); // Unable to find suitable buffer memory type!
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory memory;
	DOVK(vkAllocateMemory(device, &allocInfo, NULL, &memory), "Failed to allocate gpu memory lol");

	return memory;
}


intern FixArray<bool, VulkanMemory_Count> GetMemoryTypesPresent(VkPhysicalDevice physicalDevice, VkMemoryRequirements* memReq) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	FixArray<bool, VulkanMemory_Count> memoryTypePresentMask;
	
	for (u32 memoryType = 0; memoryType < VulkanMemory_Count; memoryType++) 
	{
		VkMemoryPropertyFlags memoryPropFlags = VulkanMemoryPropertyFlagsFromMemoryType((VulkanMemoryType)memoryType);
	
		if (memoryPropFlags == 0) 
		{
			LogError("[vk] tried to index an invalid memory type!");
			Assert(false);
		}

		bool foundMemory = false;
        for (u32 i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			// ~CleanUp: make a function to determine whether a reported GPU memory type is worthy of the VulkanMemoryType label
			// so we can easily unify it across the AllocateGpu function as well
            if ((memoryProperties.memoryTypes[i].propertyFlags & memoryPropFlags) == memoryPropFlags) 
			{
                if (memoryType == VulkanMemory_HostMappable || memoryType == VulkanMemory_DeviceLocalHostMappable)
                {
                    if (memoryProperties.memoryHeaps[memoryProperties.memoryTypes[i].heapIndex].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT) continue;
                }

				if (!memReq || (memReq->memoryTypeBits & (1 << i))) {
                	foundMemory = true;
					break;
				}
            }
        }

		memoryTypePresentMask[memoryType] = foundMemory;
	}

	return memoryTypePresentMask;
}




intern VkSamplerMipmapMode GetVulkanSamplerMipmapModeFromSamplerMipmapMode(SamplerMipmapMode mipmapMode)
{
	switch (mipmapMode)
	{
		case SamplerMipmap_Nearest:	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case SamplerMipmap_Linear: 	return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	
	Assert(false);
	return (VkSamplerMipmapMode)0;
}

intern VkFilter GetVulkanFilterFromFilterMode(FilterMode filterMode)
{
	switch (filterMode)
	{
		case Filter_Nearest: 	return VK_FILTER_NEAREST;
		case Filter_Linear: 	return VK_FILTER_LINEAR;
	}
	
	Assert(false);
	return (VkFilter)0;
}

intern VkSamplerAddressMode GetVulkanSamplerAddressModeFromSamplerAddressMode(SamplerAddressMode addressMode)
{
	switch (addressMode)
	{
		case SamplerAddress_Repeat:			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case SamplerAddress_MirroredRepeat:	return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case SamplerAddress_ClampToEdge:	return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case SamplerAddress_ClampToBorder:	return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	}
	Assert(false);
	return((VkSamplerAddressMode)0);
}


intern VkBlendFactor GetVulkanBlendFactorFromBlendFactor(BlendFactor bf /* best initialism in entire program */)
{
    switch (bf) 
	{
        case BlendFactor_Zero:                  return VK_BLEND_FACTOR_ZERO;
        case BlendFactor_One:                   return VK_BLEND_FACTOR_ONE;
        case BlendFactor_Src_Color:             return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor_One_Minus_Src_Color:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor_Dst_Color:             return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor_One_Minus_Dst_Color:   return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor_Src_Alpha:             return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor_One_Minus_Src_Alpha:   return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor_Dst_Alpha:             return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor_One_Minus_Dst_Alpha:   return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor_Src_Alpha_Saturate:    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    }

    Assert(false);
    return (VkBlendFactor)0;
}

intern VkBlendOp GetVulkanBlendOpFromBlendOp(BlendOp blendOp) {
    switch (blendOp) {
        case BlendOp_Add: 				return VK_BLEND_OP_ADD;
        case BlendOp_Subtract:			return  VK_BLEND_OP_SUBTRACT;
        case BlendOp_Reverse_Subtract:	return  VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp_Min: 				return VK_BLEND_OP_MIN;
        case BlendOp_Max: 				return VK_BLEND_OP_MAX;
    }

    Assert(false);
    return (VkBlendOp)0;
}


intern VkImageLayout GetVulkanImageLayoutFromTextureLayout(TextureLayout layout)
{
	switch (layout)
	{
		case TextureLayout_Undefined:						return VK_IMAGE_LAYOUT_UNDEFINED;
		case TextureLayout_General:							return VK_IMAGE_LAYOUT_GENERAL;
		case TextureLayout_ColorAttachmentOptimal:			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case TextureLayout_DepthStencilAttachmentOptimal:	return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case TextureLayout_DepthStencilReadOnlyOptimal:		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case TextureLayout_ShaderReadOnlyOptimal:			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case TextureLayout_TransferSrcOptimal:				return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case TextureLayout_TransferDstOptimal:				return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case TextureLayout_Preinitialised:					return VK_IMAGE_LAYOUT_PREINITIALIZED;

		case TextureLayout_PresentSrc:						return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	Assert(false);
	return (VkImageLayout)0;
}




intern VkCommandBuffer BeginImmediateSubmit(VulkanRenderer* renderer) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.commandPool = renderer->commandPool; // ~Todo use seperate command pool
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	DOVK(vkAllocateCommandBuffers(renderer->device, &allocInfo, &cmd), "Failed to allocate immediate command buffer!");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	DOVK(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin immediate command buffer recording!");

	return cmd;
}


intern void EndImmediateSubmit(VulkanRenderer* renderer, VkQueue queue, VkCommandBuffer cmd) {
	DOVK(vkEndCommandBuffer(cmd), "Failed to end immediate command buffer recording!");

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = NULL;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	// ~Todo figure out how to make this run parallel operations (for transfer)
	DOVK(vkQueueSubmit(queue, 1, &submit, NULL), "Failed to submit immediate command buffer");

	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(renderer->device, renderer->commandPool, 1, &cmd);

	// vkWaitForFences(renderer->device, 1, &immediateFence, true, 1000000000);
	// vkResetFences( renderer->device, 1, &immediateFence);
}









intern VkFormat GetVulkanFormatFromRenderFormat(RenderFormat pf) {
    switch (pf) {
        case FORMAT_R8_SNORM:                  return VK_FORMAT_R8_SNORM;
        case FORMAT_R8_UNORM:                  return VK_FORMAT_R8_UNORM;
        case FORMAT_R8_SINT:                   return VK_FORMAT_R8_SINT;
        case FORMAT_R8_UINT:                   return VK_FORMAT_R8_UINT;

        case FORMAT_R16_SNORM:                 return VK_FORMAT_R16_SNORM;
        case FORMAT_R16_UNORM:                 return VK_FORMAT_R16_UNORM;
        case FORMAT_R16_SINT:                  return VK_FORMAT_R16_SINT;
        case FORMAT_R16_UINT:                  return VK_FORMAT_R16_UINT;
        case FORMAT_R16_FLOAT:                 return VK_FORMAT_R16_SFLOAT;

        case FORMAT_R32_SINT:                  return VK_FORMAT_R32_SINT;
        case FORMAT_R32_UINT:                  return VK_FORMAT_R32_UINT;
        case FORMAT_R32_FLOAT:                 return VK_FORMAT_R32_SFLOAT;

        case FORMAT_R8G8_SNORM:                return VK_FORMAT_R8G8_SNORM;
        case FORMAT_R8G8_UNORM:                return VK_FORMAT_R8G8_UNORM;
        case FORMAT_R8G8_SINT:                 return VK_FORMAT_R8G8_SINT;
        case FORMAT_R8G8_UINT:                 return VK_FORMAT_R8G8_UINT;

        case FORMAT_R16G16_SNORM:              return VK_FORMAT_R16G16_SNORM;
        case FORMAT_R16G16_UNORM:              return VK_FORMAT_R16G16_UNORM;
        case FORMAT_R16G16_SINT:               return VK_FORMAT_R16G16_SINT;
        case FORMAT_R16G16_UINT:               return VK_FORMAT_R16G16_UINT;
        case FORMAT_R16G16_FLOAT:              return VK_FORMAT_R16G16_SFLOAT;

        case FORMAT_R32G32_SINT:               return VK_FORMAT_R32G32_SINT;
        case FORMAT_R32G32_UINT:               return VK_FORMAT_R32G32_UINT;
        case FORMAT_R32G32_FLOAT:              return VK_FORMAT_R32G32_SFLOAT;

        case FORMAT_R32G32B32_FLOAT:           return VK_FORMAT_R32G32B32_SFLOAT;
        case FORMAT_R32G32B32_UINT:            return VK_FORMAT_R32G32B32_UINT;
        case FORMAT_R32G32B32_SINT:            return VK_FORMAT_R32G32B32_SINT;

        case FORMAT_R8G8B8A8_SNORM:            return VK_FORMAT_R8G8B8A8_SNORM;
        case FORMAT_R8G8B8A8_UNORM:            return VK_FORMAT_R8G8B8A8_UNORM;
        case FORMAT_R8G8B8A8_SINT:             return VK_FORMAT_R8G8B8A8_SINT;
        case FORMAT_R8G8B8A8_UINT:             return VK_FORMAT_R8G8B8A8_UINT;
        case FORMAT_R8G8B8A8_UNORM_SRGB:       return VK_FORMAT_R8G8B8A8_SRGB;

        case FORMAT_R16G16B16A16_SNORM:        return VK_FORMAT_R16G16B16A16_SNORM;
        case FORMAT_R16G16B16A16_UNORM:        return VK_FORMAT_R16G16B16A16_UNORM;
        case FORMAT_R16G16B16A16_SINT:         return VK_FORMAT_R16G16B16A16_SINT;
        case FORMAT_R16G16B16A16_UINT:         return VK_FORMAT_R16G16B16A16_UINT;
        case FORMAT_R16G16B16A16_FLOAT:        return VK_FORMAT_R16G16B16A16_SFLOAT;

        case FORMAT_R32G32B32A32_SINT:         return VK_FORMAT_R32G32B32A32_SINT;
        case FORMAT_R32G32B32A32_UINT:         return VK_FORMAT_R32G32B32A32_UINT;
        case FORMAT_R32G32B32A32_FLOAT:        return VK_FORMAT_R32G32B32A32_SFLOAT;

        case FORMAT_B8G8R8A8_SNORM:            return VK_FORMAT_B8G8R8A8_SNORM;
        case FORMAT_B8G8R8A8_UNORM:            return VK_FORMAT_B8G8R8A8_UNORM;
        case FORMAT_B8G8R8A8_SINT:             return VK_FORMAT_B8G8R8A8_SINT;
        case FORMAT_B8G8R8A8_UINT:             return VK_FORMAT_B8G8R8A8_UINT;
        case FORMAT_B8G8R8A8_UNORM_SRGB:       return VK_FORMAT_B8G8R8A8_SRGB;

        case FORMAT_D32_FLOAT:				   return VK_FORMAT_D32_SFLOAT;
        case FORMAT_D32_FLOAT_S8_UINT:		   return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case FORMAT_D24_UNORM_S8_UINT:		   return VK_FORMAT_D24_UNORM_S8_UINT;
    }

    Assert(false);
    return (VkFormat)0;
}

intern VkPresentModeKHR GetVulkanPresentModeFromPresentMode(PresentMode presentMode) {
    switch (presentMode) {
        case PRESENTMODE_DOUBLE_BUFFER_IMMEDIATE:   return VK_PRESENT_MODE_IMMEDIATE_KHR;
        case PRESENTMODE_DOUBLE_BUFFER_FIFO:        return VK_PRESENT_MODE_FIFO_KHR;
        case PRESENTMODE_TRIPLE_BUFFER_FIFO:        return VK_PRESENT_MODE_FIFO_KHR;
        case PRESENTMODE_TRIPLE_BUFFER_MAILBOX:     return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    Assert(false);
    return (VkPresentModeKHR)0;
}





// loll conjure as in: "idc how you get it, just get me one"
intern VulkanCacheFramebuffer ConjureSuitableFramebuffer(VulkanRenderer* renderer, RendererHandle renderPass, RendererHandle renderTarget)
{
	Assert(renderPass != 0);
	Assert(renderTarget != 0);

	u32 candidateIndex = -1;

	// Search if there is a pipeline that fits our needs
	ForIdx (renderer->cacheFramebuffers, idx)
	{
		auto* candidate = &renderer->cacheFramebuffers[idx];

		if (candidate->linkedPass == renderPass &&
			candidate->linkedRenderTarget == renderTarget)
		{
			candidateIndex = idx;
			break;
		}
	}

	
	VulkanCacheFramebuffer result { };

	if (candidateIndex != -1)
	{
		result = renderer->cacheFramebuffers[candidateIndex];
	}
	else
	{
		// create a new cache pipeline
        Log("[vk] creating new framebuffer for render target - render pass combination %d-%d", renderTarget, renderPass);


		auto* renderTargetPtr = ((VulkanRenderTarget*)renderer->LookupRenderTarget(renderTarget));

		VkFramebuffer vkFramebuffer;

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = ((VulkanPass*)renderer->LookupPass(renderPass))->renderPass;
		framebufferInfo.attachmentCount = renderTargetPtr->attachments.size;

		// ~Todo:
		// make a temporary memory arena for each thread
		// so when we do multithreading we can use that temporary allocator
		auto textureViews = MakeArray<VkImageView>(renderTargetPtr->attachments.size, Frame_Arena);

		ForIdx(textureViews, idx)
		{
			textureViews[idx] = cast(VulkanTexture*, renderer->LookupTexture(renderTargetPtr->attachments.data[idx]))->textureView;
		}

		framebufferInfo.pAttachments = textureViews.data;
		framebufferInfo.width = renderTargetPtr->width;
		framebufferInfo.height = renderTargetPtr->height;
		framebufferInfo.layers = 1;

		DOVK(vkCreateFramebuffer(renderer->device, &framebufferInfo, NULL, &vkFramebuffer),
					"Failed to create framebuffer!");

		result.linkedRenderTarget = renderTarget;
		result.linkedPass = renderPass;

		result.framebuffer = vkFramebuffer;

		ArrayAdd(&renderer->cacheFramebuffers, result);
	}

	return result;
}



intern VkCompareOp GetVulkanCompareOpFromCompareOp(CompareOp op)
{
	switch (op)
	{
		case CompareOp_Never: 				return VK_COMPARE_OP_NEVER;
		case CompareOp_Less: 				return VK_COMPARE_OP_LESS;
		case CompareOp_Equal: 				return VK_COMPARE_OP_EQUAL;
		case CompareOp_Less_Or_Equal: 		return VK_COMPARE_OP_LESS_OR_EQUAL;
		case CompareOp_Greater: 			return VK_COMPARE_OP_GREATER;
		case CompareOp_Not_Equal: 			return VK_COMPARE_OP_NOT_EQUAL;
		case CompareOp_Greater_Or_Equal: 	return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case CompareOp_Always: 				return VK_COMPARE_OP_ALWAYS;
	}
	
	Assert(false);
	return (VkCompareOp)0;
}


intern void UpdatePipeline(VulkanRenderer* renderer) {
	if (renderer->activePass == 0 || renderer->activeShader == 0) return; // ~Refactor: PRINT WHEN THIS HAPPENS; THIS CAUSED ME SOOO MUCH PAINNNNNN!!!!!

	u32 candidateIndex = -1;

	// Search if there is a pipeline that fits our needs
	ForIdx (renderer->cachePipelines, idx)
	{
		auto* candidate = &renderer->cachePipelines[idx];

		if (candidate->linkedPass == renderer->activePass &&
			candidate->linkedShader == renderer->activeShader)
		{
			candidateIndex = idx;
			break;
		}
	}

	VkPipeline pipeline;

	if (candidateIndex != -1)
	{
		pipeline = renderer->cachePipelines[candidateIndex].pipeline;
	}
	else
	{
        // create a new pipeline to fit our needs
        VulkanCachePipeline cachePipeline;


        auto* shader = (VulkanShader*)renderer->LookupShader(renderer->activeShader);
        auto* pass = renderer->LookupPass(renderer->activePass);

        cachePipeline.linkedShader = renderer->activeShader;
        cachePipeline.linkedPass = renderer->activePass;

        Log("[vk] creating new pipeline for shader pass combination %d-%d", renderer->activeShader, renderer->activePass);


        {
            // create the render pipeline

            // vertex input stuff
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            // ~Cleanup
            auto vertexInputBindings = MakeDynArray<VkVertexInputBindingDescription>(0, Frame_Arena);
            defer(FreeDynArray(&vertexInputBindings));

            auto vertexInputAttributes = MakeDynArray<VkVertexInputAttributeDescription>(0, Frame_Arena);
            defer(FreeDynArray(&vertexInputAttributes));

            For (shader->vertexInputLayouts) {
                // create vertex input description
                VkVertexInputBindingDescription bindingDescription = {};
                bindingDescription.binding = it->binding;
                bindingDescription.stride = it->stride;

                switch (it->inputRate) {
                    case VERTEX_INPUT_RATE_PER_VERTEX: {
                        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                        break;
                    }
                    case VERTEX_INPUT_RATE_PER_INSTANCE: {
                        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                        break;
                    }

                    default: {
                        Assert(false);
                        break;
                    }
                }

                ArrayAdd(&vertexInputBindings, bindingDescription);

                // ~Hack HACK!!
                for (u32 i = 0; i < it->elementCount; i++) {
                    auto itElem = it->elements + i;

                    VkVertexInputAttributeDescription attributeDescription;
                    attributeDescription.binding = it->binding;
                    attributeDescription.location = itElem->location;
                    attributeDescription.format = GetVulkanFormatFromRenderFormat(itElem->format);
                    attributeDescription.offset = itElem->offset;

                    ArrayAdd(&vertexInputAttributes, attributeDescription);
                }
            }

            vertexInputInfo.vertexBindingDescriptionCount = vertexInputBindings.size;
            vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.data;
            vertexInputInfo.vertexAttributeDescriptionCount = vertexInputAttributes.size;
            vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data;




            // Input assembler
            VkPipelineInputAssemblyStateCreateInfo inputAssemblerInfo = {};
            inputAssemblerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

            switch (shader->primitiveType) {
                case Primative_Triangle_List: {
                    inputAssemblerInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                    break;
                }

                case Primative_Line_List: {
                    inputAssemblerInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                    break;
                }


                default: {
                    Assert(false);
                    break;
                }
            }

            inputAssemblerInfo.primitiveRestartEnable = VK_FALSE;






            // viewport

            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            // viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            // viewportState.pScissors = &scissor;




            // rasterizer
            VkPolygonMode fillMode;
            VkCullModeFlags cullMode;
            VkFrontFace frontFace;

            switch (shader->cullMode) {
                case Cull_None: {
                    cullMode = VK_CULL_MODE_NONE;
                    break;
                }
                case Cull_Front: {
                    cullMode = VK_CULL_MODE_FRONT_BIT;
                    break;
                }
                case Cull_Back: {
                    cullMode = VK_CULL_MODE_BACK_BIT;
                    break;
                }
                case Cull_FrontAndBack: {
                    cullMode = VK_CULL_MODE_FRONT_AND_BACK;
                    break;
                }

                default: {
                    Assert(false);
                    break;
                }
            }

            switch (shader->frontFace) {
                case FrontFace_CW: {
                    frontFace = VK_FRONT_FACE_CLOCKWISE;
                    break;
                }
                case FrontFace_CCW: {
                    frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                    break;
                }

                default: {
                    Assert(false);
                    break;
                }
            }

            switch (shader->fillMode) {
                case PolygonMode_Fill: {
                    fillMode = VK_POLYGON_MODE_FILL;
                    break;
                }
                case PolygonMode_Line: {
                    fillMode = VK_POLYGON_MODE_LINE;
                    break;
                }
                case PolygonMode_Point: {
                    fillMode = VK_POLYGON_MODE_POINT;
                    break;
                }

                default: {
                    Assert(false);
                    break;
                }
            }

            VkPipelineRasterizationStateCreateInfo rasterizer = {};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = fillMode;
            rasterizer.lineWidth = shader->lineWidth;
            rasterizer.cullMode = cullMode;
            rasterizer.frontFace = frontFace;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f; // optional
            rasterizer.depthBiasClamp = 0.0f; // optional
            rasterizer.depthBiasSlopeFactor = 0.0f; // optional





            // no msaa for now, we'll deal with layer
            VkPipelineMultisampleStateCreateInfo multisampling = {};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading = 1.0f; // optional
            multisampling.pSampleMask = NULL; // optional
            multisampling.alphaToCoverageEnable = VK_FALSE; // optional
            multisampling.alphaToOneEnable = VK_FALSE; // optional

            // depth buffer stuff (and technically stencil buffer but idk why we'd need one rn)








            VkDynamicState dynamicStates[] = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            VkPipelineDynamicStateCreateInfo dynamicState = {};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamicStates;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0;
            pipelineLayoutInfo.pSetLayouts = NULL; // optional


			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
			pushConstantRange.offset = 0;
			pushConstantRange.size = 128; // ~Hack ~Incomplete

            pipelineLayoutInfo.pushConstantRangeCount = 1; // optional
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // optional

            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &renderer->descriptorSetLayout;

            DOVK(vkCreatePipelineLayout(renderer->device, &pipelineLayoutInfo, NULL, &cachePipeline.layout), "Failed to create pipeline layout");















			// @@ShaderBlending

            // color blending stage
            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};

            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = shader->blendEnable ? VK_TRUE : VK_FALSE;

            colorBlendAttachment.srcColorBlendFactor = GetVulkanBlendFactorFromBlendFactor(shader->blendSrcColor);
            colorBlendAttachment.dstColorBlendFactor = GetVulkanBlendFactorFromBlendFactor(shader->blendDestColor);
            colorBlendAttachment.colorBlendOp = GetVulkanBlendOpFromBlendOp(shader->blendOpColor);
            colorBlendAttachment.srcAlphaBlendFactor = GetVulkanBlendFactorFromBlendFactor(shader->blendSrcAlpha);
            colorBlendAttachment.dstAlphaBlendFactor = GetVulkanBlendFactorFromBlendFactor(shader->blendDestAlpha);
            colorBlendAttachment.alphaBlendOp = GetVulkanBlendOpFromBlendOp(shader->blendOpAlpha);

			// ~Hack SUPER HACK!!!
			auto colorBlendAttachments = MakeArray<VkPipelineColorBlendAttachmentState>(pass->colorAttachmentCount, Frame_Arena);
			for (u32 i = 0; i < pass->colorAttachmentCount; ++i)
				colorBlendAttachments[i] = colorBlendAttachment;

            // we currently have no use for color blend state stuff, so yea
            VkPipelineColorBlendStateCreateInfo colorBlending = {};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY; // optional

            colorBlending.attachmentCount = colorBlendAttachments.size;
            colorBlending.pAttachments = colorBlendAttachments.data;
            colorBlending.blendConstants[0] = 0.0f; // optional
            colorBlending.blendConstants[1] = 0.0f; // optional
            colorBlending.blendConstants[2] = 0.0f; // optional
            colorBlending.blendConstants[3] = 0.0f; // optional










            auto shaderStages = MakeArray<VkPipelineShaderStageCreateInfo>(shader->stages.size, Frame_Arena);
            defer(FreeArray(&shaderStages));

            ForIdx (shaderStages, idx) {
                VkPipelineShaderStageCreateInfo shaderStageInfo = {};
                shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

                auto shaderEntry = shader->stages[idx];
                switch (shaderEntry.type) {
                    case SHADER_VERTEX: {
                        shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                        break;
                    }
                    case SHADER_PIXEL: {
                        shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                        break;
                    }
                    default: Assert(false); break; // Non-valid shader type
                }

                shaderStageInfo.module = shaderEntry.shader;
                shaderStageInfo.pName = "main"; // ??? i think this is required but idk lol

                shaderStages[idx] = shaderStageInfo;
            }



			// Depth stencil state

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = shader->depthTestEnable ? VK_TRUE : VK_FALSE;
			depthStencil.depthWriteEnable = shader->depthWriteEnable ? VK_TRUE : VK_FALSE;
			depthStencil.depthCompareOp = GetVulkanCompareOpFromCompareOp(shader->depthCompareOp);

            // create the pipeline
            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = shaderStages.size;
            pipelineInfo.pStages = shaderStages.data;

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssemblerInfo;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;

            pipelineInfo.layout = cachePipeline.layout;
            pipelineInfo.renderPass = ((VulkanPass*)pass)->renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // optional
            pipelineInfo.basePipelineIndex = -1; // optional



            DOVK(vkCreateGraphicsPipelines(renderer->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &cachePipeline.pipeline), "Failed to create render pipeline");
        }

        ArrayAdd(&renderer->cachePipelines, cachePipeline); // oops i forgot this the first time lol, thats the entire point of this function

        pipeline = cachePipeline.pipeline;
    }

    vkCmdBindPipeline(renderer->mainCommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}





void PopulateRendererSpawnerForVulkan(RendererSpawner* spawner, Array<const char*> instanceExtensions) {
    Log("[vk] Creating Vulkan renderer spawner...");

    spawner->flags |= RENDERER_SPAWNER_FLAG_SUPPORTS_GRAPHICS_PROCESSOR_AFFINITY;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Kelp Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Kelp Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;


    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

	DynArray<const char*> sumInstanceExtensions = MakeDynArray<const char*>(0, Frame_Arena);
	defer(FreeDynArray(&sumInstanceExtensions));

	ArrayResize(&sumInstanceExtensions, instanceExtensions.size);
	Memcpy(sumInstanceExtensions.data, instanceExtensions.data, instanceExtensions.size * sizeof(const char*));
	ArrayAdd(&sumInstanceExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	For (sumInstanceExtensions) {
		Log("[vk] instance ext request: %s", *it);
	}

    instanceInfo.enabledExtensionCount = sumInstanceExtensions.size;
    instanceInfo.ppEnabledExtensionNames = sumInstanceExtensions.data;

    // validation layers?
#if defined(BUILD_DEBUG)

    Log("[vk] Validation layers are enabled because we are in debug mode");

    DynArray<const char*> instanceValidationLayers = MakeDynArray<const char*>(0, Frame_Arena);
    defer(FreeDynArray(&instanceValidationLayers));
    ArrayAdd(&instanceValidationLayers, "VK_LAYER_KHRONOS_validation");
    // ArrayAdd(&instanceValidationLayers, "VK_LAYER_RENDERDOC_Capture");

    {


        u32 validLayerCount;
        vkEnumerateInstanceLayerProperties(&validLayerCount, NULL);

		// fetch a list of valid validation layers
        SArray<VkLayerProperties> validLayers = MakeArray<VkLayerProperties>(validLayerCount, Frame_Arena);
        defer(FreeArray(&validLayers));
        vkEnumerateInstanceLayerProperties(&validLayerCount, validLayers.data);

        for (size_t checkLayerIdx = 0; checkLayerIdx < instanceValidationLayers.size; checkLayerIdx++) {
            bool found = false;
            auto checkLayerName = instanceValidationLayers[checkLayerIdx];

            For (validLayers) {
                if (strcmp(it->layerName, checkLayerName) == 0) {
                    found = true;
                }
            }

            if (!found) {
                String msg = TempString();
                msg.Concat("[vk] ");
                msg.Concat("Layer ");
                msg.Concat(checkLayerName);
                msg.Concat(" is not supported! Skipping activation... \n");

                ArrayRemoveAt(&instanceValidationLayers, checkLayerIdx);
                checkLayerIdx--;

                LogWarn(GetStringView(msg));
            }
        }


    }

    {
        String msg = TempString();

        msg.Concat("[vk] Enabling Vulkan instance validation layers:\n");

        For (instanceValidationLayers) {
            const char* validation_layer = *it;
            msg.Concat("    ");
            msg.Concat(validation_layer);
            msg.Concat("\n");
        }

        Log(GetStringView(msg));
    }

    instanceInfo.enabledLayerCount = instanceValidationLayers.size;
    instanceInfo.ppEnabledLayerNames = instanceValidationLayers.data;



	// Debug messanger

	// ~Hack
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	{

		debugCreateInfo = {};
	    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	    debugCreateInfo.pfnUserCallback = VkDebugCallback;

	    instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

	}

#endif

    // Create an instance
    VkInstance instance;
    DOVK(vkCreateInstance(&instanceInfo, NULL, &instance), "Failed to create VkInstance!");

    spawner->platformApiInstance = instance;



    ///

    u32 physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
    DynArray<VkPhysicalDevice> devices = MakeDynArray<VkPhysicalDevice>(physicalDeviceCount, Frame_Arena);
    defer(FreeDynArray(&devices));

    // reserve space on the array to not reallocate when we add things to it
    ArrayResize(&spawner->graphicsProcessors, physicalDeviceCount);

    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, devices.data);

    ForIdx (devices, i) {
        auto it = devices[i];

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(it, &props);

        GraphicsProcessor proc = {};
        proc.platformHandle = it;
        // ~Todo: better solution, maybe copy_graphics_processor()?
        memcpy(proc.deviceName, props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
        proc.vendorID = props.vendorID;
        proc.deviceID = props.deviceID;

        spawner->graphicsProcessors[i] = proc;
    }


    // ~Todo:
    // populate the swapchain capabilities
    {

    }
}



enum VkQueueUsage {
    VK_QUEUE_USAGE_NONE = 0,
    VK_QUEUE_USAGE_GRAPHICS = 1,
    VK_QUEUE_USAGE_TRANSFER = 2,
    VK_QUEUE_USAGE_SPARSE_BINDING = 4,
    VK_QUEUE_USAGE_PRESENT = 8,
};


struct VkQueueUsageCandidate {
    u32 usageFlags;
    u32 queueCount;
    u16 queueCapabilityCount;
    u32 familyIndex;
};

struct VkQueueRequest {
    VkQueueUsage usage;

    VkQueue queue;
    u32 familyIndex;
    u32 queueIndex;
};





VulkanRenderer* SpawnVulkanRenderer(const VulkanRendererSpawnInfo* spawnInfo, Allocator* allocator)
{
    Log("[vk] Spawning Vulkan renderer...");

    VulkanRenderer* renderer = new (AceAlloc(sizeof(VulkanRenderer), allocator)) VulkanRenderer;
    renderer->type = RENDERER_VULKAN;
    renderer->allocator = allocator;

    // State info initialization //
    renderer->activePass = 0;
    renderer->activeShader = 0;
    renderer->cachePipelines = MakeDynArray<VulkanCachePipeline>();
    renderer->cacheFramebuffers = MakeDynArray<VulkanCacheFramebuffer>();

	For (renderer->descriptorSlots) // Zero stuff, make sure the type is none
		*it = VulkanDescriptorSlot { };
    //

    // Resource management stuffs //
    renderer->nextHandle = 1; // set this to 1 because 0 is reserved for an undefined resource handle

    renderer->bufferHandleKeys = MakeDynArray<RendererHandle>();
    renderer->buffers = MakeDynArray<VulkanBuffer>();

    renderer->passHandleKeys = MakeDynArray<RendererHandle>();
    renderer->passes = MakeDynArray<VulkanPass>();

    renderer->renderTargetHandleKeys = MakeDynArray<RendererHandle>();
    renderer->renderTargets = MakeDynArray<VulkanRenderTarget>();

    renderer->textureHandleKeys = MakeDynArray<RendererHandle>();
    renderer->textures = MakeDynArray<VulkanTexture>();

    renderer->swapchainHandleKeys = MakeDynArray<RendererHandle>();
    renderer->swapchains = MakeDynArray<VulkanSwapchain>();

    renderer->shaderHandleKeys = MakeDynArray<RendererHandle>();
    renderer->shaders = MakeDynArray<VulkanShader>();

    renderer->samplerHandleKeys = MakeDynArray<RendererHandle>();
    renderer->samplers = MakeDynArray<VulkanSampler>();
    //

    renderer->instance = (VkInstance)spawnInfo->rendererSpawnInfo.spawner.platformApiInstance;
    renderer->surface = spawnInfo->vkSurface;

    renderer->physicalDevice = (VkPhysicalDevice)spawnInfo->rendererSpawnInfo.chosenGraphicsProcessor.platformHandle;



#if defined(BUILD_DEBUG)
	// ~Todo destroy this when the vulkan renderer is destroyed
	// ~Hack
	{
 		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		debugCreateInfo = {};
	    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	    debugCreateInfo.pfnUserCallback = VkDebugCallback;

		DOVK(VkCreateDebugUtilsMessengerEXT(renderer->instance, &debugCreateInfo, NULL, &renderer->debugUtilsMessenger), "Failed to create debug messenger!");
	}
#endif


    { // physical device property stuff
        // ~Todo: make sure our gpu is capable of doing what we need to do
        // ex: check if it has present capability & swap chain capability

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(renderer->physicalDevice, &props);

        String gpuLog = TempString();
        gpuLog.Concat("[vk] ");
        gpuLog.Concat("Chosen GPU: ");
        gpuLog.Concat(props.deviceName);
        Log(gpuLog);
		
		Log("[vk] maxPushConstantsSize=%lu", props.limits.maxPushConstantsSize);

		
		Log("[vk] physical device memory types: ");
		auto memoryTypes = GetMemoryTypesPresent(renderer->physicalDevice, NULL);
		ForIdx (memoryTypes, idx)
		{
			StringView name = "Unknown";
			switch (idx)
			{
				case VulkanMemory_DeviceLocal: 				name = "VulkanMemory_DeviceLocal"; break;
				case VulkanMemory_HostMappable: 			name = "VulkanMemory_HostMappable"; break;
				case VulkanMemory_DeviceLocalHostMappable: 	name = "VulkanMemory_DeviceLocalHostMappable"; break;
				// case VulkanMemory_HostMappableCached:		name = "VulkanMemory_HostMappableCached"; break; 
			}

			Log("    - %s", name.data);
		}
    }





    DynArray<VkDeviceQueueCreateInfo> queueCreateInfos = MakeDynArray<VkDeviceQueueCreateInfo>(0, Frame_Arena);
    defer(FreeDynArray(&queueCreateInfos));

    auto queuePriorities = MakeDynArray<SArray<float>>(0, Frame_Arena); // whyyyyy vulkannnn (Legacy)
    defer({
              for (size_t i = 0; i < queuePriorities.size; i++)
              {
                  FreeArray(&queuePriorities[i]);
              }
              FreeDynArray(&queuePriorities);
          });


    FixArray<VkQueueRequest, 3> queueRequests;
    queueRequests[0] = VkQueueRequest { VK_QUEUE_USAGE_GRAPHICS };
    queueRequests[1] = VkQueueRequest { VK_QUEUE_USAGE_PRESENT };
    queueRequests[2] = VkQueueRequest { VK_QUEUE_USAGE_TRANSFER };

    // NOTE:
    // this queue allocator is super limited and doesnt handle multiple queues per family at all, so
    // if thats needed in the future, then we need to remake this

    { // queue things
        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(renderer->physicalDevice, &queueFamilyCount, NULL);

        auto queueFamilies = MakeArray<VkQueueFamilyProperties>(queueFamilyCount, Frame_Arena);
        defer(FreeArray(&queueFamilies));
        auto queueCandidatePool = MakeArray<VkQueueUsageCandidate>(queueFamilyCount, Frame_Arena);
        defer(FreeArray(&queueCandidatePool));

        vkGetPhysicalDeviceQueueFamilyProperties(renderer->physicalDevice, &queueFamilyCount, queueFamilies.data);



        ForIdx (queueFamilies, i) {
            VkQueueFamilyProperties queueFamily = queueFamilies[i];

            VkQueueUsageCandidate candidate = {};
            candidate.queueCount = queueFamily.queueCount;
            candidate.familyIndex = i;

			// String logData = TempString(0);
			// // defer(FreeString(&logData));
			// logData.Concat(TPrint("%u (x%u) -> ", i, queueFamily));

            VkBool32 supportsPresenting;
            DOVK(vkGetPhysicalDeviceSurfaceSupportKHR(renderer->physicalDevice, i, renderer->surface, &supportsPresenting), "Could not query queue family present ability!");
            if (supportsPresenting) {
                candidate.queueCapabilityCount++;
                candidate.usageFlags |= VK_QUEUE_USAGE_PRESENT;

				// logData.Concat("+Present ");
            }
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                candidate.queueCapabilityCount++;
                candidate.usageFlags |= VK_QUEUE_USAGE_GRAPHICS;

				// logData.Concat("+Graphics ");
            }
            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                candidate.queueCapabilityCount++;
                candidate.usageFlags |= VK_QUEUE_USAGE_TRANSFER;

				// logData.Concat("+Transfer ");
            }
            if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
                candidate.queueCapabilityCount++;
                candidate.usageFlags |= VK_QUEUE_USAGE_SPARSE_BINDING;

				// logData.Concat("+Sparse ");
            }

			// Log(logData);

            queueCandidatePool[i] = candidate;
        }

        // time to allocate some queueueueueues

        auto queueAmountPerFamily = MakeArray<u32>(queueFamilies.size, Engine_Arena);
        defer(FreeArray(&queueAmountPerFamily));
        ForIdx (queueAmountPerFamily, i) queueAmountPerFamily[i] = 0;

        ForIt (queueRequests, request) {
            VkQueueUsageCandidate* candidate = NULL;
            for (Size idx = 0; idx < queueCandidatePool.size; idx++) {
                auto it = &queueCandidatePool[idx];
                bool isValid = (request->usage & it->usageFlags);
                bool isBetter = (!candidate) || (candidate->queueCapabilityCount > it->queueCapabilityCount);

                if (isValid && isBetter)
                    candidate = it;
            }

            Assert(candidate);
            // Error("Could not find a valid Vulkan queue family candidate!");

            request->familyIndex = candidate->familyIndex;
            request->queueIndex = queueAmountPerFamily[candidate->familyIndex];

            queueAmountPerFamily[candidate->familyIndex]++;
        }

        ArrayResize(&queuePriorities, queueAmountPerFamily.size);
        for (u32 queueFamilyIndex = 0; queueFamilyIndex < queueAmountPerFamily.size; queueFamilyIndex++) {
            // make the queue create info
            u32 queueAmount = queueAmountPerFamily[queueFamilyIndex];

            queuePriorities[queueFamilyIndex] = MakeArray<float>(queueAmount, Frame_Arena); //"Vafan AMD, Varfor?!" - Dillon 2021//

            if (queueAmount > 0) {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
                queueCreateInfo.queueCount = queueAmount;

                // this is pain
                for (size_t i = 0; i < queuePriorities[queueFamilyIndex].size; i++) {
                    queuePriorities[queueFamilyIndex][i] = 1;
                }

                queueCreateInfo.pQueuePriorities = queuePriorities[queueFamilyIndex].data;
                ArrayAdd(&queueCreateInfos, queueCreateInfo);
            }
        }

    }


    {
        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.pQueueCreateInfos = queueCreateInfos.data;
        createInfo.queueCreateInfoCount = queueCreateInfos.size;


        FixArray<const char*, 1> deviceExtensions;
        deviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

        createInfo.ppEnabledExtensionNames = deviceExtensions.data;
        createInfo.enabledExtensionCount = deviceExtensions.size;

		For (deviceExtensions) {
			Log("[vk] device ext requested: %s", *it);
		}

        // ~Todo: ~Important check if our physical device supports the requested extensions

        DOVK(vkCreateDevice(renderer->physicalDevice, &createInfo, NULL, &renderer->device), "Failed to create Vulkan device!");


        // retrieve the queues
        For (queueRequests) {
            auto request = it;
            vkGetDeviceQueue(renderer->device, request->familyIndex, request->queueIndex, &request->queue);
        }
    }

    { // retrieve our queues
        // ~Hack: this is super hacky, hardcoded, and probably temporary, but we'll go through all the requested queues and fill them into the devices queue variables
        For (queueRequests) {
            auto request = it;
            if (request->usage == VK_QUEUE_USAGE_GRAPHICS) {
                renderer->graphicsQueue = request->queue;
                renderer->graphicsQueueFamily = request->familyIndex;
            }
            else if (request->usage == VK_QUEUE_USAGE_PRESENT) {
                renderer->presentQueue = request->queue;
                renderer->presentQueueFamily = request->familyIndex;
            }
            else if (request->usage == VK_QUEUE_USAGE_TRANSFER) {
                renderer->transferQueue = request->queue;
                renderer->transferQueueFamily = request->familyIndex;
            }
        }

    }

	Log("[vk] graphicsQueueFamily=%lu", renderer->graphicsQueueFamily);
	Log("[vk] presentQueueFamily=%lu", renderer->presentQueueFamily);
	Log("[vk] transferQueueFamily=%lu", renderer->transferQueueFamily);

    {
        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.pNext = NULL;

        commandPoolInfo.queueFamilyIndex = renderer->graphicsQueueFamily;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // ~Todo make a seperate command pool for the immediate command buffers
        DOVK(vkCreateCommandPool(renderer->device, &commandPoolInfo, NULL, &renderer->commandPool),
                       "Failed to create command pool!");

        // create command buffers
        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.pNext = NULL;
        allocateInfo.commandPool = renderer->commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        allocateInfo.commandBufferCount = 4;
        DOVK(vkAllocateCommandBuffers(renderer->device, &allocateInfo, &renderer->mainCommandList), "Failed to allocate command list!");
    }


    { // create semaphores and fence
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = NULL;
        semaphoreCreateInfo.flags = 0;
        DOVK(vkCreateSemaphore(renderer->device, &semaphoreCreateInfo, NULL, &renderer->renderSemaphore), "Failed to create render semaphore!");
        DOVK(vkCreateSemaphore(renderer->device, &semaphoreCreateInfo, NULL, &renderer->presentSemaphore), "Failed to create present semaphore!");

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = NULL;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        DOVK(vkCreateFence(renderer->device, &fenceCreateInfo, NULL, &renderer->renderFence), "Failed to create fence");
    }

    { // descriptor stuff

        { // alloc pool
            // ~Note:
            // there may be some optimization we could do with descriptors and having multiple sets,
            // for now we're just using one though
            VkDescriptorPoolSize uniformPool = {};
            uniformPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformPool.descriptorCount = Vulkan_UniformDescriptorCount * Vulkan_DescriptorSetCount;

            VkDescriptorPoolSize samplerPool = {};
            samplerPool.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerPool.descriptorCount = Vulkan_SamplerDescriptorCount * Vulkan_DescriptorSetCount;

            VkDescriptorPoolSize pools[2] = {};
            pools[0] = uniformPool;
            pools[1] = samplerPool;

            VkDescriptorPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            createInfo.pNext = NULL;
            createInfo.flags = 0;
            createInfo.maxSets = Vulkan_DescriptorSetCount;
            createInfo.poolSizeCount = 2;
            createInfo.pPoolSizes = pools;

            DOVK(vkCreateDescriptorPool(renderer->device, &createInfo, NULL, &renderer->descriptorPool), "Failed to create descriptor pool!");
        }

        { // Create layout
            VkDescriptorSetLayoutBinding bindings[Vulkan_UniformDescriptorCount + Vulkan_SamplerDescriptorCount];

            u32 bindSlot = 0;
            // uniform buffer slots
            for (u32 i = 0; i < Vulkan_UniformDescriptorCount; i++) {
                VkDescriptorSetLayoutBinding binding = {};

                binding.binding = bindSlot;
                binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                binding.descriptorCount = 1;

                bindings[bindSlot++] = binding;
            }
            // sampler slots
            for (u32 i = 0; i < Vulkan_SamplerDescriptorCount; i++) {
                VkDescriptorSetLayoutBinding binding = {};

                binding.binding = bindSlot;
                binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
                binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                binding.descriptorCount = 1;

                bindings[bindSlot++] = binding;
            }


            VkDescriptorSetLayoutCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = Vulkan_UniformDescriptorCount + Vulkan_SamplerDescriptorCount;
            createInfo.pBindings = bindings;

            DOVK(vkCreateDescriptorSetLayout(renderer->device, &createInfo, NULL, &renderer->descriptorSetLayout), "Failed to create Descriptor Set Layout");
        }

        {
            renderer->descriptorSets = MakeArray<VkDescriptorSet>(Vulkan_DescriptorSetCount);

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = NULL;
            allocInfo.descriptorPool = renderer->descriptorPool;
            allocInfo.descriptorSetCount = renderer->descriptorSets.size;

            auto layouts = MakeArray<VkDescriptorSetLayout>(renderer->descriptorSets.size, Frame_Arena);
            defer(FreeArray(&layouts));
            For (layouts) {
                *it = renderer->descriptorSetLayout;
            }
            allocInfo.pSetLayouts = layouts.data;

            DOVK(vkAllocateDescriptorSets(renderer->device, &allocInfo, renderer->descriptorSets.data), "F");
        }
    }

    Log("[vk] Finished spawning Vulkan renderer");

    return renderer;
}

void DeleteVulkanRenderer(VulkanRenderer* renderer) {
    Log("[vk] Despawning Vulkan renderer...");


    {   // Free the cache framebuffers
        For (renderer->cacheFramebuffers) {
            vkDestroyFramebuffer(renderer->device, it->framebuffer, NULL);
        }
        FreeDynArray(&renderer->cachePipelines);
    }

    {   // Free the cache pipelines
        For (renderer->cachePipelines) {
            vkDestroyPipelineLayout(renderer->device, it->layout, NULL);
            vkDestroyPipeline(renderer->device, it->pipeline, NULL);
        }
        FreeDynArray(&renderer->cachePipelines);
    }


    { // Delete all non-free'd resources
        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->bufferHandleKeys.size != 0) {
            RendererHandle handle = renderer->bufferHandleKeys[0];

            // ~~Todo: replace with an actual warn
            LogWarn("[vk] Render Buffer not freed on exit: HANDLE=%d", handle);

            renderer->FreeBuffer(handle);
        }

        FreeDynArray(&renderer->buffers);
        FreeDynArray(&renderer->bufferHandleKeys);

        ////

        ////

        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->passHandleKeys.size != 0) {
            RendererHandle handle = renderer->passHandleKeys[0];

            // ~~Todo: replace with an actual warn
            LogWarn("[vk] Render Pass not freed on exit: HANDLE=%d", handle);

            renderer->FreePass(handle);
        }

        FreeDynArray(&renderer->passes);
        FreeDynArray(&renderer->passHandleKeys);


        ////

        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->renderTargetHandleKeys.size != 0) {
            RendererHandle handle = renderer->renderTargetHandleKeys[0];

            // ~~Todo: replace with an actual warn
            LogWarn("[vk] Render Target not freed on exit: HANDLE=%d", handle);

            renderer->FreeRenderTarget(handle);
        }

        FreeDynArray(&renderer->renderTargets);
        FreeDynArray(&renderer->renderTargetHandleKeys);



        ////


        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->textureHandleKeys.size != 0) {
            RendererHandle handle = renderer->textureHandleKeys[0];

            // ~~Todo: replace with an actual warn
            LogWarn("[vk] Texture not freed on exit: HANDLE=%d", handle);

            renderer->FreeTexture(handle);
        }

        FreeDynArray(&renderer->textures);
        FreeDynArray(&renderer->textureHandleKeys);



        ////

        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->swapchainHandleKeys.size != 0) {
            RendererHandle handle = renderer->swapchainHandleKeys[0];

            // ~~Todo: replace with an actual warn
            LogWarn("[vk] Swapchain not freed on exit: HANDLE=%d", handle);

            renderer->FreeSwapchain(handle);
        }

        FreeDynArray(&renderer->swapchains);
        FreeDynArray(&renderer->swapchainHandleKeys);



        ////

        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->shaderHandleKeys.size != 0) {
            RendererHandle handle = renderer->shaderHandleKeys[0];

            LogWarn("[vk] Shader not freed on exit: HANDLE=%d", handle);

            renderer->FreeShader(handle);
        }

        FreeDynArray(&renderer->shaders);
        FreeDynArray(&renderer->shaderHandleKeys);

		
        // ~Optimize:
        // this involves an extra linear search when we already know the buffer index
        // because we're looping through all of them already,
        // maybe fix this if it becomes a problem later on
        while (renderer->samplerHandleKeys.size != 0) {
            RendererHandle handle = renderer->samplerHandleKeys[0];

            LogWarn("[vk] Sampler not freed on exit: HANDLE=%d", handle);
            
			renderer->FreeSampler(handle);
        }

        FreeDynArray(&renderer->samplers);
        FreeDynArray(&renderer->samplerHandleKeys);
    }

    vkDestroyDescriptorSetLayout(renderer->device, renderer->descriptorSetLayout, NULL);
    vkDestroyDescriptorPool(renderer->device, renderer->descriptorPool, NULL);
    FreeArray(&renderer->descriptorSets);

    vkDestroyCommandPool(renderer->device, renderer->commandPool, NULL);

    vkDestroyFence(renderer->device, renderer->renderFence, NULL);
    vkDestroySemaphore(renderer->device, renderer->presentSemaphore, NULL);
    vkDestroySemaphore(renderer->device, renderer->renderSemaphore, NULL);

    vkDestroyDevice(renderer->device, NULL);

    vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
    vkDestroyInstance(renderer->instance, NULL);

    AceFree(renderer, renderer->allocator);

    Log("[vk] Despawned Vulkan renderer");
}


RenderInfo VulkanRenderer::BeginRender(RendererHandle swapchain) {
    DOVK(vkWaitForFences(device, 1, &renderFence, true, 1000000000), "Failed to wait for fence");
    DOVK(vkResetFences(device, 1, &renderFence), "Failed to reset fence");


    // reset the descriptor set allocator
    this->allocatedDescriptorSets = 0;


    RenderInfo renderInfo {};
    renderInfo.swapchainValid = false;

    auto swapchainPtr = cast(VulkanSwapchain*, LookupSwapchain(swapchain));
    {
        u32 imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(device, swapchainPtr->swapchain, 1000000000, presentSemaphore, NULL, &imageIndex);

        // the alignment on the = sign was unintended and it's beautiful
        swapchainPtr->activeImageIndex = imageIndex;
        renderInfo.swapchainImageIndex = imageIndex;

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            // ~Todo swapchain recreation

            Log("[vk] ... is lazy #1. If you're seeing this, message ... on discord and say \"you're lazy\"");
            Assert(false);

            renderInfo.swapchainValid = false;
        }
        else {
            DOVK(result, "Failed to acquire swapchain image");
        }
    }

    DOVK(vkResetCommandBuffer(mainCommandList, 0), "Failed to reset command list");

    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext = NULL;

    cmdBeginInfo.pInheritanceInfo = NULL;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    DOVK(vkBeginCommandBuffer(mainCommandList, &cmdBeginInfo), "Failed to begin command list submission");

    return renderInfo;
}

void VulkanRenderer::EndRender(RendererHandle swapchain) {
    // ~Todo: call this from a different thread!

    DOVK(vkEndCommandBuffer(mainCommandList), "Failed to end command list submission");


    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = NULL;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &presentSemaphore;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderSemaphore;

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &mainCommandList;
    DOVK(vkQueueSubmit(graphicsQueue, 1, &submit, renderFence), "Failed to submit render!");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;

    auto swapchainPtr = cast(VulkanSwapchain*, LookupSwapchain(swapchain));

    presentInfo.pSwapchains = &swapchainPtr->swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainPtr->activeImageIndex;

    {
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            // device might not be idle here
            vkDeviceWaitIdle(device);

            // ~Todo swapchain recreation
            Assert(false);
        }
        else {
            DOVK(result, "Failed to submit present!");
        }
    }

    // vkQueueWaitIdle(presentQueue);

    vkDeviceWaitIdle(device);

    // ~Note: figure out why this only doesnt die when both vkQueueWaitIdle and fences are present
}


void VulkanRenderer::CmdBeginPass(RendererHandle pass, RendererHandle renderTarget, Rect renderArea, Array<Vec4> clearColors, float clearDepth)
{
    // ~Todo: call this from a different thread!
    this->activePass = pass;

    // ~Refactor do a bounds check to make sure there are enough clear colors!
    auto clearValues = MakeDynArray<VkClearValue>(0, Frame_Arena);
	For (clearColors)
		ArrayAdd(&clearValues, { .color = { {it->x, it->y, it->z, it->w} } });
	ArrayAdd(&clearValues, { .depthStencil = { .depth = clearDepth } });


    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext = NULL;

    VulkanCacheFramebuffer framebufferCache = ConjureSuitableFramebuffer(this, pass, renderTarget);

    rpInfo.renderPass = cast(VulkanPass*, LookupPass(pass))->renderPass;
    rpInfo.renderArea.offset.x = renderArea.x;
    rpInfo.renderArea.offset.y = renderArea.y;
    rpInfo.renderArea.extent = VkExtent2D { renderArea.width, renderArea.height };
    rpInfo.framebuffer = framebufferCache.framebuffer;
    rpInfo.clearValueCount = clearValues.size;
    rpInfo.pClearValues = clearValues.data;

    vkCmdBeginRenderPass(mainCommandList, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);



    VkViewport vkViewport;
    vkViewport.x = renderArea.x;
    vkViewport.y = renderArea.height - renderArea.y;
    vkViewport.width = renderArea.width;
    vkViewport.height = -(s32)renderArea.height;
    vkViewport.minDepth = 0;
    vkViewport.maxDepth = 1;
    vkCmdSetViewport(mainCommandList, 0, 1, &vkViewport);

    VkRect2D scissor;
    scissor.offset.x = renderArea.x;
    scissor.offset.y = renderArea.y;
    scissor.extent.width = renderArea.width;
    scissor.extent.height = renderArea.height;
    vkCmdSetScissor(mainCommandList, 0, 1, &scissor);


    // ~Todo consider whether not to do this?
    // not doing this would mean that changing render passes does not update the active shader unless SetShader was called
    UpdatePipeline(this);
}

void VulkanRenderer::CmdEndPass()
{
	// ~Todo: call this from a different thread!
	this->activePass = 0;

	vkCmdEndRenderPass(mainCommandList);
}


void VulkanRenderer::CmdSetShader(RendererHandle handle)
{
    // ~Todo: call this from a different thread!
    Assert(handle != 0);
    this->activeShader = handle;

    UpdatePipeline(this);
}

void VulkanRenderer::CmdBindVertexBuffers(u32 firstBinding, Array<RendererHandle>* buffers, Array<Size>* offsets)
{
    // ~Todo: call this from a different thread!

    Assert((offsets == NULL) || (buffers->size == offsets->size));

    auto vulkanBuffers = MakeArray<VkBuffer>(buffers->size, Frame_Arena);
    defer(FreeArray(&vulkanBuffers));

    ForIdx (*buffers, i)
	{
        auto* ourBuffer = (VulkanBuffer*)LookupBuffer((*buffers)[i]);
        Assert(ourBuffer != NULL);
        vulkanBuffers[i] = ourBuffer->buffer;
    }

    SArray<Size> zeroOffsets;

	if (!offsets)
	{
		zeroOffsets = MakeArray<Size>(buffers->size, Frame_Arena);
		For (zeroOffsets) 
			*it = 0;
	}

    vkCmdBindVertexBuffers(mainCommandList, firstBinding, buffers->size, vulkanBuffers.data, (offsets ? offsets->data : zeroOffsets.data));
    
	if (!offsets) FreeArray(&zeroOffsets);
}

#include "telemetry.h"

void VulkanRenderer::CmdDrawIndexed(u32 indexCount, u32 firstIndex, u32 vertexOffset, u32 instanceCount, u32 firstInstance)
{
    Assert(indexCount > 0);
    // ~Todo: call this from a different thread!
    // ~Todo indexed draw
	
	// ~Hack: assumes triangles
	telemetry.gfx_polygonCount += indexCount / 3;

    vkCmdDraw(mainCommandList, indexCount, instanceCount, vertexOffset, firstInstance);
}








intern void UpdateDescriptorSets(VulkanRenderer* renderer, RendererHandle shaderHandle, Array<VulkanDescriptorSlot>* descriptorSlots)
{	
    // Allocate a descriptor set or something
    auto targetSet = renderer->descriptorSets[renderer->allocatedDescriptorSets];
    Assert(renderer->allocatedDescriptorSets < renderer->descriptorSets.size);
    renderer->allocatedDescriptorSets++;

    auto bufferInfoStack = MakeDynArray<VkDescriptorBufferInfo>(0, Frame_Arena);
	defer(FreeDynArray(&bufferInfoStack));
    auto imageInfoStack = MakeDynArray<VkDescriptorImageInfo>(0, Frame_Arena);
	defer(FreeDynArray(&imageInfoStack));


    auto descriptorWrites = MakeDynArray<VkWriteDescriptorSet>(0, Frame_Arena);
	defer(FreeDynArray(&descriptorWrites));
	
    ForIdx (*descriptorSlots, idx) {
		auto it = &(*descriptorSlots)[idx];
		
		if (it->type == VulkanDescriptorSlot_None) continue;


		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = targetSet;
		write.dstBinding = idx;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;

		write.pBufferInfo 		= NULL;
        write.pImageInfo 		= NULL;
        write.pTexelBufferView 	= NULL;

		switch (it->type) {
			case VulkanDescriptorSlot_Uniform: {
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				auto* buffer = (VulkanBuffer*)renderer->LookupBuffer(it->uniform.bufferHandle);

				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = buffer->buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = buffer->size;

		        write.pBufferInfo = ArrayAdd(&bufferInfoStack, bufferInfo);
				break;
			}
			case VulkanDescriptorSlot_Sampler: {
				write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

				auto* texture 	= (VulkanTexture*)renderer->LookupTexture(it->sampler.textureHandle);
				auto* sampler 	= (VulkanSampler*)renderer->LookupSampler(it->sampler.samplerHandle);

				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageLayout 	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView 	= texture->textureView;
				imageInfo.sampler 		= sampler->sampler;

				write.pImageInfo = ArrayAdd(&imageInfoStack, imageInfo);
				break;
			}

			default: Assert(false); break;
		}

        ArrayAdd(&descriptorWrites, write);
    }

    vkUpdateDescriptorSets(renderer->device, descriptorWrites.size, descriptorWrites.data, 0, NULL);

    // lookup the pipeline associated with the shader


	// I think this should be fine because it only cares about descriptor set layouts,
	// but all the pipelines have the same one
	// what we do need to worry about is when you change shaders it doesnt keep prior ones

    VkPipelineLayout pipelineLayout;

    bool foundPipeline = false;
    ForIdx (renderer->cachePipelines, idx) {
        auto* it = &renderer->cachePipelines[idx];

        if (it->linkedShader == shaderHandle) {
            pipelineLayout = it->layout;
            foundPipeline = true;
        }
    }
    Assert(foundPipeline);

    vkCmdBindDescriptorSets(renderer->mainCommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &targetSet, 0, NULL);
}

intern void MakeSureDescriptorSlotsDontContainSomethingThatDied(VulkanRenderer* renderer)
{
	For (renderer->descriptorSlots)
	{
		if (it->type == VulkanDescriptorSlot_Uniform)
		{
			if (ArrayFind(&renderer->bufferHandleKeys, it->uniform.bufferHandle) == -1)
				it->type = VulkanDescriptorSlot_None;
		}
	}
}

void VulkanRenderer::CmdSetConstantBuffers(RendererHandle shaderHandle, u32 startSlot, Array<RendererHandle>* constantBuffers)
{
	// ~Todo: call this from a different thread!

	// ~FixMe   currently broken
	// this function override previous constant buffer bindings

	#if defined(BUILD_DEBUG)
	For (*constantBuffers)
	{
		Assert(( this->LookupBuffer(*it)->flags & RENDERBUFFER_FLAGS_CONSTANT) != 0 );
	}
	#endif

	for (u32 i = 0; i < constantBuffers->size; i++)
	{
		u32 idx = startSlot + i;
		Assert(idx < this->descriptorSlots.size);

		VulkanDescriptorSlot descriptorSlot = {};
		descriptorSlot.type = VulkanDescriptorSlot_Uniform;
		descriptorSlot.uniform.bufferHandle = (*constantBuffers)[i];

		this->descriptorSlots[idx] = descriptorSlot;
	}

	MakeSureDescriptorSlotsDontContainSomethingThatDied(this);
	UpdateDescriptorSets(this, shaderHandle, &this->descriptorSlots);
}

void VulkanRenderer::CmdSetSamplers(RendererHandle shaderHandle, u32 startSlot, Array<TextureSampler>* textureSamplers)
{
	ForIdx (*textureSamplers, idx)
	{
		auto textureSampler = (*textureSamplers)[idx];
		
		u32 slot = startSlot + idx;
		Assert(slot < this->descriptorSlots.size);

		VulkanDescriptorSlot descriptorSlot = {};
		descriptorSlot.type = VulkanDescriptorSlot_Sampler;
		descriptorSlot.sampler = textureSampler;

		this->descriptorSlots[slot] = descriptorSlot;
	}
	
	MakeSureDescriptorSlotsDontContainSomethingThatDied(this);
	UpdateDescriptorSets(this, shaderHandle, &this->descriptorSlots);
}








RendererHandle VulkanRenderer::CreateBuffer(void* data, Size size, u32 flags)
{
	VulkanBuffer buffer;
    buffer.flags = flags;
    buffer.size = size;

	// Make sure we have a buffer usage
	Assert(flags & (RENDERBUFFER_FLAGS_USAGE_STATIC | RENDERBUFFER_FLAGS_USAGE_DYNAMIC | RENDERBUFFER_FLAGS_USAGE_STREAM));

    VkBufferUsageFlags usageFlags = 0;
    if (flags & RENDERBUFFER_FLAGS_VERTEX) 			usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (flags & RENDERBUFFER_FLAGS_INDEX) 			usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (flags & RENDERBUFFER_FLAGS_CONSTANT) 		usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	if (flags & RENDERBUFFER_FLAGS_USAGE_STATIC) 	usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC) 	usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;





    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer.size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = usageFlags;
    DOVK(vkCreateBuffer(device, &bufferInfo, NULL, &buffer.buffer), "Failed to create buffer");


	// Buffer usage breakdown
	//	Static:
	//		- 1 Buffer on DeviceLocal Memory
	//		- 1 Temporary Allocated Staging buffer on HostMappable non DeviceLocal memory
	//  Dynamic:
	//		- 1 Buffer on GPU Dedicated Memory
	//		- 1 Persistant Staging buffer on HostMappable non DeviceLocal memory
	//  Stream:
	//		- (First) Attempt to allocate HostMappableDeviceLocal memory
	//		- If none exists, then just allocate HostMappable memory



	// query memory types
	// ~Optimization: Cache this?
	
	
	


	VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, buffer.buffer, &memReq);

	auto memoryTypesPresent = GetMemoryTypesPresent(this->physicalDevice, &memReq);

	VulkanMemoryType memoryType;
	
	if (flags & RENDERBUFFER_FLAGS_USAGE_STATIC)
	{
		Assert(memoryTypesPresent[VulkanMemory_DeviceLocal]);
		memoryType = VulkanMemory_DeviceLocal;
	}
	else if (flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC)
	{
		Assert(memoryTypesPresent[VulkanMemory_DeviceLocal]);
		memoryType = VulkanMemory_DeviceLocal;
		
	}
	else if (flags & RENDERBUFFER_FLAGS_USAGE_STREAM) 
	{
		if (memoryTypesPresent[VulkanMemory_DeviceLocalHostMappable])
			memoryType = VulkanMemory_DeviceLocalHostMappable;
		else
		{
			memoryType = VulkanMemory_HostMappable;
			Assert(memoryTypesPresent[VulkanMemory_HostMappable]);
		}
	}
	else
	{
		Assert(false); // No buffer usage
	}

	buffer.memory = AllocateGpuMemory(this->device, this->physicalDevice, memReq, memoryType);
	vkBindBufferMemory(device, buffer.buffer, buffer.memory, 0);

	bool hasStagingBuffer = (flags & RENDERBUFFER_FLAGS_USAGE_STATIC) || (flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC);

	if (hasStagingBuffer)
	{
		Assert(memoryTypesPresent[VulkanMemory_HostMappable]); // ~Fix Investigate the sketchy stuff that was going on here (it was true but the value was 204 and it wasn't == true?)

		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.size = buffer.size;
		stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		DOVK(vkCreateBuffer(device, &stagingBufferInfo, NULL, &buffer.stagingBuffer), "Failed to create staging buffer");

		VkMemoryRequirements stagingMemReq;
		vkGetBufferMemoryRequirements(device, buffer.stagingBuffer, &stagingMemReq);

		buffer.stagingMemory = AllocateGpuMemory(this->device, this->physicalDevice, stagingMemReq, VulkanMemory_HostMappable); 
		vkBindBufferMemory(device, buffer.stagingBuffer, buffer.stagingMemory, 0);
	}


	// Make sure that our data isn't null if we wanna copy memory over
	if (data) 
	{
		// ~Optimize 1:1 allocation to buffer memory is really bad lol, we should do something better VERY soon
		VkDeviceMemory memory = hasStagingBuffer ? buffer.stagingMemory : buffer.memory;

		void* destData;
		vkMapMemory(device, memory, 0, bufferInfo.size, 0, &destData);
		memcpy(destData, data, size);
		vkUnmapMemory(device, memory);
    
		// Do a transfer operation
		if (hasStagingBuffer) 
		{
			auto cmd = BeginImmediateSubmit(this);
			
			VkBufferCopy copy;
			copy.dstOffset = 0;
			copy.srcOffset = 0;
			copy.size = buffer.size;
			vkCmdCopyBuffer(cmd, buffer.stagingBuffer, buffer.buffer, 1, &copy);
			EndImmediateSubmit(this, this->graphicsQueue, cmd);
		}
	}


	if ((flags & RENDERBUFFER_FLAGS_USAGE_STATIC))
	{
		vkDestroyBuffer(this->device, buffer.stagingBuffer, NULL);
		vkFreeMemory(this->device, buffer.stagingMemory, NULL);
	}
	

	// manager stuffs
	RendererHandle bufferHandle = this->nextHandle++;
	ArrayAdd(&bufferHandleKeys, bufferHandle);
	ArrayAdd(&buffers, buffer);

	return bufferHandle;
}


void VulkanRenderer::FreeBuffer(RendererHandle handle)
{
	Size idx = ArrayFind(&this->bufferHandleKeys, handle);
    Assert(idx != -1); // failed

    if (idx != -1) 
	{
        VulkanBuffer* buffer = &this->buffers[idx];
        vkDestroyBuffer(device, buffer->buffer, NULL);
        vkFreeMemory(device, buffer->memory, NULL);

		if (buffer->flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC)
		{
			vkDestroyBuffer(this->device, buffer->stagingBuffer, NULL);
			vkFreeMemory(this->device, buffer->stagingMemory, NULL);
		}
		
        ArrayRemoveAt(&this->bufferHandleKeys, idx);
        ArrayRemoveAt(&this->buffers, idx);
    }
}


Buffer* VulkanRenderer::LookupBuffer(RendererHandle handle) {
    for (size_t idx = 0; idx < bufferHandleKeys.size; idx++) {
        if (bufferHandleKeys[idx] == handle) {
            return &buffers[idx];
        }
    }
    LogWarn("[vk] Failed to lookup buffer handle %d", handle);
    return NULL;
}


intern VkAttachmentLoadOp GetVulkanLoadOpFromAttachOp(RenderAttachOp op) {
    switch (op) {
        case RENDERATTACHOP_DONT_CARE: 	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        case RENDERATTACHOP_LOAD: 		return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RENDERATTACHOP_CLEAR: 		return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }

    Assert(false);
    return (VkAttachmentLoadOp)0;
}


intern VkAttachmentStoreOp GetVulkanStoreOpFromAttachOp(RenderAttachOp op) {
    switch (op) {
        case RENDERATTACHOP_DONT_CARE: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        case RENDERATTACHOP_STORE: return VK_ATTACHMENT_STORE_OP_STORE;
    }

    Assert(false);
    return (VkAttachmentStoreOp)0;
}


intern VkSampleCountFlagBits GetVulkanSampleFlagFromSampleCount(u8 sampleCount) {
	switch (sampleCount) {
		case 1:  return VK_SAMPLE_COUNT_1_BIT;
		case 2:  return VK_SAMPLE_COUNT_2_BIT;
		case 4:  return VK_SAMPLE_COUNT_4_BIT;
		case 8:  return VK_SAMPLE_COUNT_8_BIT;
		case 16: return VK_SAMPLE_COUNT_16_BIT;
		case 32: return VK_SAMPLE_COUNT_32_BIT;
		case 64: return VK_SAMPLE_COUNT_64_BIT;
	}
	Assert(false);
	return (VkSampleCountFlagBits)0;
}

RendererHandle VulkanRenderer::CreatePass(const RenderPassDescription* desc) {
	VulkanPass pass;

	Size attachmentCount = desc->colorAttachments.size + (desc->hasDepthAttachment ? 1 : 0);
	auto attachments = MakeArray<VkAttachmentDescription>(attachmentCount, Frame_Arena);
	defer(FreeArray(&attachments));
	auto colorAttachmentRefs = MakeArray<VkAttachmentReference>(desc->colorAttachments.size, Frame_Arena);
	defer(FreeArray(&colorAttachmentRefs));

	Size nextAttachment = 0;

    For (desc->colorAttachments) {
		// auto* it = &desc->colorAttachments[idx]; // ... why do i need .data here? C++ why are u weird // nvm fixed it because const is a thing in the operator[] now

        VkAttachmentDescription colorAttachment = {};


        colorAttachment.format = GetVulkanFormatFromRenderFormat(it->format);
        colorAttachment.samples = GetVulkanSampleFlagFromSampleCount(it->sampleCount);
        colorAttachment.loadOp = GetVulkanLoadOpFromAttachOp(it->loadOp);
        colorAttachment.storeOp = GetVulkanStoreOpFromAttachOp(it->storeOp);
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = GetVulkanImageLayoutFromTextureLayout(it->inLayout);
        colorAttachment.finalLayout = GetVulkanImageLayoutFromTextureLayout(it->outLayout);

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = nextAttachment;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments[nextAttachment] = colorAttachment;
        colorAttachmentRefs[nextAttachment] = colorAttachmentRef;

		nextAttachment++;
    }


    VkAttachmentReference depthAttachmentRefFinal;
	if (desc->hasDepthAttachment) {
		auto it = &desc->depthAttachment;
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = GetVulkanFormatFromRenderFormat(it->format);
        depthAttachment.samples = GetVulkanSampleFlagFromSampleCount(it->sampleCount);
        depthAttachment.loadOp = GetVulkanLoadOpFromAttachOp(it->loadOp);
        depthAttachment.storeOp = GetVulkanStoreOpFromAttachOp(it->storeOp);
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = GetVulkanImageLayoutFromTextureLayout(it->inLayout);
		depthAttachment.finalLayout = GetVulkanImageLayoutFromTextureLayout(it->outLayout);

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = nextAttachment;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[nextAttachment] = depthAttachment;
        depthAttachmentRefFinal = depthAttachmentRef;

		nextAttachment++;
	}


    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentRefs.size;
    subpass.pColorAttachments = colorAttachmentRefs.data;
	subpass.pDepthStencilAttachment = desc->hasDepthAttachment ? &depthAttachmentRefFinal : NULL;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachments.size;
    renderPassInfo.pAttachments = attachments.data;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    DOVK(vkCreateRenderPass(device, &renderPassInfo, NULL, &pass.renderPass), "Failed to create render pass");


	pass.hasDepthAttachment = desc->hasDepthAttachment;
	pass.colorAttachmentCount = colorAttachmentRefs.size;

    // Resource management stuff
    RendererHandle handle = nextHandle++;
    ArrayAdd(&passHandleKeys, handle);
    ArrayAdd(&passes, pass);

    return handle;
}

void VulkanRenderer::FreePass(RendererHandle handle) {
    for (size_t idx = 0; idx < passHandleKeys.size; idx++) {
        if (passHandleKeys[idx] == handle) {
            VulkanPass* pass = &passes[idx];
            vkDestroyRenderPass(device, pass->renderPass, NULL);

            ArrayRemoveAt(&passHandleKeys, idx);
            ArrayRemoveAt(&passes, idx);

            return; // so we dont hit the assert or loop more
        }
    }
    Assert(false); // failed
}

RenderPass* VulkanRenderer::LookupPass(RendererHandle handle) {
    for (size_t idx = 0; idx < passHandleKeys.size; idx++) {
        if (passHandleKeys[idx] == handle) {
            return &passes[idx];
        }
    }
    Assert(false); // failed
    return NULL;
}

RendererHandle VulkanRenderer::CreateRenderTarget(const RenderTargetDescription* desc) {
    VulkanRenderTarget renderTarget;
	bool hasDepthAttachment = desc->depthAttachment != RendererHandle_None;

    renderTarget.width = desc->width;
    renderTarget.height = desc->height;
	renderTarget.attachments = MakeArray<RendererHandle>(desc->colorAttachments->size + (hasDepthAttachment ? 1 : 0));
	Memcpy(renderTarget.attachments.data, desc->colorAttachments->data, sizeof(RendererHandle) * desc->colorAttachments->size); // ~Refactor ArrayCopy function
	if (hasDepthAttachment)
		renderTarget.attachments[renderTarget.attachments.size - 1] = desc->depthAttachment;

    // manager stuffs
    RendererHandle renderTargetHandle = nextHandle++;
    ArrayAdd(&renderTargetHandleKeys, renderTargetHandle);
    ArrayAdd(&renderTargets, renderTarget);

    return renderTargetHandle;
}

void VulkanRenderer::FreeRenderTarget(RendererHandle handle) {
    Size idx = ArrayFind(&renderTargetHandleKeys, handle);
    Assert(idx != -1);

    VulkanRenderTarget* renderTarget = &renderTargets[idx];

	FreeArray(&renderTarget->attachments);
    ArrayRemoveAt(&renderTargetHandleKeys, idx);
    ArrayRemoveAt(&renderTargets, idx);
}

RenderTarget* VulkanRenderer::LookupRenderTarget(RendererHandle handle) {
    Size idx = ArrayFind(&renderTargetHandleKeys, handle);
    Assert(idx != -1);

    return &renderTargets[idx];
}




// vkCreateImageView(
//     VkDevice                                    device,
//     const VkImageViewCreateInfo*                pCreateInfo,
//     const VkAllocationCallbacks*                pAllocator,
//     VkImageView*                                pView);




RendererHandle VulkanRenderer::CreateSwapchain(const SwapchainDescription* desc) {
    VulkanSwapchain swapchain;

	int width, height;
    {
        // ~Cleanup use static arrays; maybe not actually? doesnt really matter

        // query physical device info
        VkSurfaceCapabilitiesKHR swapChainCapabilities;
        auto swapChainFormats = MakeDynArray<VkSurfaceFormatKHR>(0, Frame_Arena);
        defer(FreeDynArray(&swapChainFormats));
        auto swapChainPresentModes = MakeDynArray<VkPresentModeKHR>(0, Frame_Arena);
        defer(FreeDynArray(&swapChainPresentModes));

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainCapabilities);


        u32 formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
        if (formatCount != 0)  {
            ArrayResize(&swapChainFormats, formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapChainFormats.data);
        }

        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
        if (presentModeCount != 0) {
            ArrayResize(&swapChainPresentModes, presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, swapChainPresentModes.data);
        }


        // choose present mode
        VkPresentModeKHR presentMode = GetVulkanPresentModeFromPresentMode(desc->presentMode);
        // ~Todo validate present mode is allowed in the swapchain

        VkSurfaceFormatKHR surfaceFormat;
        {
            bool found = false;
            ForIdx (swapChainFormats, i) {
                auto availFormat = swapChainFormats[i];
                if (availFormat.format == GetVulkanFormatFromRenderFormat(desc->format)
                    && availFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    surfaceFormat = availFormat;
                    found = true;
                    break;
                }
            }

            Assert(found);
        }

        VkExtent2D extent = swapChainCapabilities.currentExtent;


        // ~Todo: Fix before ship lol
        // probably when u move this to a different function, just take in the window client pixel size
        Assert(extent.width != UINT32_MAX);

		width = extent.width;
		height = extent.height;

        u32 imageCount;


        // ~Hack: this is wrong in certain cases (when minImageCount != 2)
        switch (desc->presentMode) {
            case PRESENTMODE_DOUBLE_BUFFER_IMMEDIATE:
            case PRESENTMODE_DOUBLE_BUFFER_FIFO: {
                imageCount = 2;
                break;
            }

            case PRESENTMODE_TRIPLE_BUFFER_FIFO:
            case PRESENTMODE_TRIPLE_BUFFER_MAILBOX: {
                imageCount = 2 + 1;
                break;
            }

            default: {
                Assert(false);
                break;
            }
        }

        // bounds check image count
        if (swapChainCapabilities.maxImageCount != 0
            && imageCount > swapChainCapabilities.maxImageCount) {
            imageCount = swapChainCapabilities.maxImageCount;
        }

        Log("[vk] Swapchain Image Count: %d => %d <= %d", swapChainCapabilities.minImageCount, imageCount, swapChainCapabilities.maxImageCount);

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // ~Todo: deal with resource ownership

        /*
        u32 queueIndices[] = {
            graphicsQueueFamily,
            presentQueueFamily,
        };
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndices;
        */

        createInfo.preTransform = swapChainCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        DOVK(vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain.swapchain), "Failed to create Vulkan swapchain!");
    }

    {
        u32 imageCount;
        vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, NULL);
        auto swapchainImages = MakeArray<VkImage>(imageCount, Frame_Arena);
        defer(FreeArray(&swapchainImages));
        vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, swapchainImages.data);

        // create swap chain textures
        swapchain.textures = MakeArray<RendererHandle>(imageCount);
        for (u32 imageIndex = 0; imageIndex < imageCount; imageIndex++) {
            VkImageViewCreateInfo createInfo { };
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[imageIndex];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = GetVulkanFormatFromRenderFormat(desc->format);

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkImageView imgView;
            DOVK(vkCreateImageView(device, &createInfo, NULL, &imgView), "Failed to create swapchain texture views.");

            // ~Refactor
			// this is like a partially filled VulkanTexture struct,
			// maybe we could stick a flag on here so that it returns an error if u use a 
			// function that requires stuff like the VkDeviceMemory of the texture
			VulkanTexture texture { };
			texture.flags = TextureFlags_ColorAttachment;
			texture.width = width;
			texture.height = height;
			texture.format = desc->format;
			texture.mipmaps = 1;

			texture.vkFlags = VulkanTextureFlag_OnlyOwnTextureView;
			texture.texture = swapchainImages[imageIndex];
            texture.textureView = imgView;

			RendererHandle handle = nextHandle++;
            ArrayAdd(&this->textureHandleKeys, handle);
            ArrayAdd(&this->textures, texture);

            swapchain.textures[imageIndex] = handle;
        }
    }

    RendererHandle handle = nextHandle++;
    ArrayAdd(&swapchainHandleKeys, handle);
    ArrayAdd(&swapchains, swapchain);

    return handle;
}

void VulkanRenderer::FreeSwapchain(RendererHandle handle) {
    Size idx = ArrayFind(&swapchainHandleKeys, handle);
    Assert(idx != -1);

    VulkanSwapchain* swapchain = &swapchains[idx];
    vkDestroySwapchainKHR(device, swapchain->swapchain, NULL);

    For (swapchain->textures) {
        FreeTexture(*it);
    }

    ArrayRemoveAt(&swapchainHandleKeys, idx);
    ArrayRemoveAt(&swapchains, idx);
}

Swapchain* VulkanRenderer::LookupSwapchain(RendererHandle handle) {
    Size idx = ArrayFind(&swapchainHandleKeys, handle);
    Assert(idx != -1);

    return &swapchains[idx];
}





RendererHandle VulkanRenderer::CreateShader(const ShaderDescription* desc) {
    VulkanShader shader;

    shader.stages = MakeArray<VulkanShaderStage>(desc->stages->size);
    ForIdx (*desc->stages, i) {
        auto it = &(*desc->stages)[i];

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = it->size;
        createInfo.pCode = (u32*)it->data;

        VkShaderModule shaderModule;
        DOVK(vkCreateShaderModule(device, &createInfo, NULL, &shaderModule), "Failed to create shader!");

        VulkanShaderStage shaderStage;
        shaderStage.type = it->type;
        shaderStage.shader = shaderModule;
        shader.stages[i] = shaderStage;
    }


    // ~Todo put this in vertex buffer
    shader.vertexInputLayouts = MakeArray<VertexInputLayout>(desc->vertexInputLayouts->size);
    memcpy(shader.vertexInputLayouts.data, desc->vertexInputLayouts->data, sizeof(VertexInputLayout) * desc->vertexInputLayouts->size);

    shader.primitiveType = desc->primitiveType;

    shader.fillMode = desc->fillMode;
    shader.frontFace = desc->frontFace;
    shader.lineWidth = desc->lineWidth;
    shader.cullMode = desc->cullMode;

    shader.blendEnable = desc->blendEnable;
    shader.blendSrcColor = desc->blendSrcColor;
    shader.blendDestColor = desc->blendDestColor;
    shader.blendOpColor = desc->blendOpColor;
    shader.blendSrcAlpha = desc->blendSrcAlpha;
    shader.blendDestAlpha = desc->blendDestAlpha;
    shader.blendOpAlpha = desc->blendOpAlpha;

	shader.depthTestEnable = desc->depthTestEnable;
	shader.depthWriteEnable = desc->depthWriteEnable;
	shader.depthCompareOp = desc->depthCompareOp;

    RendererHandle handle = nextHandle++;
    ArrayAdd(&shaderHandleKeys, handle);
    ArrayAdd(&shaders, shader);

    return handle;
}

void VulkanRenderer::FreeShader(RendererHandle handle) {
    Size idx = ArrayFind(&shaderHandleKeys, handle);
    Assert(idx != -1);

    auto* shader = &shaders[idx];
    // ~Todo this will be removed in refactor
    FreeArray(&shader->vertexInputLayouts);

    For (shader->stages) {
        vkDestroyShaderModule(this->device, it->shader, NULL);
    }
    FreeArray(&shader->stages);

    ArrayRemoveAt(&shaderHandleKeys, idx);
    ArrayRemoveAt(&shaders, idx);
}

Shader* VulkanRenderer::LookupShader(RendererHandle handle) {
    Size idx = ArrayFind(&shaderHandleKeys, handle);
    Assert(idx != -1);

    return &shaders[idx];
}



RendererHandle VulkanRenderer::CreateSampler(FilterMode magFilter, FilterMode minFilter, SamplerAddressMode addressModeU, SamplerAddressMode addressModeV, SamplerAddressMode addressModeW, SamplerMipmapMode mipmapMode)
{
	VulkanSampler sampler {};

	sampler.magFilter = magFilter;
	sampler.minFilter = minFilter;
	sampler.addressModeU = addressModeU;
	sampler.addressModeV = addressModeV;
	sampler.addressModeW = addressModeW;
	sampler.mipmapMode = mipmapMode;
	

	VkSamplerCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	
	// these values probably wont need to be changed so here are reasonable defaults,
	// if they do then just add them to the sampler description
	createInfo.unnormalizedCoordinates = VK_FALSE; 	// normalized => [0, 1) -- unnormalized => [0, texWidth or texHeight)
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.compareEnable = VK_FALSE; 			// no friciking clue what this does
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS; 	// or this

	// texture stuff
	createInfo.magFilter = GetVulkanFilterFromFilterMode(magFilter);
	createInfo.minFilter = GetVulkanFilterFromFilterMode(minFilter);
	createInfo.addressModeU = GetVulkanSamplerAddressModeFromSamplerAddressMode(addressModeU);
	createInfo.addressModeV = GetVulkanSamplerAddressModeFromSamplerAddressMode(addressModeV);
	createInfo.addressModeW = GetVulkanSamplerAddressModeFromSamplerAddressMode(addressModeW);


	// mipmapping ~Todo
	createInfo.mipmapMode = GetVulkanSamplerMipmapModeFromSamplerMipmapMode(mipmapMode);
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	// ansiotropy stuff ~Todo
	createInfo.anisotropyEnable = VK_FALSE;
	VkPhysicalDeviceProperties properties = {};
	vkGetPhysicalDeviceProperties(this->physicalDevice, &properties);
	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	// ~Todo error handling that isnt garbage
	DOVK(vkCreateSampler(this->device, &createInfo, NULL, &sampler.sampler), "Failed to create sampler!");

    RendererHandle handle = nextHandle++;
    ArrayAdd(&this->samplerHandleKeys, handle);
    ArrayAdd(&this->samplers, sampler);
	return handle;
}

void VulkanRenderer::FreeSampler(RendererHandle handle)
{
	Size idx = ArrayFind(&samplerHandleKeys, handle);
    Assert(idx != -1);

    auto* sampler = &samplers[idx];
    
	vkDestroySampler(this->device, sampler->sampler, NULL);

    ArrayRemoveAt(&this->samplerHandleKeys, idx);
    ArrayRemoveAt(&this->samplers, idx);
}

Sampler* VulkanRenderer::LookupSampler(RendererHandle handle)
{
	// ~Todo @ErrorHandle
    Size idx = ArrayFind(&samplerHandleKeys, handle);
    Assert(idx != -1);

    return &samplers[idx];
}





void* VulkanRenderer::MapBuffer(RendererHandle handle) {

    auto* buffer = (VulkanBuffer*)LookupBuffer(handle);
    Assert(buffer != NULL);

	Assert((buffer->flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC) || (buffer->flags & RENDERBUFFER_FLAGS_USAGE_STREAM));

	bool hasStagingBuffer = (buffer->flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC);

    void* mappedMemory;
    DOVK(vkMapMemory(this->device, hasStagingBuffer ? buffer->stagingMemory : buffer->memory, 0, VK_WHOLE_SIZE, 0, &mappedMemory), "F");



    return mappedMemory;
}

void VulkanRenderer::UnmapBuffer(RendererHandle handle) {
    auto* buffer = (VulkanBuffer*)LookupBuffer(handle);
    Assert(buffer != NULL);

	bool hasStagingBuffer = (buffer->flags & RENDERBUFFER_FLAGS_USAGE_DYNAMIC);

    vkUnmapMemory(this->device, hasStagingBuffer ? buffer->stagingMemory : buffer->memory);

	// Do a transfer operation
	// ~Incomplete this wont work if we want to do persistant buffer mapping in the future
	if (hasStagingBuffer) 
	{
		auto cmd = BeginImmediateSubmit(this);

		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = buffer->size;
		vkCmdCopyBuffer(cmd, buffer->stagingBuffer, buffer->buffer, 1, &copy);

		EndImmediateSubmit(this, this->graphicsQueue, cmd);
	}
}



void VulkanRenderer::UploadTextureData(RendererHandle handle, Size size, void* data) 
{
	auto* texture = (VulkanTexture*)LookupTexture(handle);
	Assert(texture != NULL);

	Assert(size <= RenderFormatBytesPerPixel(texture->format) * texture->width * texture->height);


	void* mappedMemory;
	DOVK(vkMapMemory(this->device, texture->stagingMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory), "Failed to map image memory!");

	Memcpy(mappedMemory, data, size);

	vkUnmapMemory(this->device, texture->stagingMemory);

	// ~Todo Textures will need to work for both reading and writing to the GPU and other things, so this should be changed
	
	// Copy the staging memory to the texture
	auto cmd = BeginImmediateSubmit(this);


	VkImageMemoryBarrier transferBarrier = {};
	transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	transferBarrier.srcAccessMask = 0;
	transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	transferBarrier.image = texture->texture;
	transferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	transferBarrier.subresourceRange.baseMipLevel = 0;
	transferBarrier.subresourceRange.levelCount = 1;
	transferBarrier.subresourceRange.baseArrayLayer = 0;
	transferBarrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &transferBarrier);




	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageExtent = { texture->width, texture->height, 1 };

	vkCmdCopyBufferToImage(cmd, texture->stagingBuffer, texture->texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);



	VkImageMemoryBarrier readableBarrier = {};
	readableBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	readableBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	readableBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	readableBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	readableBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	readableBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	readableBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	readableBarrier.image = texture->texture;
	readableBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	readableBarrier.subresourceRange.baseMipLevel = 0;
	readableBarrier.subresourceRange.levelCount = 1;
	readableBarrier.subresourceRange.baseArrayLayer = 0;
	readableBarrier.subresourceRange.layerCount = 1;


	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &readableBarrier);


	EndImmediateSubmit(this, this->graphicsQueue, cmd);
}






RendererHandle VulkanRenderer::CreateTexture(u32 width, u32 height, RenderFormat format, u32 mipmaps, u64 flags) {
	VulkanTexture texture { };
	texture.vkFlags = VulkanTextureFlag_None;
	texture.width = width;
	texture.height = height;
	texture.format = format;
	texture.mipmaps = mipmaps;
	texture.flags = flags;

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.extent = VkExtent3D { width, height, 1 };
	createInfo.imageType = VK_IMAGE_TYPE_2D; // vc: If we need 1- and 3-dimensional textures then we might want to modify the Texture struct rather than create new texture types
	createInfo.mipLevels = mipmaps;
	createInfo.format = GetVulkanFormatFromRenderFormat(format);
	createInfo.arrayLayers = 1;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL; // vc: we can also specific linear tiling but that isnt really necessary rn
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT; // vc: texture description
	createInfo.flags = 0;

	// ~? may be a bad idea // ~Refactor make this a flag
	createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (flags & TextureFlags_Sampled) 					createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (flags & TextureFlags_DepthStencilAttachment) 	createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (flags & TextureFlags_ColorAttachment) 			createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	DOVK(vkCreateImage(this->device, &createInfo, NULL, &texture.texture), "Failed to create texture!");


	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(this->device, texture.texture, &memReq);


	texture.memory = AllocateGpuMemory(this->device, this->physicalDevice, memReq, VulkanMemory_DeviceLocal);
	vkBindImageMemory(this->device, texture.texture, texture.memory, 0);



	{ // staging buffer
		size_t pixelDataSize = RenderFormatBytesPerPixel(format) * width * height;

		VkBufferCreateInfo stagingBufferInfo = {};
		stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferInfo.size = pixelDataSize;
		stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		DOVK(vkCreateBuffer(device, &stagingBufferInfo, NULL, &texture.stagingBuffer), "Failed to create texture staging buffer");
	
		VkMemoryRequirements stagingMemReq;
		vkGetBufferMemoryRequirements(device, texture.stagingBuffer, &stagingMemReq);

		texture.stagingMemory = AllocateGpuMemory(this->device, this->physicalDevice, stagingMemReq, VulkanMemory_HostMappable); 

		vkBindBufferMemory(device, texture.stagingBuffer, texture.stagingMemory, 0);
	}


	// texture view
	u32 aspectMask;
	if (FormatHasDepth(texture.format))
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	else
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	// ~Todo mipmapping
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = texture.texture;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = GetVulkanFormatFromRenderFormat(texture.format);
    viewCreateInfo.subresourceRange.aspectMask = aspectMask;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
	DOVK(vkCreateImageView(this->device, &viewCreateInfo, NULL, &texture.textureView), "Failed to create texture view!");


	// manager stuffs
    RendererHandle textureHandle = this->nextHandle++;
    ArrayAdd(&textureHandleKeys, textureHandle);
    ArrayAdd(&textures, texture);

    return textureHandle;
}

void VulkanRenderer::FreeTexture(RendererHandle handle) {
	Size idx = ArrayFind(&textureHandleKeys, handle);
    Assert(idx != -1);

	auto* texture = &textures[idx];

	vkDestroyImageView(this->device, texture->textureView, NULL);

	if (!(texture->vkFlags & VulkanTextureFlag_OnlyOwnTextureView))
	{
		vkDestroyImage(this->device, texture->texture, NULL);
		vkFreeMemory(this->device, texture->memory, NULL);
	
		vkDestroyBuffer(this->device, texture->stagingBuffer, NULL);
		vkFreeMemory(this->device, texture->stagingMemory, NULL);
	}

    ArrayRemoveAt(&textureHandleKeys, idx);
    ArrayRemoveAt(&textures, idx);
}

Texture* VulkanRenderer::LookupTexture(RendererHandle handle) {
	Size idx = ArrayFind(&textureHandleKeys, handle);
    Assert(idx != -1);

    return &textures[idx];
} 

void VulkanRenderer::CmdUpdateBuffer(RendererHandle buffer, Size start, Size size, void* data) {
	// ~Todo ~Hack
	// Havent seen anything yet but this function may require TRANSFER_DST to be true according to vulkan spec (even though it worked without that?)
	// im gonna do some testing to see why and stuff
	vkCmdUpdateBuffer(this->mainCommandList, ((VulkanBuffer*)this->LookupBuffer(buffer))->buffer, start, size, data);
}

void VulkanRenderer::CmdPushConstants(RendererHandle shader, Size start, Size size, void* data) {
	// ~Cleanup move finding pipelines into a function cause CmdSetConstantBuffers also uses this
	VkPipelineLayout pipelineLayout;

    bool foundPipeline = false;
    ForIdx (this->cachePipelines, idx) {
        auto* it = &this->cachePipelines[idx];

        if (it->linkedShader == shader) {
            pipelineLayout = it->layout;
            foundPipeline = true;
        }
    }
    Assert(foundPipeline);
    
	vkCmdPushConstants(this->mainCommandList, pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, start, size, data);
}


void VulkanRenderer::CmdPipelineBarrier(PipelineBarrier barrier)
{
	auto textureBarriers = MakeArray<VkImageMemoryBarrier>(barrier.textureBarriers.size, Frame_Arena);
	ForIdx (barrier.textureBarriers, idx)
	{
		auto texBarrier = barrier.textureBarriers[idx];
		
		Texture* texture = this->LookupTexture(texBarrier.texture);

		bool hasDepth = FormatHasDepth(texture->format);
		// ~Refactor (...) query this from our saved texture data
		VkImageAspectFlags aspectMask = 0;
		if (hasDepth) aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		else aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		textureBarriers[idx] = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			
			.oldLayout = GetVulkanImageLayoutFromTextureLayout(texBarrier.inLayout),
			.newLayout = GetVulkanImageLayoutFromTextureLayout(texBarrier.outLayout),

			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

	
			.image = ((VulkanTexture*)texture)->texture, // ~Todo null check
			.subresourceRange = {
				.aspectMask = aspectMask,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
	}

	// ~Optimise ABSOLUTELY not optimised at all lol
	vkCmdPipelineBarrier(
		this->mainCommandList, 
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0, 
		0, NULL, 
		0, NULL, 
		textureBarriers.size, textureBarriers.data);
}



#endif
