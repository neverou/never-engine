#include "zefir.h"
#include "game.h"
#include "resource.h"
#include "entityutil.h"

#include "logger.h"

void Zefir::Init(World* world)
{
	this->mesh = &LoadModel("models/zefir.smd")->mesh;
	this->material = LoadMaterial("models/prop_test.mat");

	InitPhysics(this, world, Rigidbody_Dynamic);
	AddBoxCollider(this->rigidbody, CreateXform(), v3(0.5, 0.5, 0.5));	
	SetAxisLock(this->rigidbody, AxisLock_RotateX | AxisLock_RotateY | AxisLock_RotateZ);
}

void Zefir::Destroy(World* world)
{

}


void Zefir::Start(World* world)
{

}

void Zefir::Update(World* world)
{

	Player* player = (Player*)GetWorldEntity(world, world->gameManager.playerId);
	if (player != NULL)
	{
		// ~Todo use world space for the positions @ActorXformUtil
		AddForce(this->rigidbody, (player->xform.position - this->xform.position) * v3(deltaTime * 5), Force_Impulse);
	}
}

RegisterEntity(Zefir);