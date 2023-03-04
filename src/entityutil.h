#pragma once

#include "entity.h"

#include "str.h"




struct World;

struct EntityRegEntry {
	char name[64];
	EntityTypeId typeId;
	Size entitySize;
	void(*setupFunc)(Entity*);
};

void _RegisterEntity(StringView name, Size entitySize, void(*setupFunc)(Entity*));
Array<EntityRegEntry> GetEntityRegistrar();
bool EntityEntryName(StringView name, EntityRegEntry* entry);
bool EntityEntryTypeId(EntityTypeId typeId, EntityRegEntry* entry);

// ~Hack replace this with metaprogramming for the entity system!!!
#define GetEntityEntry(entityType, entry) { bool _ = EntityEntryName(#entityType, entry); AssertMsg(_, "Invalid entity type!"); }


#define RegisterEntity(entity) int __register##entity() 	\
{ 														\
	_RegisterEntity(#entity, sizeof(entity), [](Entity* act) \
	{ 													\
		entity theEntity = {};							\
		Memcpy(act, &theEntity, sizeof(theEntity));		\
	}); 												\
	return 0;											\
};														\
int __register##entity##data = __register##entity();		


Entity* SpawnEntity(EntityRegEntry entry, World* world, Xform xform);
Entity* SpawnEntity(StringView name, World* world, Xform xform);
Entity* SpawnEntity(EntityTypeId typeId, World* world, Xform xform);
bool DeleteEntity(World* world, Entity* entity);

void ReloadEntity(World* world, Entity* entity);

Entity* DeserializeEntity(Array<u8> data, World* world);
bool   SerializeEntity(Entity* entity, Array<u8>* data);

Entity* LoadEntity(StringView entity, World* world);
bool   SaveEntity(StringView mapPath, Entity* entity);

bool LoadWorld(StringView worldPath, World* loadWorld);
bool SaveWorld(StringView worldPath, World* world);
