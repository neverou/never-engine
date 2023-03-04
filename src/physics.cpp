#include "physics.h"
#include "game.h"

#include "allocators.h"


constexpr int Thread_Count = 1;

constexpr u32 Max_Query_Hits = 256;


// // ~Hack remove this asap, was only a test
// intern rp3d::Quaternion MatrixToQuat(Mat4 mat)
// {
// 	float m00 = mat.m[0];
// 	float m10 = mat.m[1];
// 	float m20 = mat.m[2];

// 	float m01 = mat.m[0 + 1 * 4];
// 	float m11 = mat.m[1 + 1 * 4];
// 	float m21 = mat.m[2 + 1 * 4];

// 	float m02 = mat.m[0 + 2 * 4];
// 	float m12 = mat.m[1 + 2 * 4];
// 	float m22 = mat.m[2 + 2 * 4];

// 	rp3d::Quaternion q;
// 	float t = 0;
// 	if (m22 < 0) {
// 		if (m00 >m11) {
// 			t = 1 + m00 -m11 -m22;
// 			q = rp3d::Quaternion( t, m01+m10, m20+m02, m12-m21 );
// 		}
// 		else {
// 			t = 1 -m00 + m11 -m22;
// 			q = rp3d::Quaternion( m01+m10, t, m12+m21, m20-m02 );
// 		}
// 	}
// 	else {
// 		if (m00 < -m11) {
// 			t = 1 -m00 -m11 + m22;
// 			q = rp3d::Quaternion( m20+m02, m12+m21, t, m01-m10 );
// 		}
// 		else {
// 			t = 1 + m00 + m11 + m22;
// 			q = rp3d::Quaternion( m12-m21, m20-m02, m01-m10, t );
// 		}
// 	}
// 	q.x *= 0.5F / sqrtf(t);
// 	q.y *= 0.5F / sqrtf(t);
// 	q.z *= 0.5F / sqrtf(t);
// 	q.w *= 0.5F / sqrtf(t);
// 	return q.getUnit();
// }

// intern Xform FromPhysTransform(rp3d::Transform t) { return Xform { FromPhysConvVec3(t.getPosition()), quatToMatrix(t.getOrientation()), v3(1) }; } // ~Incomplete rotation
// intern rp3d::Transform ToPhysTransform(Xform t) { return rp3d::Transform(ToPhysConvVec3(t.position), MatrixToQuat(t.rotation)); } // ~Incomplete rotation 

// intern rp3d::PhysicsCommon physicsCommon;


#ifdef BUILD_DEBUG
#define _DEBUG
#else
#define NDEBUG
#endif

#include "physx/PxPhysicsAPI.h"
#include "physx/extensions/PxDefaultSimulationFilterShader.h"
using namespace physx;



intern PxMat44 ToPhysMat4(Mat4 matrix) { return PxMat44(Transpose(matrix).m); }

intern Vec3 FromPhysVec3(PxVec3 v) { return v3(v.x, v.y, v.z); }
intern PxVec3 ToPhysVec3(Vec3 v) { return PxVec3(v.x, v.y, v.z); }




// // ~Hack remove this asap, was only a test
intern Mat4 quatToMatrix(PxQuat q)
{
    float sqw = q.w*q.w;
    float sqx = q.x*q.x;
    float sqy = q.y*q.y;
    float sqz = q.z*q.z;

	float m00, m10, m20,
		  m01, m11, m21,
		  m02, m12, m22;

    // invs (inverse square length) is only required if quaternion is not already normalised
    float invs = 1 / (sqx + sqy + sqz + sqw);
    m00 = ( sqx - sqy - sqz + sqw)*invs ; // since sqw + sqx + sqy + sqz =1/invs*invs
    m11 = (-sqx + sqy - sqz + sqw)*invs ;
    m22 = (-sqx - sqy + sqz + sqw)*invs ;
    
    float tmp1 = q.x*q.y;
    float tmp2 = q.z*q.w;
    m10 = 2.0 * (tmp1 + tmp2)*invs ;
    m01 = 2.0 * (tmp1 - tmp2)*invs ;
    
    tmp1 = q.x*q.z;
    tmp2 = q.y*q.w;
    m20 = 2.0 * (tmp1 - tmp2)*invs ;
    m02 = 2.0 * (tmp1 + tmp2)*invs ;
    tmp1 = q.y*q.z;
    tmp2 = q.x*q.w;
    m21 = 2.0 * (tmp1 + tmp2)*invs ;
    m12 = 2.0 * (tmp1 - tmp2)*invs ;

	return Transpose(CreateMatrix(
		m00, m10, m20, 0,
		m01, m11, m21, 0,
		m02, m12, m22, 0,
		0,   0,   0,   1
	));
}


intern Xform FromPhysTransform(PxTransform transform) { 
	Xform xform = CreateXform();
	xform.position = FromPhysVec3(transform.p);
	xform.rotation = quatToMatrix(transform.q);
	return xform;
}

intern PxTransform ToPhysTransform(Xform xform) { 
	return PxTransform(ToPhysMat4(XformMatrix(xform)));
}

intern PxDefaultErrorCallback physxDefaultErrorCallback;
intern PxDefaultAllocator 	physxDefaultAllocatorCallback;
intern PxPvd* 				physxPvd;

intern PxFoundation* physxFoundation;
intern PxPhysics* physics;
intern PxSimulationFilterShader physxFilterShader = PxDefaultSimulationFilterShader;
intern PxCooking* cooking;

intern PxDefaultCpuDispatcher* cpuDispatcher;

void InitPhysics()
{
	physxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, physxDefaultAllocatorCallback, physxDefaultErrorCallback);
	if(!physxFoundation)
   		FatalError("PxCreateFoundation failed!");

	bool recordMemoryAllocations = true;

	physxPvd = PxCreatePvd(*physxFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	physxPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *physxFoundation, PxTolerancesScale(), recordMemoryAllocations, physxPvd);
	if(!physics)
		FatalError("PxCreatePhysics failed!");


	///////////////////////////////////////////////////////////////

	cpuDispatcher = PxDefaultCpuDispatcherCreate(Thread_Count);
	if(!cpuDispatcher)
		FatalError("PxDefaultCpuDispatcherCreate failed!");

	/// cooking

	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *physxFoundation, PxCookingParams(physics->getTolerancesScale()));
	if (!cooking)
		FatalError("PxCreateCooking failed!");
}

void DestroyPhysics()
{
	
}


static PxMaterial* tmpMaterial;

void InitPhysicsWorld(PhysicsWorld* world)
{
	world->rigidbodies = MakeBucketArray<64, Rigidbody>();

	PxScene* scene;

	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);

	if(!sceneDesc.cpuDispatcher)
		sceneDesc.cpuDispatcher = cpuDispatcher;

	if(!sceneDesc.filterShader)
		sceneDesc.filterShader = physxFilterShader;

	// #ifdef PLATFORM_WINDOWS // is this windows only??
	// if(!sceneDesc.gpuDispatcher && mCudaContextManager)
	// {
	// 	sceneDesc.gpuDispatcher = mCudaContextManager->getGpuDispatcher();
	// }
	// #endif

	scene = physics->createScene(sceneDesc);
	if (!scene)
		FatalError("PhysX CreateScene failed!");

	// scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
	// scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);

	world->scene = scene;

	// ~Temp
	tmpMaterial = physics->createMaterial(0.5,0.5,0.0);
}

void DestroyPhysicsWorld(PhysicsWorld* world)
{
	FreeBucketArray(&world->rigidbodies);
	world->scene->release();
}

void UpdatePhysicsWorld(PhysicsWorld* world, float dt)
{
    world->scene->simulate(dt);
	world->scene->fetchResults(true);
}


Rigidbody* MakeRigidbody(PhysicsWorld* world, Xform xform, RigidbodyType type)
{
	Rigidbody rigidbody = {};
	rigidbody.type = type;

	if (!Equal(xform.scale, v3(1)))
	{
		LogWarn("[physics] Cannot create rigidbody with non-identity scale! (Scale must be 1, 1, 1)");
		return NULL;
	}

	auto transform = PxTransform(ToPhysMat4(XformMatrix(xform)));

	switch (type)
	{
		case Rigidbody_Static: 
		{
			rigidbody.body = physics->createRigidStatic(transform);			
			break;
		}
		case Rigidbody_Dynamic: 
		{
			rigidbody.body = physics->createRigidDynamic(transform);
			break;
		}
		default: Assert(false);
	}

	world->scene->addActor(*rigidbody.body);

	return BucketArrayAdd(&world->rigidbodies, rigidbody);
}

void DestroyRigidbody(PhysicsWorld* world, Rigidbody* rigidbody)
{
	world->scene->removeActor(*rigidbody->body);
	rigidbody->body->release();
	BucketArrayRemove(&world->rigidbodies, rigidbody);
}

void SetXform(Rigidbody* rigidbody, Xform xform)
{
	rigidbody->body->setGlobalPose(ToPhysTransform(xform));
}

Xform GetXform(Rigidbody* rigidbody)
{
	PxTransform pose = rigidbody->body->getGlobalPose();
	return FromPhysTransform(pose);
}

void SetVelocity(Rigidbody* rigidbody, Vec3 velocity)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	((PxRigidDynamic*)rigidbody->body)->setLinearVelocity(ToPhysVec3(velocity));	
}

Vec3 GetVelocity(Rigidbody* rigidbody)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	return FromPhysVec3(((PxRigidDynamic*)rigidbody->body)->getLinearVelocity()); 
}

void SetAngularVelocity(Rigidbody* rigidbody, Vec3 velocity)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	((PxRigidDynamic*)rigidbody->body)->setAngularVelocity(ToPhysVec3(velocity));
}

Vec3 GetAngularVelocity(Rigidbody* rigidbody)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	return FromPhysVec3(((PxRigidDynamic*)rigidbody->body)->getAngularVelocity()); 
}

void SetKinematic(Rigidbody* rigidbody, bool kinematic)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	((PxRigidDynamic*)rigidbody->body)->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
}

bool GetKinematic(Rigidbody* rigidbody)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	return ( u32(((PxRigidDynamic*)rigidbody->body)->getRigidBodyFlags()) & PxRigidBodyFlag::eKINEMATIC ) != 0;
}

void SetAxisLock(Rigidbody* rigidbody, int flags)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	((PxRigidDynamic*)rigidbody->body)->setRigidDynamicLockFlags((PxRigidDynamicLockFlag::Enum)flags);
}

int GetAxisLock(Rigidbody* rigidbody)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	return (RigidbodyAxisLockFlags)((u32)((PxRigidDynamic*)rigidbody->body)->getRigidDynamicLockFlags());
}

//

void SetGravity(PhysicsWorld* world, Vec3 gravity)
{
	world->scene->setGravity(ToPhysVec3(gravity));
}

Vec3 GetGravity(PhysicsWorld* world)
{
	return FromPhysVec3(world->scene->getGravity());
}

//

void AddForce(Rigidbody* rigidbody, Vec3 force, ForceMode mode)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	PxForceMode::Enum forceMode;
	
	if (mode == Force_Continuous)     forceMode = PxForceMode::Enum::eFORCE;
	if (mode == Force_Impulse)        forceMode = PxForceMode::Enum::eIMPULSE;
	if (mode == Force_VelocityChange) forceMode = PxForceMode::Enum::eVELOCITY_CHANGE;
	if (mode == Force_Acceleration)   forceMode = PxForceMode::Enum::eACCELERATION;

	((PxRigidDynamic*)rigidbody->body)->addForce(ToPhysVec3(force), forceMode);
}


void AddTorque(Rigidbody* rigidbody, Vec3 torque, ForceMode mode)
{
	Assert(rigidbody->type == Rigidbody_Dynamic);
	PxForceMode::Enum forceMode;
	
	if (mode == Force_Continuous)     forceMode = PxForceMode::Enum::eFORCE;
	if (mode == Force_Impulse)        forceMode = PxForceMode::Enum::eIMPULSE;
	if (mode == Force_VelocityChange) forceMode = PxForceMode::Enum::eVELOCITY_CHANGE;
	if (mode == Force_Acceleration)   forceMode = PxForceMode::Enum::eACCELERATION;

	((PxRigidDynamic*)rigidbody->body)->addTorque(ToPhysVec3(torque), forceMode);
}

////


// ~Todo: multiple return queries

// ~Refactor compress all the queries
bool CastRay(PhysicsWorld* world, Vec3 o, Vec3 d, float maxDist, RayHit* hitPtr)
{
	PxVec3 origin = ToPhysVec3(o);
	PxVec3 unitDir = ToPhysVec3(Normalize(d));
	PxRaycastBuffer physHit;

	RayHit hit { };

	// Raycast against all static & dynamic objects (no filtering)
	// The main result from this call is the closest hit, stored in the 'hit.block' structure
	bool status = world->scene->raycast(origin, unitDir, maxDist, physHit);
	if (status)
	{
		hit.position = FromPhysVec3(physHit.block.position);
		hit.normal   = FromPhysVec3(physHit.block.normal);
		hit.distance = physHit.block.distance;

		// lookup the rigidbody
		Rigidbody* body = NULL;

		// ~Refactor make this (monstrosity) a macro
		for (auto it = BucketArrayBegin(&world->rigidbodies); BucketIteratorValid(it); it = BucketIteratorNext(it))
		{
			Rigidbody* rb = GetBucketIterator(it);

			if (rb->body == physHit.block.actor)
			{
				body = rb;
				break;
			}
		}

		Assert(body != NULL);
		hit.rigidbody = body;
	}

	*hitPtr = hit;

	return status;
}


bool SweepBox(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, Vec3 halfExtents, float maxDist, RayHit* hitPtr)
{
	PxSweepBuffer hit;
	PxBoxGeometry sweepShape = PxBoxGeometry(ToPhysVec3(halfExtents));
	
	Xform initialPose = pose;
	initialPose.position = initialPose.position + o;
	bool status = world->scene->sweep(sweepShape, ToPhysTransform(initialPose), ToPhysVec3(d), maxDist, hit);

	if (status)
	{
		hitPtr->position = FromPhysVec3(hit.block.position);
		hitPtr->normal   = FromPhysVec3(hit.block.normal);
		hitPtr->distance = hit.block.distance;
		if (hitPtr->distance == 0)
			hitPtr->position = o;

		// lookup the rigidbody
		Rigidbody* body = NULL;

		// ~Refactor make this (monstrosity) a macro
		for (auto it = BucketArrayBegin(&world->rigidbodies); BucketIteratorValid(it); it = BucketIteratorNext(it))
		{
			Rigidbody* rb = GetBucketIterator(it);

			if (rb->body == hit.block.actor)
			{
				body = rb;
				break;
			}
		}

		Assert(body != NULL);
		hitPtr->rigidbody = body;
	}

	return status;
}


bool SweepCapsule(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, float radius, float halfHeight, float maxDist, RayHit* hitPtr)
{
	PxSweepBuffer hit;
	PxCapsuleGeometry sweepShape = PxCapsuleGeometry(radius, halfHeight - radius);

	Xform initialPose = pose;
	initialPose.position = initialPose.position + o;
	initialPose.rotation = Mul(initialPose.rotation, RotationMatrixAxisAngle(v3(0,0,1), 90));
	bool status = world->scene->sweep(sweepShape, ToPhysTransform(initialPose), ToPhysVec3(d), maxDist, hit);

	if (status)
	{
		hitPtr->position = FromPhysVec3(hit.block.position);
		hitPtr->normal   = FromPhysVec3(hit.block.normal);
		hitPtr->distance = hit.block.distance;
		if (hitPtr->distance == 0)
			hitPtr->position = o;

		// lookup the rigidbody
		Rigidbody* body = NULL;

		// ~Refactor make this (monstrosity) a macro
		for (auto it = BucketArrayBegin(&world->rigidbodies); BucketIteratorValid(it); it = BucketIteratorNext(it))
		{
			Rigidbody* rb = GetBucketIterator(it);

			if (rb->body == hit.block.actor)
			{
				body = rb;
				break;
			}
		}

		Assert(body != NULL);
		hitPtr->rigidbody = body;
	}

	return status;
}



bool SweepSphere(PhysicsWorld* world, Vec3 o, Vec3 d, Xform pose, float radius, float maxDist, RayHit* hitPtr)
{
	PxSweepBuffer hit;
	PxSphereGeometry sweepShape = PxSphereGeometry(radius);
	
	Xform initialPose = pose;
	initialPose.position = initialPose.position + o;
	bool status = world->scene->sweep(sweepShape, ToPhysTransform(initialPose), ToPhysVec3(d), maxDist, hit);

	if (status)
	{
		hitPtr->position = FromPhysVec3(hit.block.position);
		hitPtr->normal   = FromPhysVec3(hit.block.normal);
		hitPtr->distance = hit.block.distance;
		if (hitPtr->distance == 0)
			hitPtr->position = o;

		// lookup the rigidbody
		Rigidbody* body = NULL;

		// ~Refactor make this (monstrosity) a macro
		for (auto it = BucketArrayBegin(&world->rigidbodies); BucketIteratorValid(it); it = BucketIteratorNext(it))
		{
			Rigidbody* rb = GetBucketIterator(it);

			if (rb->body == hit.block.actor)
			{
				body = rb;
				break;
			}
		}

		Assert(body != NULL);
		hitPtr->rigidbody = body;
	}

	return status;
}




Rigidbody* CheckBox(PhysicsWorld* world, Xform pose, Vec3 halfExtents)
{
	PxBoxGeometry overlapShape = PxBoxGeometry(ToPhysVec3(halfExtents));
	PxTransform shapePose = ToPhysTransform(pose);

	FixArray<PxOverlapHit, Max_Query_Hits> hitBuffer;

	PxOverlapBuffer hit(hitBuffer.data, hitBuffer.size);
	bool status = world->scene->overlap(overlapShape, shapePose, hit);

	if (!status)
		return NULL;

	PxOverlapHit actualHit = hitBuffer.data[0];
	
	// lookup the rigidbody
	Rigidbody* body = NULL;
	for (auto it = BucketArrayBegin(&world->rigidbodies); BucketIteratorValid(it); it = BucketIteratorNext(it))
	{
		Rigidbody* rb = GetBucketIterator(it);

		if (rb->body == actualHit.actor)
		{
			body = rb;
			break;
		}
	}

	Assert(body != NULL);
	return body;
}


Rigidbody* CheckSphere(PhysicsWorld* world, Xform pose, float radius)
{
	PxSphereGeometry overlapShape = PxSphereGeometry(radius);
	PxTransform shapePose = ToPhysTransform(pose);

	FixArray<PxOverlapHit, Max_Query_Hits> hitBuffer;

	PxOverlapBuffer hit(hitBuffer.data, hitBuffer.size);
	bool status = world->scene->overlap(overlapShape, shapePose, hit);

	if (!status)
		return NULL;

	PxOverlapHit actualHit = hitBuffer.data[0];

	// lookup the rigidbody
	Rigidbody* body = NULL;

	// ~Refactor make this (monstrosity) a macro
	for (auto it = BucketArrayBegin(&world->rigidbodies); BucketIteratorValid(it); it = BucketIteratorNext(it))
	{
		Rigidbody* rb = GetBucketIterator(it);

		if (rb->body == actualHit.actor)
		{
			body = rb;
			break;
		}
	}

	Assert(body != NULL);
	return body;
}


////

// ~Todo return the shape so that the actor can modify it
void AddBoxCollider(Rigidbody* rigidbody, Xform relativeXform, Vec3 halfExtents)
{
	PxShape* shape = PxRigidActorExt::createExclusiveShape(*rigidbody->body, PxBoxGeometry(ToPhysVec3(halfExtents)), *tmpMaterial); // ~Todo material
	shape->setLocalPose(ToPhysTransform(relativeXform));
}


void AddSphereCollider(Rigidbody* rigidbody, Xform relativeXform, float radius)
{
	PxShape* shape = PxRigidActorExt::createExclusiveShape(*rigidbody->body, PxSphereGeometry(radius), *tmpMaterial); // ~Todo material
	shape->setLocalPose(ToPhysTransform(relativeXform));
}



void AddConcaveMeshCollider(Rigidbody* rigidbody, Xform relativeXform, TriangleMesh* mesh)
{

	// @@Release somehow make this a compile step so we cook the collision meshes in advance

	PxTolerancesScale scale;
	PxCookingParams params(scale);
	// disable mesh cleaning - perform mesh validation on development configurations
	// params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
	// disable edge precompute, edges are set for each triangle, slows contact generation
	params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
	// lower hierarchy for intern mesh
	// params.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;

	cooking->setParams(params);

	SArray<PxVec3> vertices = MakeArray<PxVec3>(mesh->vertices.size, Frame_Arena);
	{
		int i = 0;
		For (mesh->vertices)
		{
			vertices[i++] = ToPhysVec3(it->position);
		}
	}

	PxTriangleMeshDesc meshDesc = {};
	meshDesc.flags = PxMeshFlags();
	meshDesc.points.count           = vertices.size;
	meshDesc.points.stride          = sizeof(PxVec3);
	meshDesc.points.data            = vertices.data;

	SArray<PxU32> indices = MakeArray<PxU32>(mesh->vertices.size, Frame_Arena);
	{
		int i = 0;
		For (mesh->vertices) // ~Todo make this work with index buffer @@MeshIndices
		{
			indices[i] = i;
			i++;
		}
	}
	meshDesc.triangles.count        = indices.size / 3;
	meshDesc.triangles.stride       = 3 * sizeof(PxU32);
	meshDesc.triangles.data         = indices.data;

	// mesh should be validated before cooked without the mesh cleaning
	//bool res = cooking->validateTriangleMesh(meshDesc);
	//Assert(res);
	
	PxTriangleMesh* aTriangleMesh = cooking->createTriangleMesh(meshDesc, physics->getPhysicsInsertionCallback());

	PxRigidActorExt::createExclusiveShape(*rigidbody->body, PxTriangleMeshGeometry(aTriangleMesh), *tmpMaterial); // ~Todo material
}



void AddConvexMeshCollider(Rigidbody* rigidbody, Xform relativeXform, TriangleMesh* mesh)
{
	Assert(false); // This function doesn't do anything lol
	
	// ~FixMe we need to do convex hull generations ourself, ugh
	// ~FixMe memory management

	// Vec3* vertices = (Vec3*)Alloc(sizeof(Vec3) * mesh->vertices.size);
	// u32 vertCount = 0;
	// u32 indexCount = mesh->vertices.size;
	
	// u32* indices = (u32*)Alloc(sizeof(u32) * mesh->vertices.size);

	
	// for (u32 i = 0; i < mesh->vertices.size; i++)
	// {
	// 	Vec3 vert = mesh->vertices[i].position;

	// 	bool found = false;
	// 	u32 idx = 0;
	// 	for (u32 j = 0; j < vertCount; j++)
	// 	{
	// 		if (Equal(vertices[j], vert))
	// 		{
	// 			found = true;
	// 			idx = j;
	// 			break;
	// 		}
	// 	}
	// 	if (found)
	// 	{
	// 		indices[i] = idx;
	// 	} 
	// 	else
	// 	{
	// 		vertices[vertCount] = vert;
	// 		indices[i] = vertCount;
	// 		vertCount++;
	// 	}
	// }

	// // Description of the six faces of the convex mesh 
	// auto* polygonFaces = new rp3d::PolygonVertexArray::PolygonFace[indexCount / 3]; 
	// rp3d::PolygonVertexArray::PolygonFace* face = polygonFaces; 
	// for (int f = 0; f < indexCount / 3; f++) { 
	// 	face->indexBase = f * 3; 
	// 	face->nbVertices = 3; 
	// 	face++; 
	// }

	// // PolygonVertexArray::PolygonVertexArray 	( 	uint  	nbVertices,
	// // 	const void *  	verticesStart,
	// // 	int  	verticesStride,
	// // 	const void *  	indexesStart,
	// // 	int  	indexesStride,
	// // 	uint  	nbFaces,
	// // 	PolygonFace *  	facesStart,
	// // 	VertexDataType  	vertexDataType,
	// // 	IndexDataType  	indexDataType 
	// // )
	
	// // Create the polygon vertex array 
	// rp3d::PolygonVertexArray *polygonVertexArray = new rp3d::PolygonVertexArray(vertCount, vertices, sizeof(Vec3), 
	// indices, sizeof(u32), indexCount / 3, polygonFaces, 
	// rp3d::PolygonVertexArray::VertexDataType::VERTEX_FLOAT_TYPE, 
	// rp3d::PolygonVertexArray::IndexDataType::INDEX_INTEGER_TYPE); 
	
	// // Create the polyhedron mesh 
	// rp3d::PolyhedronMesh* polyhedronMesh = physicsCommon.createPolyhedronMesh(polygonVertexArray); 
	
	// // Create the convex mesh collision shape 
	// rp3d::ConvexMeshShape* convexMesh = physicsCommon.createConvexMeshShape(polyhedronMesh);

	// rp3d::Transform transform = ToPhysTransform(relativeXform); 
	// rp3d::Collider* collider = rigidbody->body->addCollider(convexMesh, transform);
}


#include "gizmos.h"

void DrawPhysicsDebug(PhysicsWorld* physicsWorld)
{
	// const PxRenderBuffer& rb = physicsWorld->scene->getRenderBuffer();
	// Log("l: %d, t: %d, p: %d", rb.getNbLines(), rb.getNbTriangles(), rb.getNbPoints());
	// for(PxU32 i=0; i < rb.getNbLines(); i++)
	// {
	// 	if (i > 50) break;
	// 	const PxDebugLine& line = rb.getLines()[i];
	// 	Vec4 color0 = v4( (line.color0 & 0xFF) / 255.0, ((line.color0 << 8) & 0xFF) / 255.0, ((line.color0 << 16) & 0xFF) / 255.0, ((line.color0 << 24) & 0xFF) / 255.0 );
	// 	Vec4 color1 = v4( (line.color1 & 0xFF) / 255.0, ((line.color1 << 8) & 0xFF) / 255.0, ((line.color1 << 16) & 0xFF) / 255.0, ((line.color1 << 24) & 0xFF) / 255.0 );

	// 	Vec3 tmpTan = v3(0, 1, 0);
	// 	Vec3 lineDir = Normalize(FromPhysVec3(line.pos1) - FromPhysVec3(line.pos0));
	// 	if (Dot(lineDir, tmpTan) > 0.5)
	// 		tmpTan = v3(1, 0, 0);
	// 	Vec3 tangent = Normalize(Cross(tmpTan, lineDir));
	// 	Vec3 bitangent = Cross(lineDir, tangent);

	// 	float scale = 0.1 * Length(FromPhysVec3(line.pos1) - FromPhysVec3(line.pos0));

	// 	// GizmoCube(FromPhysVec3(line.pos0), v3(1), v3(color0) );
	// 	// GizmoCube(FromPhysVec3(line.pos1), v3(1), v3(color1) );

	// 	PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos0) - tangent * v3(scale), v3(color0) });
	// 	PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos1) - tangent * v3(scale), v3(color1) });
	// 	PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos1) + tangent * v3(scale), v3(color1) });

	// 	PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos0) - tangent * v3(scale), v3(color0) });
	// 	PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos1) + tangent * v3(scale), v3(color1) });
	// 	PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos0) + tangent * v3(scale), v3(color0) });

	// 	// PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos0) - bitangent * v3(scale), v3(color0) });
	// 	// PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos1) - bitangent * v3(scale), v3(color1) });
	// 	// PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos1) + bitangent * v3(scale), v3(color1) });
	// 	// PushGizmoVertex(GizmoVertex { FromPhysVec3(line.pos0) + bitangent * v3(scale), v3(color0) });
	// }
}
