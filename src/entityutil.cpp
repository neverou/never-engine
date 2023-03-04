#include "entityutil.h"

#include "array.h"
#include "util.h"
#include "str.h"

#include "game.h"
#include "allocators.h"

intern DynArray<EntityRegEntry> entityRegistrar;
intern EntityTypeId nextTypeId;

intern void InitRegistrar() {
	// ~CleanUp de-init the registrar on exit probably
	entityRegistrar = MakeDynArray<EntityRegEntry>();
	nextTypeId = 1;
}


// NOTE(voidless):
// ONLY using the C-runtime library because the logger has not been initialized before main();
// eg gotta use printf
// this will get removed when i add metaprogramming for entities
#include <stdio.h>

void _RegisterEntity(StringView name, u64 actorSize, void(*setupFunc)(Entity*))
{
	local_persist bool init = false;
	if (!init)
    {
		InitRegistrar();
		init = true;
	}
    
	For (entityRegistrar)
    {
		if (Equals(name, it->name))
        {
			printf("[entity] Tried to add duplicate actor to registrar (%s)\n", name.data);
			return;
		}
	}
    
    
	EntityRegEntry entry {};
	Assert(name.length < sizeof(entry.name));

	Memcpy(entry.name, name.data, name.length);
	entry.entitySize = actorSize;
	entry.typeId = nextTypeId++;
    entry.setupFunc = setupFunc;

	// ~Todo: make some mode for the logger to work before the game has started?
	printf("[entity] registered entity type %s (%u)\n", entry.name, entry.typeId);
    
	ArrayAdd(&entityRegistrar, entry);
}

Array<EntityRegEntry> GetEntityRegistrar()
{
	return entityRegistrar;
}

bool EntityEntryName(StringView name, EntityRegEntry* entry)
{
	For (entityRegistrar)
    {
		if (Equals(it->name, name))
        {
			*entry = *it;
			return true;
		}
	}
	return false;
}

bool EntityEntryTypeId(EntityTypeId typeId, EntityRegEntry* entry)
{
	For (entityRegistrar)
    {
		if (it->typeId == typeId)
        {
			*entry = *it;
			return true;
		}
	}
	return false;
}


intern Entity* InitEntity(World* world, EntityRegEntry entry)
{
	Size idx = ArrayFind(&world->entityPoolTypes, entry.typeId);
	if (idx == -1) 
		return NULL;

	Entity* entity = (Entity*)TempAlloc(entry.entitySize);

    // vtable and stuff. completely overrides the actor memory so this has to be first
	entry.setupFunc(entity); 
	entity->xform   = CreateXform();
	entity->typeId  = entry.typeId;
    entity->flags   = 0;
    entity->started = false;
    entity->name    = MakeString();

	// init strings (all the other types are already init by default because ZII)
	{
		DataDesc* layout = entity->GetDataLayout();
		if (layout != NULL)
        {
			while (layout->type != FIELD_NONE)
            {
				if (layout->type == FIELD_STRING)
				{
					void* field = (u8*)entity + layout->offset;
					String* str = (String*)field;
					*str = MakeString();
				}
				layout++;
			}
		}
	}
        
	return (Entity*)AddEntityToPool(world->entityPools[idx], entity);
}

// For random generator
#include <stdlib.h>
#include <time.h>
//

// generate a unique id for the actor
intern EntityId GenEntityId()
{
	static bool init = false;
	if (!init)
	{
		srand(time(NULL));
		init = true;
	}

	return (EntityId)rand(); // ~Hack Not a great solution...
}




intern void IntroduceEntityToSector(World* world, Entity* entity)
{
    //     sector stuff     //

    // make sure that the entity isn't global
    // (eg. not belonging to any sector bc otherwise it would get unloaded)
    if ((entity->flags & Entity_Global) == 0)
    {
        // @EntityXformUtil
        WorldSectorId sectorId = GetSectorId(Mul(v3(0), 1, EntityWorldXformMatrix(world, entity)));

        // logging //
        // Log("### %lu-> %f, %f, %f", ent->id, ent->xform.position.x, ent->xform.position.y, ent->xform.position.z);
        // Log("### %lu-> %d, %d", ent->id, ent->xform.position.x, sectorId.x, sectorId.z);

        // Look for the sector this entity would belong to
        WorldSector* sector = NULL;

        // If it doesnt exist create it
        if (!HasKey(world->sectors, sectorId))
        {
            Insert(&world->sectors, sectorId, { });
            Log("[world] New sector (%d, %d)", sectorId.x, sectorId.z);
            
            sector = &world->sectors[sectorId];
            InitWorldSector(sector);
        }
        else
        {
            sector = &world->sectors[sectorId];
        }

        sector->active = true;

        AssertMsg(ArrayFind(&sector->entities, entity->id) == -1, "Entity ended up being added to the sector twice");
        ArrayAdd(&sector->entities, entity->id);
    }
}





Entity* SpawnEntity(EntityRegEntry entry, World* world, Xform xform) {
	Entity* entity = InitEntity(world, entry);
	if (entity != NULL) {
		entity->id = GenEntityId();
		entity->xform = xform;
		entity->Init(world);
        // ~TODO @RuntimeSpawnEntity make SpawnEntity only for runtime entities! (eg make a different one for the editor's usage) 
        IntroduceEntityToSector(world, entity);
	}
	return entity;
}

Entity* SpawnEntity(StringView name, World* world, Xform xform) {
	EntityRegEntry entry;
	if (!EntityEntryName(name, &entry)) return NULL;
	return SpawnEntity(entry, world, xform);
}

Entity* SpawnEntity(EntityTypeId typeId, World* world, Xform xform) {
	EntityRegEntry entry;
	if (!EntityEntryTypeId(typeId, &entry)) return NULL;
	return SpawnEntity(entry, world, xform);
}


#include "resource.h"

Entity* DeserializeEntity(Array<u8> data, World* world)
{
    TextHandler handler = { };
    handler.source.data   = (char*)data.data;
    handler.source.length = data.size;
    
    bool foundEntType = false;
    StringView entityType;
    Xform xform = CreateXform();

    bool foundEntName = false;
    StringView entityName = "Unknown";

    u64 entityFlags = 0;

    bool startedAlready = false;
    
    struct ActorKeyValue {
        StringView key;
        StringView value;
    };
    
    // ~Todo Maybe we could put temporary allocators and stuff in a context struct
    auto keyValues = MakeDynArray<ActorKeyValue>(0, Frame_Arena); 
    defer(FreeDynArray(&keyValues));
    
    u32 lineCount = 0;
    EntityId id = 0;
    while (true) {
        bool found = false;
        String line = ConsumeNextLine(&handler, &found);
        lineCount++;
        if (!found) break;

        // ~Cleanup make working with string views easier. maybe even remove them lol
        
        // handle comments
        s32 findHash = FindStringFromLeft(line, '#');
        
        StringView str = line;
        if (findHash != -1) str = Substr(line, 0, findHash);
        
        
        auto parts = BreakString(str, ' ');
        
        if (parts.size >= 2) {
            StringView command = parts[0], rhs = parts[1];
            
            if (Equals(command, "$type"))
            {
                entityType = rhs.data;
                foundEntType = true;
            }
            else if (Equals(command, "$id"))
            {
                id = ParseU64(rhs);
            }
            else if (Equals(command, "$xform"))
            {
                auto xformParts = BreakString(rhs, ';');
                if (xformParts.size != 3)
                {
                    LogError("[entity] Failed to parse transform of actor!");
                    return NULL;
                }

                { // Parse position
                    auto positionParts = BreakString(xformParts[0], ',');

                    if (positionParts.size != 3)
                    {
                        LogError("[entity] Failed to parse transform position of actor!");
                        return NULL;
                    }

                    bool sucX, sucY, sucZ;
                    xform.position.x = ParseFloat(positionParts[0], &sucX);
                    xform.position.y = ParseFloat(positionParts[1], &sucY);
                    xform.position.z = ParseFloat(positionParts[2], &sucZ);
                
                    Assert(sucX && sucY && sucZ);
                }
                
                { // Parse rotation
                    auto rotationParts = BreakString(xformParts[1], ',');

                    if (rotationParts.size != 16)
                    {
                        LogError("[entity] Failed to parse transform rotation of actor!");
                        return NULL;
                    }

                    bool success = true;
                    ForIdx (rotationParts, idx)
                    {
                        bool s;
                        xform.rotation.m[idx] = ParseFloat(rotationParts[idx], &s);
                        success &= s;
                    }
                    Assert(success);
                }

                { // Parse scale
                    auto scaleParts = BreakString(xformParts[2], ',');

                    if (scaleParts.size != 3)
                    {
                        LogError("[entity] Failed to parse transform scale of actor!");
                        return NULL;
                    }

                    bool sucX, sucY, sucZ;
                    xform.scale.x = ParseFloat(scaleParts[0], &sucX);
                    xform.scale.y = ParseFloat(scaleParts[1], &sucY);
                    xform.scale.z = ParseFloat(scaleParts[2], &sucZ);
                
                    Assert(sucX && sucY && sucZ);
                }
                
            }
            else if (Equals(command, "$name"))
            {
                entityName = rhs;
                foundEntName = true;
            }
            else if (Equals(command, "$flags"))
            {
                entityFlags = ParseU64(rhs);
            }
            else if (Equals(command, "$started"))
            {
                startedAlready = Equals(rhs, "1");
            }
            else
            {
                ArrayAdd(&keyValues, { command, rhs });
            }
        }
    }
    
    if (id == 0)
    {
        LogError("[entity] actor must have a non-zero id! (%s)", entityName.data);
        return NULL;
    }
    
    
    if (!foundEntType)
    {
        LogError("[entity] invalid actor type file! missing actor type (%s)", entityName.data);
        return NULL;
    }
    
    
    
    // try to create the actor
    EntityRegEntry actorEntry;
    bool foundActorEntry = EntityEntryName(entityType, &actorEntry);
    
    if (!foundActorEntry)
    {
        LogError("[entity] failed to find actor entry for %s in %s", entityType.data, entityName.data);
        return NULL;
    }
    
    Entity* entity = InitEntity(world, actorEntry);
    
    entity->id = id;

    Log("@@@@ %lu", entity->id);

    if (foundEntName)
    {
        entity->name.Update(entityName);
    }
    
    entity->flags   = entityFlags;
    entity->started = startedAlready;

    if (entity != NULL) {
        // init the entity
        entity->xform = xform;
        
        
        DataDesc* layout = entity->GetDataLayout();
        if (layout != NULL) {
            while (layout->type != FIELD_NONE) {
                bool found = false;
                StringView value;
                For (keyValues) {
                    if (Equals(it->key, layout->fieldName)) {
                        value = it->value;
                        found = true;
                        break;
                    }
                }
                
                // TODO(voidless) replace sscanf
                // TODO(voidless) log errors in parsing and return NULL if failed
                if (found) {
                    void* field = (u8*)entity + layout->offset;
                    switch (layout->type) {
                        case FIELD_INT:     sscanf(value.data, "%d", (s32*)field); break;
                        case FIELD_FLOAT:   *((float*)field) = ParseFloat(value); break;
                        case FIELD_VECTOR2: sscanf(value.data, "%f,%f", &((Vec2*)field)->x, &((Vec2*)field)->y); break;
                        case FIELD_VECTOR3: sscanf(value.data, "%f,%f,%f", &((Vec3*)field)->x, &((Vec3*)field)->y, &((Vec3*)field)->z); break;
                        case FIELD_VECTOR4: sscanf(value.data, "%f,%f,%f,%f", &((Vec4*)field)->x, &((Vec4*)field)->y, &((Vec4*)field)->z, &((Vec4*)field)->w); break;
                        case FIELD_STRING: {
                            String* str = (String*)field;
                            str->Update(value);
                            break;
                        }
                        case Field_Model: {
                            ModelResource** modelResource = (ModelResource**)field;
                            *modelResource = LoadModel(value);
                            break;
                        }
                        case Field_Entity: (*(u64*)field) = ParseU64(value); break;
                    }
                    
                }
                else {
                    LogWarn("[entity] Could not load field (%s) while loading %s entity from %s", layout->fieldName, entityType.data, entityName.data);
                }
                
                layout++;
            }
        }
        
        
        entity->Init(world);
    }
    else
    {
        Log("[entity] unknown actor type %s in actor %s", entityType.data, entityName.data);
        return NULL;
    }
    
    return entity;
}

Entity* LoadEntity(StringView actorPath, World* world)
{
    Array<u8> data;
    if (!ReadEntireFile(actorPath.data, &data))
    {
		LogWarn("Failed to load actor %s, no file found~", actorPath.data);
		return NULL;
	}
    
    Entity* actor = DeserializeEntity(data, world);
    
    if (!actor) return NULL;
    
    EntityId id;
    // get id from the filename
    {
        s32 lastSlash = FindStringFromRight(actorPath, '/');
        String EntityIdStr = Substr(actorPath, lastSlash == -1 ? 0 : lastSlash + 1, actorPath.length);
        sscanf(EntityIdStr.data, "%lu", &id);
    }
    
    if (id != actor->id)
        LogWarn("Corruption detected! Entity %lu has file id %lu", actor->id, id);

    return actor;
}

intern void UniversalActorDestroy(World* world, Entity* actor)
{
    // TODO: use dedicated function to remove features of rigidbody!

	if (actor->features & EntityFeature_Rigidbody)
	{
		DestroyRigidbody(&world->physicsWorld, actor->rigidbody);
        
		actor->rigidbody = NULL;
	}

	if (actor->features & EntityFeature_Animator)
	{
		DeleteAnimator(&actor->animator, game->renderer);
	}
    
	actor->flags    = 0;
    actor->features = 0;
}



// Don't delete on the same frame, mark for deletion
bool DeleteEntity(World* world, Entity* actor)
{
	Assert(world);
	Assert(actor);

    FreeString(&actor->name);
	
	EntityRegEntry entry;
	if (EntityEntryTypeId(actor->typeId, &entry))
    {   
		actor->Destroy(world);
		UniversalActorDestroy(world, actor);

		DataDesc* layout = actor->GetDataLayout();
        if (layout != NULL)
        {
			while (layout->type != FIELD_NONE)
            {
				void* field = (u8*)actor + layout->offset;
				switch (layout->type)
                {
					case FIELD_STRING:
                    {
						String* str = (String*)field;
						FreeString(str);
						break;
					}
					case Field_Model:
                    {
						// @@FreeingResources @@MemoryManage
						break;
					}
				}
                
				layout++;
			}
		}
        
		Size idx = ArrayFind(&world->entityPoolTypes, entry.typeId);
		if (idx == -1)
        {
			LogError("[entity] Failed to free actor %u of type %s[%u] (unable to find pool)", actor->id, entry.name, actor->typeId);
			return false;
		}

		RemoveEntityFromPool(world->entityPools[idx], actor->id);
	}
	else
    {
		LogError("[entity] Failed to free actor %u with type id %u (unable to lookup type id)", actor->id, actor->typeId);
		return false;
	}

    return true;
}



void ReloadEntity(World* world, Entity* entity)
{
	Assert(entity);
    
	EntityRegEntry entry;
	if (EntityEntryTypeId(entity->typeId, &entry)) {
		// ~Todo ~Refactor Maybe just clear the entity struct completely and just save the saveload values?>
		entity->Destroy(world);
		UniversalActorDestroy(world, entity);
		entity->Init(world);
	}
}


bool SerializeEntity(Entity* entity, Array<u8>* data)
{
    EntityRegEntry entry;
    if (!EntityEntryTypeId(entity->typeId, &entry)) {
        LogWarn("[entity] Failed to find entity type (%lu) for entity %s", entity->typeId, entity->name.data); // @@entityNames
        return false;
    }
    
    
    String str = TempString();
    
    DataDesc* layout = entity->GetDataLayout();
    
    str.Concat(TPrint("$type %s\n", entry.name));
    str.Concat(TPrint("$id %lu\n", entity->id));
    str.Concat(TPrint("$xform %f,%f,%f;%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f;%f,%f,%f\n",  // %f,%f,%f;%f,%f,%f;%f,%f,%f\n
                          entity->xform.position.x, entity->xform.position.y, entity->xform.position.z,
                          entity->xform.rotation.m[0], entity->xform.rotation.m[1], entity->xform.rotation.m[2], entity->xform.rotation.m[3],
                          entity->xform.rotation.m[4], entity->xform.rotation.m[5], entity->xform.rotation.m[6], entity->xform.rotation.m[7],
                          entity->xform.rotation.m[8], entity->xform.rotation.m[9], entity->xform.rotation.m[10], entity->xform.rotation.m[11],
                          entity->xform.rotation.m[12], entity->xform.rotation.m[13], entity->xform.rotation.m[14], entity->xform.rotation.m[15],
                          entity->xform.scale.x, entity->xform.scale.y, entity->xform.scale.z));
    if (entity->name.length != 0) 
        str.Concat(TPrint("$name %s\n", entity->name.data));
    str.Concat(TPrint("$flags %lu\n", entity->flags));
    str.Concat(TPrint("$started %d\n", entity->started));


    if (layout) {
        while (layout->type != FIELD_NONE) {
            str.Concat(layout->fieldName);
            str.Concat(" ");
            
            void* field = (u8*)entity + layout->offset;
            switch (layout->type) {
                case FIELD_INT:     str.Concat(TPrint("%d", *((int*)field) )); break;
                case FIELD_FLOAT:   str.Concat(TPrint("%f", *((float*)field) )); break;
                case FIELD_VECTOR2: str.Concat(TPrint("%f,%f", ((Vec2*)field)->x, ((Vec2*)field)->y )); break;
                case FIELD_VECTOR3: str.Concat(TPrint("%f,%f,%f", ((Vec3*)field)->x, ((Vec3*)field)->y, ((Vec3*)field)->z)); break;
                case FIELD_VECTOR4: str.Concat(TPrint("%f,%f,%f,%f", ((Vec4*)field)->x, ((Vec4*)field)->y,((Vec4*)field)->z, ((Vec4*)field)->w )); break;
                case Field_Entity:   str.Concat(TPrint("%lu", *((u64*)field))); break;

                case FIELD_STRING:
                {
                    str.Concat(*((String*)field));
                    break;
                }
                
                case Field_Model: 
                {
                    ModelResource** modelResource = (ModelResource**)field;
                    if ((*modelResource))
                        str.Concat((*modelResource)->name);
                    else
                        str.Concat("null");
                    
                    break;
                }
                
                
                default:
                {
                    LogWarn("Failed to serialize field %s\n", layout->fieldName);
                }
            }
            
            str.Concat("\n");
            
            layout++;
        }
    }
    
    *data = Array<u8> { str.length, (u8*)str.data };
    
    return true;
}

bool SaveEntity(StringView mapPath, Entity* entity) {
    Assert(entity != NULL);
    Assert((entity->flags & Entity_Runtime) == 0); // Don't allow us to save a runtime entity, this is a bug in the program!

    StringView savePath = TPrint("%s/%lu.actor", mapPath.data, entity->id);
    
    EntityRegEntry entry;
    if (!EntityEntryTypeId(entity->typeId, &entry)) {
        LogWarn("[entity] Failed to find entity type (%lu) for entity %s", entity->typeId, mapPath.data); // @@ActorNames
        return false;
    }
    
    File file;
    if (Open(&file, savePath, FILE_WRITE | FILE_CREATE | FILE_TRUNCATE))
    {
        Array<u8> data;
        SerializeEntity(entity, &data);
        Write(&file, data.data, data.size);

        Close(&file);
    }
    else 
    {
        LogWarn("[entity] Failed to save %s entity to %s", entry.name, savePath.data);
        return false;
    }
    
    return true;
}



bool LoadWorld(StringView worldPath, World* loadWorld)
{
    World world = CreateWorld();
    
    TextFileHandler handler;
    if (!OpenFileHandler(TPrint("%s", worldPath.data), &handler))
    {
        LogWarn("failed to open world! (%s)", worldPath.data);
        return false;
    }
    defer(CloseFileHandler(&handler));
    
    constexpr int Mode_Global = 0;
    constexpr int Mode_Sector = 1;
    int mode = -1;

    WorldSectorId sectorHead = {};

    for (;;)
    {
        bool found;
        String line = ConsumeNextLine(&handler, &found);
        if (!found) break;

        // Remove comments
        s32 hashPosition = FindStringFromLeft(line, '#');
        if (hashPosition != -1)
            line = Substr(line, 0, hashPosition);        
        line = TrimSpacesLeft(line);

        Array<String> parts = BreakString(line, ' ');

        if (parts.size > 0)
        {

            if (Equals(parts[0], "global"))
            {
                mode = Mode_Global;
            }

            if (Equals(parts[0], "sector"))
            {
                Array<String> rawSectorId = BreakString(parts[1], ',');
                
                WorldSectorId sectorId = {};

                bool success = true;
                if (success) sectorId.x = ParseS32(rawSectorId[0], &success);
                if (success) sectorId.z = ParseS32(rawSectorId[1], &success);

                if (!success)
                {
                    LogError("[world] Failed to parse sector in world (%s)! Perhaps it is corrupt?", worldPath.data);
                    return false;
                }


                
                mode = Mode_Sector;
                sectorHead = sectorId;


                if (HasKey(world.sectors, sectorId))
                {
                    LogError("[world] World (%s) has the same sector twice!! Corrupt much!?", worldPath.data);
                    continue;
                }

                Insert(&world.sectors, sectorId, { });
                WorldSector* sector = &world.sectors[sectorId];
                InitWorldSector(sector);
                Log("[world] opened sector table [%d,%d] for world %s", sectorId.x, sectorId.z, worldPath.data);

            }

            if (Equals(parts[0], "entity"))
            {
                bool success = true;
                
                EntityId entityId = 0;
                if (success) entityId = ParseU64(parts[1], &success);

                if (!success)
                {
                    LogError("[world] Failed to parse entity in world (%s)! Perhaps it is corrupt?", worldPath.data);
                    return false;
                }




                if (mode == Mode_Global)
                {
                    StringView actorPath = AppendPaths(GetPathDirectory(worldPath), TPrint("%lu", entityId));
                    Entity* entity = LoadEntity(TPrint("%s.actor", actorPath.data), &world);
                    Assert((entity->flags & Entity_Global) != 0);
                    
                    Log("[world] loaded global entity %lu for world %s", entityId, worldPath.data);
                    
                }
                else if (mode == Mode_Sector)
                {
                    Assert(HasKey(world.sectors, sectorHead));


                    WorldSector* sector = &world.sectors[sectorHead];

                    Assert(ArrayFind(&sector->entities, entityId) == -1);

                    ArrayAdd(&sector->entities, entityId);


                    Log("[world] sector table'd entity %lu for world %s", entityId, worldPath.data);
                }
                else
                {
                    LogWarn("[world] Attempted to add entity to neither 'global' or 'sector' buckets, is world file (%s) corrupt?", worldPath.data);
                }
            }

        }

        // Log("[entity] loaded entity %s for world %s", line.data, worldPath.data);
        // StringView actorPath = AppendPaths(GetPathDirectory(worldPath), line.data);
        // LoadEntity(TPrint("%s.actor", actorPath.data), &world);

    }

    world.path.Update(worldPath);
    *loadWorld = world;
    return true;
}



void LoadSector(World* world, WorldSectorId sectorId)
{
    Assert(HasKey(world->sectors, sectorId));

	WorldSector* sector = &world->sectors[sectorId];
    
    if (sector->loaded) return;

    sector->active = true;
    sector->loaded = true;

	For (sector->entities)
	{
		EntityId entityId = *it;

        // Ok, so basically what happens is that some entities will spawn subentities
        // which are then added to the sector, already in the world
        // so we need to make sure to not try to load those from disk!
		if (GetWorldEntity(world, entityId) != NULL) continue;

        Log("[entity] loaded sector entity %lu for world %s in sector [%d,%d]", entityId, world->path.data, sectorId.x, sectorId.z);
        StringView actorPath = AppendPaths(GetPathDirectory(world->path), TPrint("%lu.actor", entityId));
        LoadEntity(actorPath, world);
	}
}

void UnloadSector(World* world, WorldSectorId sectorId)
{
    Assert(HasKey(world->sectors, sectorId));

	WorldSector* sector = &world->sectors[sectorId];
    sector->active = false;
    sector->loaded = false;

	For (sector->entities)
	{
		EntityId entityId = *it;

        Entity* entity = GetWorldEntity(world, entityId);
        if (entity == NULL) continue;

        DeleteEntity(world, entity);
	}

    // DestroyWorldSector(sector);
    // Remove(&world->sectors, sectorId);
}

bool SaveWorld(StringView worldPath, World* world) {
    StringView folderPath = GetPathDirectory(worldPath);

    UpdateSectors(world);


    File file;
    if (Open(&file, worldPath, FILE_CREATE | FILE_WRITE | FILE_TRUNCATE))
    {
        {
            StringView strGlo = "global\n";
            Write(&file, strGlo.data, strGlo.length);
        }

        For (world->entityPools) {
            for (EntityPoolIt pit = EntityPoolBegin(*it); EntityPoolItValid(pit); pit = EntityPoolNext(pit)) {
                Entity* entity = GetEntityFromPoolIt(pit);

                // there are entities that arent in sectors so we need to make sure they're not runtime entities
                if ((entity->flags & Entity_Runtime) != 0) continue;

                // we want only the Global entities for the next step
                if ((entity->flags & Entity_Global) == 0) continue;

                if (!SaveEntity(folderPath, entity)) return false;

                String str = TPrint("entity %lu\n", entity->id);
                Write(&file, str.data, str.length);
            }
        }

        For (world->sectors.keys)
        {
            String strSec = TPrint("sector %d,%d\n", it->x, it->z);
            Write(&file, strSec.data, strSec.length);

            auto* sectorIt = &world->sectors[*it];
            ForIt (sectorIt->entities, entIt)
            {
                if (sectorIt->active)
                {
                    Entity* entity = GetWorldEntity(world, *entIt);
                    if (entity == NULL) continue;


                    AssertMsg((entity->flags & Entity_Global) == 0, "Not supposed to be any global entities here!");

                    if ((entity->flags & Entity_Runtime) != 0) continue; // Don't try to save runtime entities!

                    if (!SaveEntity(folderPath, entity)) return false;
                }

                String strEnt = TPrint("entity %lu\n", *entIt);
                Write(&file, strEnt.data, strEnt.length);
            }
        }

        SaveTerrain(world);

        Close(&file);
        return true;
    }

    // File file;
    // if (Open(&file, worldPath, FILE_CREATE | FILE_WRITE | FILE_TRUNCATE)) {
    //     For (world->entityPools) {
    //         for (EntityPoolIt pit = EntityPoolBegin(*it); EntityPoolItValid(pit); pit = EntityPoolNext(pit)) {
    //             Entity* entity = GetEntityFromPoolIt(pit);

    //             if ((entity->flags & Entity_Runtime) != 0) continue;

    //             if (!SaveEntity(folderPath, entity)) return false;

    //             String str = TPrint("%lu\n", entity->id);
    //             Write(&file, str.data, str.length);
    //         }
    //     }

    //     Close(&file);
    //     return true;
    // }


    return false;
}