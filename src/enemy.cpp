#include "enemy.h"
#include "game.h"
#include "resource.h"
#include "entityutil.h"

void Enemy::Init(World* world)
{
	this->material = LoadMaterial("materials/prop_test.mat");
	if (this->model) this->mesh = &this->model->mesh;

	InitPhysics(this, world, Rigidbody_Dynamic);

	if(this->mesh) AddSphereCollider(this->rigidbody, CreateXform(), 1);
	SetAxisLock(this->rigidbody, AxisLock_RotateX | AxisLock_RotateY | AxisLock_RotateZ);

	Log("Enemy::Init()");
}

void Enemy::Start(World* world)
{
	Log("Enemy::Start()");
}

void Enemy::Update(World* world)
{
	Entity* target = GetWorldEntity(world, this->target);
	if (target)
	{
		Vec3 direction = Normalize(target->xform.position - this->xform.position);
		SetVelocity(this->rigidbody, v3(this->speed) * direction * v3(deltaTime));
	}
}

void Enemy::Destroy(World* world)
{

}

RegisterEntity(Enemy);