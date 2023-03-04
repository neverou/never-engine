#pragma once

#include "std.h"
#include "maths.h"
#include "mesh.h"


const float Physics_Timestep = 1.0 / 200.0;

struct World;
struct Entity;

namespace physx { 
	class PxScene;
	class PxRigidActor;
}


enum RigidbodyType
{
	Rigidbody_None = 0,
	Rigidbody_Static,
	Rigidbody_Dynamic,
};

struct Rigidbody
{
	RigidbodyType type;
	
	physx::PxRigidActor* body;
	Entity* entity;
};


struct PhysicsWorld
{
	BucketArray<64, Rigidbody> rigidbodies;

	physx::PxScene* scene; 
};



void InitPhysics();
void DestroyPhysics();

void InitPhysicsWorld(PhysicsWorld* world);
void DestroyPhysicsWorld(PhysicsWorld* world);
void UpdatePhysicsWorld(PhysicsWorld* world, float dt);

Rigidbody* MakeRigidbody(PhysicsWorld* world, Xform xform, RigidbodyType type);
void DestroyRigidbody(PhysicsWorld* world, Rigidbody* rigidbody);

// rigidbody state
void SetXform(Rigidbody* rigidbody, Xform xform);
Xform GetXform(Rigidbody* rigidbody);

void SetVelocity(Rigidbody* rigidbody, Vec3 velocity);
Vec3 GetVelocity(Rigidbody* rigidbody);

void SetAngularVelocity(Rigidbody* rigidbody, Vec3 velocity);
Vec3 GetAngularVelocity(Rigidbody* rigidbody);

void SetKinematic(Rigidbody* rigidbody, bool kinematic);
bool GetKinematic(Rigidbody* rigidbody);


enum RigidbodyAxisLockFlags
{
	AxisLock_TranslateX = (1 << 0),
	AxisLock_TranslateY = (1 << 1),
	AxisLock_TranslateZ = (1 << 2),
	AxisLock_RotateX    = (1 << 3),
	AxisLock_RotateY    = (1 << 4),
	AxisLock_RotateZ    = (1 << 5),
};
void SetAxisLock(Rigidbody* rigidbody, int flags);
int GetAxisLock(Rigidbody* rigidbody);

// world state
void SetGravity(PhysicsWorld* world, Vec3 gravity);
Vec3 GetGravity(PhysicsWorld* world);

enum ForceMode
{
	Force_Impulse,
	Force_Continuous,
	Force_Acceleration,
	Force_VelocityChange,
};

void AddForce(Rigidbody* rigidbody, Vec3 force, ForceMode mode);
// ~Todo AddForceAtPosition
// ~Todo AddForceAtLocalPosition

void AddTorque(Rigidbody* rigidbody, Vec3 torque, ForceMode mode);

// queries
struct RayHit
{
	Vec3 position;
	Vec3 normal;
	float distance;

	Rigidbody* rigidbody;
};

bool CastRay(PhysicsWorld* world, Vec3 o, Vec3 d, float maxDist, RayHit* hit);

bool SweepBox(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, Vec3 halfExtents, float maxDist, RayHit* hit);
bool SweepCapsule(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, float radius, float halfHeight, float maxDist, RayHit* hit);
bool SweepSphere(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, float radius, float maxDist, RayHit* hit);

Rigidbody* CheckBox(PhysicsWorld* world, Xform pose, Vec3 halfExtents);
Rigidbody* CheckCapsule(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, float radius, float halfHeight);
Rigidbody* CheckSphere(PhysicsWorld* world, Xform pose, float radius);

// colliders
void AddBoxCollider(Rigidbody* rigidbody, Xform relativeXform, Vec3 halfExtents);
void AddSphereCollider(Rigidbody* rigidbody, Xform relativeXform, float radius);
void AddConcaveMeshCollider(Rigidbody* rigidbody, Xform relativeXform, TriangleMesh* mesh);
void AddConvexMeshCollider(Rigidbody* rigidbody, Xform relativeXform, TriangleMesh* mesh);

// debug
void DrawPhysicsDebug(PhysicsWorld* physicsWorld);