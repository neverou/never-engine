#pragma once


#include "renderer.h"

// i have no clue if this compiles or not cause RENDERER_IMPL_OPENGL is false at time of writing 
#if defined(RENDERER_IMPL_OPENGL) && (defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX))

struct OpenGLPlatformLayer {
	virtual void SwapBuffers() = 0;
	virtual void MakeCurrent() = 0;
	virtual void ClearCurrent() = 0;
	virtual void Destroy() = 0;
};




struct SDL_Window;
typedef void* SDL_GLContext;
struct OpenGLLayerSDL : public OpenGLPlatformLayer {
	SDL_Window* window;
	SDL_GLContext context;

	void SwapBuffers() override;
	void MakeCurrent() override;
	void ClearCurrent() override;
	void Destroy() override;

	
};

OpenGLLayerSDL* SpawnOpenGLLayerSDL(SDL_Window* window);



struct OpenGLBuffer : public Buffer {
	u32 glHandle;
};

struct OpenGLShader : public Shader {
	u32 glHandle;
};

struct OpenGLRenderer : public Renderer {
	OpenGLPlatformLayer* platformLayer;
	RendererHandle nextHandle;

	// Resources
	DynArray<RendererHandle> bufferHandleKeys;
	DynArray<OpenGLBuffer> buffers;
	DynArray<RendererHandle> shaderHandleKeys;
	DynArray<OpenGLShader> shaders;
	//

	// Resource Management //
    RendererHandle CreateBuffer(const BufferDescription* desc) override;
    void FreeBuffer(RendererHandle handle) override;
    Buffer* LookupBuffer(RendererHandle handle) override;

    RendererHandle CreatePass(const RenderPassDescription* desc) override;
    void FreePass(RendererHandle handle) override;
    RenderPass* LookupPass(RendererHandle handle) override;

	RendererHandle CreateFramebuffer(const FramebufferDescription* desc) override;
    void FreeFramebuffer(RendererHandle handle) override;
    Framebuffer* LookupFramebuffer(RendererHandle handle) override;
    
	RendererHandle CreateTextureView(const TextureViewDescription* desc) override;
    void FreeTextureView(RendererHandle handle) override;
    TextureView* LookupTextureView(RendererHandle handle) override;
    
	RendererHandle CreateTexture(const TextureDescription* desc) override;
    void FreeTexture(RendererHandle handle) override;
    Texture* LookupTexture(RendererHandle handle) override;

	RendererHandle CreateSwapchain(const SwapchainDescription* desc) override;
    void FreeSwapchain(RendererHandle handle) override;
    Swapchain* LookupSwapchain(RendererHandle handle) override;
    
	RendererHandle CreateShader(const ShaderDescription* desc) override;
    void FreeShader(RendererHandle handle) override;
    Shader* LookupShader(RendererHandle handle) override;
    
	void* MapBuffer(RendererHandle handle) override;
    void UnmapBuffer(RendererHandle handle) override; 
    //

    // Commands //
    RenderInfo BeginRender(RendererHandle swapchain) override;
    void EndRender(RendererHandle swapchain) override;
    
    void CmdBeginPass(RendererHandle pass, RendererHandle renderTarget, Rect renderArea) override;
    void CmdEndPass() override;
    
    void CmdSetShader(RendererHandle handle) override;

    void CmdBindVertexBuffers(u32 firstBinding, Array<RendererHandle>* buffers, Array<Size>* offsets) override;
    void CmdDrawIndexed(u32 indexCount, u32 firstIndex, u32 vertexOffset, u32 instanceCount, u32 firstInstance) override;
    
    void CmdSetConstantBuffers(RendererHandle shaderHandle, u32 startSlot, Array<RendererHandle>* constantBuffers) override;

	void CmdUpdateBuffer(RendererHandle buffer, Size start, Size size, void* data) override;
	void CmdPushConstants(RendererHandle shader, Size start, Size size, void* data) override;
    //
};


OpenGLRenderer* SpawnOpenGLRenderer(OpenGLPlatformLayer* platformLayer, RendererSpawnInfo* spawnInfo);
void DeleteOpenGLRenderer(OpenGLRenderer* renderer);

#endif