#include "resource.h"
#include "game.h"
#include "allocators.h"

#include "logger.h"

#include "sys.h"

intern ResourceManager resourceManager;

#include "stb/stb_image.h"

void InitResourceManager() {
	resourceManager.shaders  = MakeBucketArray<Resource_Bucket_Size, ShaderResource>();
	resourceManager.models   = MakeBucketArray<Resource_Bucket_Size, ModelResource>();
	resourceManager.textures = MakeBucketArray<Resource_Bucket_Size, TextureResource>();
	resourceManager.audios   = MakeBucketArray<Resource_Bucket_Size, AudioResource>();
	resourceManager.anims    = MakeBucketArray<Resource_Bucket_Size, AnimResource>();
}

void DestroyResourceManager() { // ~TODO free each resource aswell
	FreeBucketArray(&resourceManager.shaders);
	FreeBucketArray(&resourceManager.models);
	FreeBucketArray(&resourceManager.textures);
	FreeBucketArray(&resourceManager.audios);
	FreeBucketArray(&resourceManager.anims);
}

ResourceManager* GetResourceManager() {
	return &resourceManager;
}




// Utilities
template<typename T>
intern T* SearchResource(BucketArray<Resource_Bucket_Size, T>* array, StringView name) {
	for (auto it = BucketArrayBegin(array); BucketIteratorValid(it); it = BucketIteratorNext(it)) {
		T* resource = GetBucketIterator(it);
		
		if (Equals(resource->name, name))
		{
			return resource;
		}
	}
	return NULL;
}


// ~Temp: use custom format
const StringView File_Extension_Model = ".smd";

Array<String> AllModelNames()
{
	// ~Hack: todo recursive search from <<res/>>

	DynArray<String> modelNames = MakeDynArray<String>(0, Frame_Arena);

	auto modelPaths = ListAllFiles("models");
	For (modelPaths)
	{
		if (Equals(Substr(*it, it->length - 4, it->length), File_Extension_Model))
		{
			String modelName = TPrint("models/%s", it->data);
			ArrayAdd(&modelNames, modelName);
		}
	}

	return modelNames;
}

#include "resource_shader.cpp"
#include "resource_model.cpp"
#include "resource_texture.cpp"
#include "resource_material.cpp"
#include "resource_audio.cpp"
#include "resource_anim.cpp"