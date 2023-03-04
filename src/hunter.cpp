#include "hunter.h"

void Hunter::Init(World* world) {
    this->material = LoadMaterial("models/hunter.mat");

    // @MemoryManage deal with resource memory handling
    auto model = LoadModel("models/link.smd");
    if (model)
        this->mesh = &model->mesh;
    else
        this->mesh = NULL;


    // init character
    InitCharacter(this);
}

void Hunter::Start(World* world) {

}

void Hunter::Update(World* world) {
    Vec3 movement = v3(0,-1,1) * v3(deltaTime);
    StepCharacter(world, this, movement);
}

void Hunter::Destroy(World* world) {
    DestroyCharacter(this);
}

RegisterEntity(Hunter);