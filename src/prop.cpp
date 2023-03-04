#include "prop.h"
#include "game.h"
#include "entityutil.h"
#include "resource.h"

void Prop::Init(World* world)
{
	// Init graphics
	{
		if (this->model)
			this->mesh = &this->model->mesh;
		else
			this->mesh = NULL;

		this->material = LoadMaterial(this->materialPath);
	}

	// Init physics
	{
		if (this->mesh && InitPhysics(this, world, Rigidbody_Static))
			AddConcaveMeshCollider(this->rigidbody, CreateXform(), this->mesh);
	}

}

void Prop::Destroy(World* world)
{
	// @@FreeingResources
}


void Prop::Start(World* world)
{
}

void Prop::Update(World* world)
{
}


RegisterEntity(Prop);