#pragma once

#include "world.h"

#include "resource.h"

enum PlayerState {
	PlayerState_Ground,
	PlayerState_Air,
};

struct Player : public Entity
{
	PlayerState state;
	BEGIN_DATADESC(Player)
	DEFINE_FIELD(FIELD_FLOAT, speed)
	DEFINE_FIELD(FIELD_FLOAT, sprintSpeed)
	DEFINE_FIELD(FIELD_FLOAT, jumpSpeed)
	// DEFINE_FIELD(Field_Entity, camera)
	DEFINE_FIELD(FIELD_VECTOR2, rotation)
	DEFINE_FIELD(FIELD_FLOAT, acceleration)
	DEFINE_FIELD(FIELD_FLOAT, airAcceleration)
	END_DATADESC()

	EntityId camera;
	Vec2 rotation;

	EntityId playerVisual;
	
	float speed;
	float sprintSpeed;
	float jumpSpeed;
	float acceleration;
	float airAcceleration;

	Vec3 velocity;
	
	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};

// The visual representation of the player
struct PlayerVisual : public Entity
{
	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};