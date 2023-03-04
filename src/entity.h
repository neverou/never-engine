#pragma once

#include "std.h"
#include "mesh.h"
#include "util.h"
#include "str.h"

#include "maths.h"
#include "material.h"

#include "physics.h"
#include "anim.h"

#include "character_controller.h"

typedef u64 EntityId;
typedef u64 EntityFlags;
typedef u64 EntityFeatures;
typedef u32 EntityTypeId;

enum
{
    Entity_IsStatic 	= 0x1, // Entity is stationary in the world and doesn't change graphically or physically.
	Entity_Global       = 0x2, // This means the Entity will not get unloaded due to distance
	Entity_Runtime      = 0x4, // Entity will not be saved to disk
};

enum DataDescType
{
	FIELD_NONE = 0,
	FIELD_INT,
	FIELD_STRING,
	FIELD_FLOAT,
	FIELD_VECTOR2,
	FIELD_VECTOR3,
	FIELD_VECTOR4,
	Field_Model,
	Field_Entity,
	Field_Material,
};

struct DataDesc
{
	char fieldName[64];
	DataDescType type;
	u64 offset;
};

struct EntityFrustumCullingData
{
	bool init;
	Vec3 origin;
	float radius;
};

enum
{
    EntityFeature_Rigidbody 			= 0x1,
	EntityFeature_Animator  			= 0x2,
	EntityFeature_Combat    			= 0x4,
	EntityFeature_CharacterController 	= 0x8,
};


struct CombatInfo
{
	int maxHealth;

	int health;
};


struct Entity
{
	String name;

	EntityId id;
	EntityFlags typeId;
	EntityFlags flags;

	Xform xform;
	EntityId parent;

	// Runtime info
	TriangleMesh* mesh;
	MaterialId    material;

	//   -> Features
	EntityFeatures features;
	Animator       		animator;
	Rigidbody*     		rigidbody;
	CombatInfo     		combatInfo;
	CharacterController characterController;

	// -> Misc
	EntityFrustumCullingData frustumCulling;
	bool started;

	inline virtual DataDesc* GetDataLayout() { return NULL; }

	virtual void Init(World* world) = 0;   // Init is called when the entity is created; no garuntee the rest of the world exists
	virtual void Start(World* world) = 0;  // Start is called when the entity is created and the world is ready (eg. on the first update cycle)
	virtual void Update(World* world) = 0;
	virtual void Destroy(World* world) = 0;
};

// ~Refactor create tools to iterate the data layout vs the while loop

struct World;

bool InitPhysics(Entity* entity, World* world, RigidbodyType bodyType);
bool InitAnimation(Entity* entity);


// voidless's weird macro magic :p
// Used for registering all the types of entities w/ the game engine
// so it knows what types of entities there are at runtime
// (This is because C++ doesnt have any introspection ability so theres no way for the code to figure out on its own)
#define BEGIN_DATADESC(typename) inline DataDesc* GetDataLayout() override { typedef typename typenameTypedef; DataDesc dataDesc[] = {
#define END_DATADESC() { "", FIELD_NONE, 0 } }; DataDesc* dataDescTmp = (DataDesc*)TempAlloc(sizeof(dataDesc)); Memcpy(dataDescTmp, dataDesc, sizeof(dataDesc)); return dataDescTmp; }
#define DEFINE_FIELD(type, field) { #field, type, Offset(typenameTypedef, field) },
