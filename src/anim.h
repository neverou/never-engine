#pragma once
#include "maths.h"
#include "mesh.h"

// ~Note 
// We can use SSBOs so we dont need a max bone count!
// Probably not necessary tho

#define ANIMATOR_MAX_BONES 128

struct ArmatureFrame
{
	int id;
	Xform pose;
};

struct ArmatureBuffer
{
	Mat4 bones[ANIMATOR_MAX_BONES];
	int boneCount;
};

struct Animator
{
	TriangleMesh* mesh;
	DynArray<ArmatureFrame> armatureFrames;

	ArmatureBuffer armatureBufferData;
	RendererHandle armatureBuffer;
};

// there is currently no way to switch meshes on the animator
void InitAnimator(Animator* animator, Renderer* renderer, TriangleMesh* mesh);
void DeleteAnimator(Animator* animator, Renderer* renderer);

void UpdateGpuAnimator(Animator* animator, Renderer* renderer);

//////

struct AnimationFrame
{
	SArray<ArmatureFrame> armatures;
};

struct Animation
{
	float frameRate;
	SArray<AnimationFrame> frames;
};

void InitAnimation(Animation* animation, float frameRate, int frameCount);
void DeleteAnimation(Animation* animation);
