#include "character_controller.h"



void InitCharacter(Entity* entity)
{
    Assert(entity != NULL);
    Assert((entity->features & EntityFeature_CharacterController) == 0);

    entity->features |= EntityFeature_CharacterController;

    CharacterController characterController {};

    Assert(characterController.maxGroundedAngle == 70);

    entity->characterController = characterController;
}

void DestroyCharacter(Entity* entity)
{
    Assert(entity != NULL);

    Assert((entity->features & EntityFeature_CharacterController) != 0);
}


void StepCharacter(World* world, Entity* entity, Vec3 movement)
{
    CharacterController* character = &entity->characterController;


    // NOTE(...):
    // this was previously only running if the player controller was in the grounded state?

    { // grounded check
        RayHit hit;
        if (SweepCapsule(&world->physicsWorld, 
                            entity->xform.position + v3(0, character->stepHeight, 0), 
                            v3(0, -1, 0),
                            CreateXform(),
                            character->radius,
                            character->halfHeight,
                            character->stepHeight * 2,
                            &hit))
        {
            GizmoDisc(hit.position, hit.normal, 1.0f, 1.2f, v3(0.5, 0.0, 0.5));

            // // Doesn't work correctly (no stepping) as of rn, need to take into account the direction we're going
            // this->xform.position = this->xform.position + v3(0, stepHeight, 0) + v3(hit.distance) * v3(0,-1,0);

            // The angle we touch the ground is not flat enough to count us as grounded.
            if (hit.normal.y < Cos(character->maxGroundedAngle))
            {
                character->grounded = false;
            }
        }
        else
        {
            character->grounded = false;
        }
    }


    // physics stuff
    if (Length(movement) > 0)
    {
        Vec3 movementAccum = movement;

        const int Move_Steps = 10;
        for (int i = 0; i < Move_Steps; i++)
        {
            RayHit hit;
            if (Length(movementAccum) > 0
                && SweepCapsule(&world->physicsWorld, 
                                entity->xform.position, 
                                Normalize(movementAccum),
                                CreateXform(),
                                character->radius,
                                character->halfHeight,
                                Length(movementAccum),
                                &hit))
            {
                Vec3 movementAtIntersection = movementAccum - Normalize(movementAccum) * v3(hit.distance);
                float normalComponent = Dot(hit.normal, movementAtIntersection);
                
                Vec3 tangentDir = Normalize(movementAccum) - v3(Dot(Normalize(movementAccum), hit.normal)) * hit.normal;
                Vec3 preHitMovement = Normalize(movementAccum) * v3(hit.distance);
                

                Vec3 step = preHitMovement + hit.normal * v3(-normalComponent);

                movementAccum = tangentDir * v3(Length(movementAccum) - hit.distance);

                entity->xform.position = entity->xform.position + step;

                if (hit.normal.y >= Cos(character->maxGroundedAngle))
                {
                    character->grounded = true;
                }
            }
            else
            {
                entity->xform.position = entity->xform.position + movementAccum;
                break; // no need to move any more
            }
        }
    }
}

