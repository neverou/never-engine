#include "resource.h"

TextureResource* LoadTexture(StringView name)
{
	{
		auto* existing = SearchResource(&resourceManager.textures, name);
		if (existing) {
			// Log("Existing model! %s", name.data);
			return existing;
		}
	}

	auto* renderer = game->renderer;

	File file;
	if (Open(&file, TPrint("%s", name.data), FILE_READ)) {
		Size length = FileLength(&file);

		void* data = TempAlloc(length);
		Read(&file, data, length);

		Close(&file);

		stbi_set_flip_vertically_on_load(true);

		// ~Todo Add HDR textures
		s32 w, h, channels;
		u8* textureData = stbi_load_from_memory((u8*)data, length, &w, &h, &channels, 4);

		if (!textureData) {
			LogWarn("[res] Failed to load image (%s): %s", name.data, stbi_failure_reason());
		}

		RendererHandle textureHandle = renderer->CreateTexture(w, h, FORMAT_R8G8B8A8_UNORM_SRGB, 1, TextureFlags_Sampled);

		renderer->UploadTextureData(textureHandle, w * h * RenderFormatBytesPerPixel(FORMAT_R8G8B8A8_UNORM_SRGB), textureData);


		TextureResource texture = {};
		texture.name = CopyString(name);
		texture.textureHandle = textureHandle;

		Log("[res] loaded texture (%s)", name.data);
		return BucketArrayAdd(&resourceManager.textures, texture);
	}

	LogWarn("[res] Tried to read texture that does not exist!");

	// ~Todo return default image?
	return NULL;
}

