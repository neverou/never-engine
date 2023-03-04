#pragma once 

#include "std.h"
#include "renderer.h"
#include "maths.h"

struct MeshVertex
{
	Vec3 position;
	Vec3 normal;
	Vec2 uv;
	
	float weights[4];
	float boneIds[4];
};

typedef u8 MeshFlags;
enum
{
    MeshFlag_None     = 0x0,
    MeshFlag_Skeletal = 0x1,
	MeshFlag_Static   = 0x2, // Not used yet!
};

struct Armature
{
	int id;
	int parentId; // parent=0 means there is no parent
	String name;
	Xform pose; 
};

struct TriangleMesh
{
    MeshFlags flags;

    DynArray<MeshVertex> 		 vertices;
    DynArray<u32>        		 indices;

	// Anim Data
	DynArray<Armature> armatures;

    // gpu data
	bool invalidated;
    RendererHandle vertexBuffer;
    // RendererHandle indexBuffer; // ~Incomplete do this
};

void InitMesh(TriangleMesh* mesh, MeshFlags flags);
void DeleteMesh(TriangleMesh* mesh);

void UpdateGpuMesh(TriangleMesh* mesh, Renderer* renderer);
