#include "opengl_renderer.h"


#if defined(RENDERER_IMPL_OPENGL)
#include "std.h"
#include "glad/glad/glad.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) // Platforms that use SDL
#include <sdl/SDL.h>

void OpenGLLayerSDL::SwapBuffers()
{
	SDL_GL_SwapWindow(window);
}

void OpenGLLayerSDL::MakeCurrent()
{
	SDL_GL_MakeCurrent(window, context);
}

void OpenGLLayerSDL::ClearCurrent() {
	SDL_GL_MakeCurrent(window, NULL);
}

void OpenGLLayerSDL::Destroy() {
	SDL_GL_DeleteContext(context);
}

OpenGLLayerSDL* SpawnOpenGLLayerSDL(SDL_Window* window) {
	OpenGLLayerSDL* layer = new OpenGLLayerSDL;
    
	layer->window = window;
	layer->context = SDL_GL_CreateContext(window);
    
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		return NULL;
	}
    
	
	return layer;
}

#endif



OpenGLRenderer* SpawnOpenGLRenderer(OpenGLPlatformLayer* platformLayer, RendererSpawnInfo* spawnInfo) {
	OpenGLRenderer* renderer = new OpenGLRenderer;
	renderer->type = RENDERER_OPENGL;
    
	renderer->platformLayer = platformLayer;
	renderer->nextHandle = 1;
    
	return renderer;
}

void DeleteOpenGLRenderer(OpenGLRenderer* renderer) {
	renderer->platformLayer->Destroy();
	delete renderer->platformLayer;
	
	delete renderer;
}








RendererHandle OpenGLRenderer::CreateBuffer(const BufferDescription* desc) {
	u32 glHandle;
	glGenBuffers(1, &glHandle);
    
	GLbitfield flags = 0;
    
	flags |= GL_MAP_PERSISTENT_BIT;
    
	if (desc->flags && RENDERBUFFER_FLAGS_USAGE_DYNAMIC) flags |= GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT |  GL_MAP_WRITE_BIT;
    
	glNamedBufferStorage(glHandle, desc->size, desc->data, flags);
	
	OpenGLBuffer buffer {};
	buffer.flags = desc->flags;
	buffer.size = desc->size;
	buffer.glHandle = glHandle;
    
	RendererHandle handle = this->nextHandle++; 
	ArrayAdd(&this->bufferHandleKeys, handle);
	ArrayAdd(&this->buffers, buffer);
	return handle;
}
void OpenGLRenderer::FreeBuffer(RendererHandle handle) {
	glDeleteBuffers(1, &handle);
}

Buffer* OpenGLRenderer::LookupBuffer(RendererHandle handle) {
    Size idx = ArrayFind(&bufferHandleKeys, handle);
    Assert(idx != -1);
    return &buffers[idx];    
}




RendererHandle OpenGLRenderer::CreatePass(const RenderPassDescription* desc) {
	return 0;
}
void OpenGLRenderer::FreePass(RendererHandle handle) {
	
}
RenderPass* OpenGLRenderer::LookupPass(RendererHandle handle) {
	auto buffer = (RenderPass*)TempAlloc(sizeof(RenderPass));
	Memset(buffer, sizeof(buffer), 0);
	return buffer;
}





RendererHandle OpenGLRenderer::CreateFramebuffer(const FramebufferDescription* desc) {
	
}
void OpenGLRenderer::FreeFramebuffer(RendererHandle handle) {
	
}
Framebuffer* OpenGLRenderer::LookupFramebuffer(RendererHandle handle) {
	auto buffer = (Framebuffer*)TempAlloc(sizeof(Framebuffer));
	Memset(buffer, sizeof(buffer), 0);
	return buffer;
}




RendererHandle OpenGLRenderer::CreateTextureView(const TextureViewDescription* desc) {
	
}
void OpenGLRenderer::FreeTextureView(RendererHandle handle) {
	
}
TextureView* OpenGLRenderer::LookupTextureView(RendererHandle handle) {
	auto buffer = (TextureView*)TempAlloc(sizeof(TextureView));
	Memset(buffer, sizeof(buffer), 0);
	return buffer;
}




RendererHandle OpenGLRenderer::CreateTexture(const TextureDescription* desc) {
	
}
void OpenGLRenderer::FreeTexture(RendererHandle handle) {
	
}
Texture* OpenGLRenderer::LookupTexture(RendererHandle handle) {
	auto buffer = (Texture*)TempAlloc(sizeof(Texture));
	Memset(buffer, sizeof(buffer), 0);
	return buffer;
}





RendererHandle OpenGLRenderer::CreateSwapchain(const SwapchainDescription* desc) {
	return 0;
}
void OpenGLRenderer::FreeSwapchain(RendererHandle handle) {
	
}
Swapchain* OpenGLRenderer::LookupSwapchain(RendererHandle handle) {
	return NULL;
}



RendererHandle OpenGLRenderer::CreateShader(const ShaderDescription* desc) {
	OpenGLShader shader;
    
	shader.glHandle = glCreateProgram();
    
    For (*desc->stages) {
		u32 shaderType = 0;
		StringView shaderTypename = "";
		switch (it->type) {
			case SHADER_VERTEX: shaderType = GL_VERTEX_SHADER; shaderTypename = "vertex"; break;
			case SHADER_PIXEL:  shaderType = GL_FRAGMENT_SHADER; shaderTypename = "pixel"; break;
		}
		u32 stage = glCreateShader(shaderType);
        
		glShaderBinary(1, &stage, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, it->data, it->size);
		glSpecializeShaderARB(stage, "main", 0, NULL, NULL);
		
		GLint success = GL_FALSE;
		glGetShaderiv(stage, GL_COMPILE_STATUS, &success);
		if(success == GL_FALSE)
		{
			GLchar errorLog[1024] = {0};
			glGetShaderInfoLog(stage, 1024, NULL, errorLog);
			LogError("[gl] error compiling %s shader: \n%s", shaderTypename.data, errorLog);
		}
        
		glAttachShader(shader.glHandle, stage);
    }
    
    
	glLinkProgram(shader.glHandle);
	GLint success = GL_FALSE;
	glGetProgramiv(shader.glHandle, GL_LINK_STATUS, &success);
	if(success == GL_FALSE)
	{
		GLchar errorLog[1024] = {0};
		glGetProgramInfoLog(shader.glHandle, 1024, NULL, errorLog);
		LogError("[gl] error linking shader: \n%s", errorLog);
	}
	
    
    shader.vertexInputLayouts = MakeArray<VertexInputLayout>(desc->vertexInputLayouts->size);
    Memcpy(shader.vertexInputLayouts.data, desc->vertexInputLayouts->data, sizeof(VertexInputLayout) * desc->vertexInputLayouts->size);
    
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
    
    RendererHandle handle = nextHandle++;
    ArrayAdd(&shaderHandleKeys, handle);
    ArrayAdd(&shaders, shader);
	return handle;
}

void OpenGLRenderer::FreeShader(RendererHandle handle) {
	// ~Incomplete
}

Shader* OpenGLRenderer::LookupShader(RendererHandle handle) {
	Size idx = ArrayFind(&shaderHandleKeys, handle);
    Assert(idx != -1);
    return &shaders[idx]; 
}




void* OpenGLRenderer::MapBuffer(RendererHandle handle) {
	auto buffer = (OpenGLBuffer*)LookupBuffer(handle);
	return glMapNamedBuffer(buffer->glHandle, GL_READ_WRITE);
}

void OpenGLRenderer::UnmapBuffer(RendererHandle handle) {
	auto buffer = (OpenGLBuffer*)LookupBuffer(handle);
	glUnmapNamedBuffer(buffer->glHandle);
} 



RenderInfo OpenGLRenderer::BeginRender(RendererHandle swapchain) {
	RenderInfo info {};
	platformLayer->MakeCurrent();
	return info;
}
void OpenGLRenderer::EndRender(RendererHandle swapchain) {
	platformLayer->SwapBuffers();
}

void OpenGLRenderer::CmdBeginPass(RendererHandle pass, RendererHandle renderTarget, Rect renderArea) {
	// ~Incomplete
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
}

void OpenGLRenderer::CmdEndPass() {
	
}

void OpenGLRenderer::CmdSetShader(RendererHandle handle) {
	auto shader = (OpenGLShader*)LookupShader(handle);
	glUseProgram(shader->glHandle);
}

#include "mesh.h"
void OpenGLRenderer::CmdBindVertexBuffers(u32 firstBinding, Array<RendererHandle>* buffers, Array<Size>* offsets) {
	ForIdx (*buffers, idx) { 
		auto buf = (OpenGLBuffer*)LookupBuffer(buffers->data[idx]);
		// ~Optimize
		glBindBuffer(GL_ARRAY_BUFFER, buf->glHandle);
        // ~Incomplete, read this date from pipeline 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), NULL);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)Offset(MeshVertex, normal));
	}
}
void OpenGLRenderer::CmdDrawIndexed(u32 indexCount, u32 firstIndex, u32 vertexOffset, u32 instanceCount, u32 firstInstance) {
	// ~Incomplete read primitive type from the active render pipeline
	glDrawArraysInstancedBaseInstance(GL_TRIANGLES, firstIndex, indexCount, instanceCount, firstInstance);
}

void OpenGLRenderer::CmdSetConstantBuffers(RendererHandle shaderHandle, u32 startSlot, Array<RendererHandle>* constantBuffers) {
	
}

void OpenGLRenderer::CmdUpdateBuffer(RendererHandle buffer, Size start, Size size, void* data) {
	
}
void OpenGLRenderer::CmdPushConstants(RendererHandle shader, Size start, Size size, void* data) {
	
}





#endif
