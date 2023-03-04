#pragma once

#include "array.h"

#include "entity.h"

#include "renderer.h"
#include "event.h"
#include "physics.h"
#include "entityutil.h"

#include "terrain.h"

#include "audio.h"

#include "game_manager.h"


constexpr EntityId ACTOR_FIRST_ID = 1;

constexpr Size ACTOR_POOL_BLOCK_SIZE = 32;
struct EntityPoolBlock {
	EntityPoolBlock* next;
	Size size;
	bool mask[ACTOR_POOL_BLOCK_SIZE];
};

constexpr u32 Audio_Player_Block_Size = 32;

struct EntityPool {
	Size actorSize;
	EntityPoolBlock* first;
};


#define SECTOR_SIZE 128
struct WorldSectorId {
	int x, z;
};

inline bool operator==(WorldSectorId a, WorldSectorId b) {
	return a.x == b.x && a.z == b.z;
}


struct WorldSector {
	bool active;
	bool loaded; // from disk
	DynArray<EntityId> entities;
};


struct World {
	String path;

	EntityId nextEntityId;

	DynArray<EntityTypeId> entityPoolTypes;
    DynArray<EntityPool> entityPools;

	Map<WorldSectorId, WorldSector> sectors;

	// physics
	PhysicsWorld physicsWorld;

	// terrain
	DynArray<TerrainChunkIdx> terrainKeys;
	DynArray<EntityId> terrains;

	// audio
	BucketArray<Audio_Player_Block_Size, AudioPlayer> audioPlayers;

	// gameplay
    GameManager gameManager;
};

WorldSectorId GetSectorId(Vec3 pos);

void InitWorldSector(WorldSector* sector);
void DestroyWorldSector(WorldSector* sector);
void UpdateSectors(World* world);


World CreateWorld();
void DeleteWorld(World* world);
void UpdateWorld(World* world);
Entity* GetWorldEntity(World* world, EntityId EntityId);



EntityPool MakeEntityPool(Size actorSize);
void FreeEntityPool(World* world, EntityPool EntityPool);

void* AddEntityToPool(EntityPool EntityPool, Entity* actor);
void RemoveEntityFromPool(EntityPool EntityPool, EntityId EntityId);

void CleanupEntityPool(EntityPool EntityPool);

bool GetWorldEntityPool(World* world, EntityTypeId typeId, EntityPool* pool);

Mat4 EntityWorldXformMatrix(World* world, Entity* actor);
// ~Todo util functions to get actor pos and stuff in world space @@EntityXformUtil

struct EntityPoolIt {
	EntityPool pool;
	EntityPoolBlock* block;
	Size index;
};

EntityPoolIt EntityPoolBegin(EntityPool pool);
EntityPoolIt EntityPoolNext(EntityPoolIt it);
bool EntityPoolItValid(EntityPoolIt it);
Entity* GetEntityFromPoolIt(EntityPoolIt it);