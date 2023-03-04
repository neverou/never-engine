#pragma once

struct CharacterController
{
    float maxGroundedAngle = 70;
    float stepHeight = 0.2;
    float radius = 0.25;
    float halfHeight = 1;

    bool grounded = false;
};

void InitCharacter(Entity* entity);
void DestroyCharacter(Entity* entity);

void StepCharacter(World* world, Entity* entity, Vec3 movement);