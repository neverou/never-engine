#include "entity.h"

#include "game.h"

// physics
bool InitPhysics(Entity* entity, World* world, RigidbodyType bodyType)
{
	// Make sure the arguments are valid
	Assert(world != NULL);
	Assert(entity != NULL);
	Assert(bodyType != Rigidbody_None);
	
	// Make sure we don't already have a rigidbody
	Assert(!(entity->features & EntityFeature_Rigidbody));

	entity->rigidbody = MakeRigidbody(&world->physicsWorld, entity->xform, bodyType);
	if (entity->rigidbody)
		entity->rigidbody->entity = entity;
	else
		return false;

	// @Todo allow deleting rigidbody

	return true;
}

bool InitAnimation(Entity* entity)
{
	Assert(entity != NULL);

	// Make sure we don't already have an animator
	Assert(!(entity->features & EntityFeature_Animator));
	if (entity->mesh == NULL) return false;

	InitAnimator(&entity->animator, game->renderer, entity->mesh);
	entity->features |= EntityFeature_Animator;

	
	// @Todo allow deleting animator

	return true;
}
