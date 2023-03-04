#pragma once
#include "world.h"
struct Zefir : public Entity {
	BEGIN_DATADESC(Zefir)
	END_DATADESC()

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};