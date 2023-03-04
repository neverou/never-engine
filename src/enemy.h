#pragma once
#include "resource.h"
#include "world.h"
#include "physics.h"

struct Enemy : public Entity {
	BEGIN_DATADESC(Enemy)
	DEFINE_FIELD(FIELD_FLOAT, speed)
	DEFINE_FIELD(Field_Model, model)
	DEFINE_FIELD(Field_Entity, target)
	END_DATADESC()

	ModelResource* model;
	EntityId target;
	float speed;

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};
