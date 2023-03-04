#include "world.h"
#include "std.h"

// entity includes
#include "player.h"
#include "prop.h"
#include "enemy.h"
#include "zefir.h"
//

#include "allocators.h"

#include "logger.h"

#include <string.h>



WorldSectorId GetSectorId(Vec3 pos)
{
	return {
		.x = (int)Floor(pos.x / SECTOR_SIZE),
		.z = (int)Floor(pos.z / SECTOR_SIZE)
	};
}

//


void InitWorldSector(WorldSector* sector)
{
	sector->active = false;
	sector->entities = MakeDynArray<EntityId>();
}

void DestroyWorldSector(WorldSector* sector)
{
	FreeDynArray(&sector->entities);
}


void UpdateSectors(World* world)
{
	ForIdx (world->sectors.keys, keyIdx) {
		auto key = world->sectors.keys[keyIdx];
		auto it = &world->sectors[key];

		// Don't touch inactive sectors
		if (!it->active) continue;

		WorldSectorId oldSectorId = key;

		ForIdx (it->entities, entIdx)
		{
			auto* entIt = &it->entities[entIdx];
			Entity* entity = GetWorldEntity(world, *entIt);

			if (entity == NULL)
			{
				ArrayRemoveAt(&it->entities, entIdx);
				entIdx--; // removed the current item so we gotta step back 1
				continue;
			}

			// @EntityXformUtil
			WorldSectorId newSectorId = GetSectorId(Mul(v3(0), 1, EntityWorldXformMatrix(world, entity)));

			if (oldSectorId.x != newSectorId.x ||
				oldSectorId.z != newSectorId.z)
			{
				Log("@@@ %lu-> %f, %f, %f", entity->id, entity->xform.position.x, entity->xform.position.y, entity->xform.position.z);
				Log("@@@ Sector (%s) %d, %d, -> %d, %d", entity->name.data, oldSectorId.x, oldSectorId.z, newSectorId.x, newSectorId.z);	

				EntityId entId = *entIt; // save the ID before its removed from the array

				ArrayRemoveAt(&it->entities, entIdx);
				entIdx--; // removed the current item so we gotta step back 1

				// If it doesnt exist create it
				if (!HasKey(world->sectors, newSectorId))
				{
					Insert(&world->sectors, newSectorId, { });
					Log("[world] New sector (%d, %d)", newSectorId.x, newSectorId.z);
					InitWorldSector(&world->sectors[newSectorId]);
					world->sectors[newSectorId].active = true;

					it = &world->sectors[key]; // When you update the sectors the memory address of one might change SO lets not do that lol
				}

				Assert(HasKey(world->sectors, newSectorId));
				WorldSector* newSector = &world->sectors[newSectorId];
				ArrayAdd(&newSector->entities, entId);
			}

			Assert(it->entities.size < 100);
		}
	}

	// @Todo(...): Figure out why this is commented out 
	// ForIdx (world->sectors.keys, keyIdx) {
	// 	auto key = world->sectors.keys[keyIdx];
	// 	auto it = &world->sectors[key];

	// 	if (it->entities.size == 0) {
	// 		Log("[world] Remove sector (%d, %d)", key.x, key.z);

	// 		DestroyWorldSector(it);
			
	// 		Remove(&world->sectors, key);
	// 		keyIdx--; // removed the current item so we gotta step back 1
	// 	}
	// }
}



World CreateWorld() {
    World world = {};

	world.path = MakeString();

    //
	world.nextEntityId = ACTOR_FIRST_ID;
    
    
	world.entityPoolTypes = MakeDynArray<EntityTypeId>();
    world.entityPools = MakeDynArray<EntityPool>();
    
	For (GetEntityRegistrar()) {
		ArrayAdd(&world.entityPoolTypes, it->typeId);
        
		auto pool = MakeEntityPool(it->entitySize);
		ArrayAdd(&world.entityPools, pool);
	}
    
	world.sectors = MakeMap<WorldSectorId, WorldSector>();
	
	// physics
	InitPhysicsWorld(&world.physicsWorld);
    
	// terrain
	world.terrainKeys = MakeDynArray<TerrainChunkIdx>();
	world.terrains = MakeDynArray<EntityId>();
    
	// audio
	world.audioPlayers = MakeBucketArray<Audio_Player_Block_Size, AudioPlayer>();
    
	// gameplay
	InitGameManager(&world.gameManager);

    return world;
}

void DeleteWorld(World* world)
{

	// terrain
	FreeDynArray(&world->terrainKeys);
	FreeDynArray(&world->terrains);

	//
	For (world->entityPools)
	{
		for (EntityPoolIt actorIt = EntityPoolBegin(*it); EntityPoolItValid(actorIt); actorIt = EntityPoolNext(actorIt))
		{
			Entity* actor = GetEntityFromPoolIt(actorIt);
			DeleteEntity(world, actor);
		}
	}

	DestroyPhysicsWorld(&world->physicsWorld);

	FreeDynArray(&world->entityPools);
	FreeDynArray(&world->entityPoolTypes);

	For (world->sectors.values)
		DestroyWorldSector(it);
	FreeMap(&world->sectors);

	FreeString(&world->path);



	// audio
	FreeBucketArray(&world->audioPlayers);

	// gameplay
	DestroyGameManager(&world->gameManager);
}


void UpdateWorld(World* world) {
	For (world->entityPools)
		CleanupEntityPool(*it);
    

	// Run start function on entities
	ForIt (world->entityPools, poolIt)
	{
		// TODO: make a macro for this
		// (the inside of the for loop)
		for (EntityPoolIt it = EntityPoolBegin(*poolIt); EntityPoolItValid(it); it = EntityPoolNext(it))
		{
			Entity* ent = GetEntityFromPoolIt(it);

			if (!ent->started)
			{
				ent->Start(world);
				ent->started = true;
			}
		}
	}

	ForIt (world->entityPools, poolIt)
	{
		for (EntityPoolIt it = EntityPoolBegin(*poolIt); EntityPoolItValid(it); it = EntityPoolNext(it))
		{
			GetEntityFromPoolIt(it)->Update(world);
		}
	}

	UpdateSectors(world);
}


Entity* GetWorldEntity(World* world, EntityId entityId)
{
	if (entityId == 0) return NULL;
    
	ForIt (world->entityPools, pool)
	{
		for (EntityPoolIt it = EntityPoolBegin(*pool); EntityPoolItValid(it); it = EntityPoolNext(it))
		{
			Entity* entity = GetEntityFromPoolIt(it);
			if (entity->id == entityId)
				return entity;
		}
	}
    
	return NULL;
}





intern EntityPoolBlock* AllocEntityPoolBlock(Size size) {
	EntityPoolBlock* block = (EntityPoolBlock*)Alloc(sizeof(EntityPoolBlock) + size);

	block->next = NULL;
	block->size = size;
	// Make sure the block mask is filled with false by default
	Memset(block->mask, sizeof(block->mask), 0);

	return block;
}


intern void FreeEntityPoolBlock(EntityPoolBlock* block) {
    AssertMsg(block->next == NULL, "Freeing a non-leaf entity pool block");
    
	Free(block);
}





EntityPool MakeEntityPool(Size actorSize) {
	EntityPool EntityPool;
    
	EntityPool.actorSize = actorSize;
	EntityPool.first = AllocEntityPoolBlock(actorSize * ACTOR_POOL_BLOCK_SIZE);
    
	return EntityPool;
}

void FreeEntityPool(World* world, EntityPool EntityPool) {
	auto stack = MakeDynArray<EntityPoolBlock*>(1, Frame_Arena); 
	defer(FreeDynArray(&stack));
    
	auto* head = EntityPool.first;
	while (head != NULL) {
		ArrayAdd(&stack, head);
		head = head->next;
	}
    
	for (Size i = stack.size - 1; i >= 0; i--) {
		auto* block = stack[i];
		FreeEntityPoolBlock(block);
	}
}


bool GetWorldEntityPool(World* world, EntityTypeId typeId, EntityPool* pool) {
	Size idx = ArrayFind(&world->entityPoolTypes, typeId);
	if (idx == -1)
		return false;
	*pool = world->entityPools[idx];
	return true;
}

void* AddEntityToPool(EntityPool EntityPool, Entity* actor) {
	EntityPoolBlock* block = EntityPool.first;
    
	while (block) {
		u8* actorMemory = (u8*)(block + 1);
		for (Size i = 0; i < ACTOR_POOL_BLOCK_SIZE; i++) {
			if (!block->mask[i]) {
				block->mask[i] = true;
                
				void* actorPtr = actorMemory + EntityPool.actorSize * i;
				Memcpy(actorPtr, actor, EntityPool.actorSize);
				return actorPtr;
			}
		}
        
		if (!block->next) {
			block->next = AllocEntityPoolBlock(EntityPool.actorSize * ACTOR_POOL_BLOCK_SIZE);
		}
		
		block = block->next;
	}
    
    // How did you get here??
	Assert(false); 
	return NULL;
}

void RemoveEntityFromPool(EntityPool EntityPool, EntityId EntityId) {
	for (EntityPoolIt it = EntityPoolBegin(EntityPool); EntityPoolItValid(it); it = EntityPoolNext(it)) {
		Entity* actor = GetEntityFromPoolIt(it);
		if (actor->id == EntityId) {
			it.block->mask[it.index] = false; 
			return;
		} 
	}
    
	FatalError("Tried to delete an actor that wasn't in the pool!");
}



void CleanupEntityPool(EntityPool EntityPool) {
    // Shift actors to the left and remove any unused space
    
	// ~Optimize This is actually garbage, dont do this please (bubble sort thing)
    // 
    // We're looping through and moving each actor left individually rather than
    // measuring empty space and shifting everything after it left
    
	bool unaligned = true;
	while (unaligned)
    {
		unaligned = false;
        
		bool* prevMask = NULL;
		Entity* prevActor = NULL;
        
        
		EntityPoolBlock* block = EntityPool.first;
		while (block) {
			u8* actorMemory = (u8*)(block + 1);
			for (Size i = 0; i < ACTOR_POOL_BLOCK_SIZE; i++) {
				
				bool* mask = &block->mask[i];
				Entity* actor = (Entity*)(actorMemory + EntityPool.actorSize * i);
				
				if (!(prevMask == NULL || prevActor == NULL))
                {
					if (!(*prevMask) && (*mask))
                    {
						*mask     = false;
						*prevMask = true;
                        
						Memcpy(prevActor, actor, EntityPool.actorSize);
                        
						unaligned = true;
					}
				}
                
				prevMask = mask;
				prevActor = actor;
                
                
			}
			block = block->next;
		}
	}
    
    
	// Remove unused blocks
	{
		auto stack = MakeDynArray<EntityPoolBlock*>(0, Frame_Arena); 
		defer(FreeDynArray(&stack));
        
		{
			EntityPoolBlock* block = EntityPool.first;
			while (block) {
				ArrayAdd(&stack, block);
				block = block->next;
			}
		}
        
		for (Size i = stack.size - 1; i > 0; i--) {
			EntityPoolBlock* block = stack[i];
            
			bool isBlockUsed = false;
			for (Size i = 0; i < ACTOR_POOL_BLOCK_SIZE; i++) {
				if (block->mask[i]) {
					isBlockUsed = true;
					break;
				}
			}
            
			if (!isBlockUsed) {
				Assert(block->next == NULL);
				FreeEntityPoolBlock(block);
				stack[i - 1]->next = NULL;// Hehe  poo poo
			}
		}
	}
}




EntityPoolIt EntityPoolBegin(EntityPool pool)
{
	EntityPoolIt it {};
	it.index = 0;
	it.block = pool.first;
	it.pool = pool;
    
	if (!EntityPoolItValid(it))
		it = EntityPoolNext(it);
	return it;
}

EntityPoolIt EntityPoolNext(EntityPoolIt it)
{
	// This function implementation is equivilent to BucketIteratorNext
	for (;;) {
		it.index++;
        
		if (it.index >= ACTOR_POOL_BLOCK_SIZE) {
			it.index = 0;
			it.block = it.block->next;
		}
        
		if (!it.block || it.block->mask[it.index])
			break;
	}
    
	return it;
}

bool EntityPoolItValid(EntityPoolIt it) {
	// This function implementation is equivilent to BucketIteratorValid
	return it.block != NULL && it.block->mask[it.index];
}



Entity* GetEntityFromPoolIt(EntityPoolIt it) {
	Assert(it.block != NULL);
	Assert(it.block->mask[it.index]);
    
	Entity* actor = (Entity*)(((u8*)(it.block + 1)) + (it.pool.actorSize * it.index));
	return actor;
}






Mat4 EntityWorldXformMatrix(World* world, Entity* actor) {
	Mat4 worldMatrix = CreateMatrix(1);
	
	auto actorStack = MakeDynArray<Entity*>(0, Frame_Arena);
	defer(FreeDynArray(&actorStack));
    
	Entity* current = actor;
	while (current != NULL) {
		ArrayAdd(&actorStack, current);
		current = GetWorldEntity(world, current->parent);
	}
    
	for (auto* it = actorStack.data + actorStack.size; it != actorStack.data; ) { 
		it--;
        
		Entity* itActor = *it;
		worldMatrix = Mul(worldMatrix, XformMatrix(itActor->xform));
	}
    
	return worldMatrix;
}