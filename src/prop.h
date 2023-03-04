#pragma once

#include "world.h"

#include "resource.h"
#include "physics.h"

struct Prop : public Entity {
	BEGIN_DATADESC(Prop)
	DEFINE_FIELD(Field_Model, model)
	DEFINE_FIELD(FIELD_STRING, materialPath)
	END_DATADESC()

	ModelResource* model;

	String materialPath;

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};