#include "mesh.h"

#include "logger.h"
#include "game.h"

void InitMesh(TriangleMesh* mesh, MeshFlags flags)
{
	mesh->flags = flags;
	mesh->vertices = MakeDynArray<MeshVertex>();

	if (mesh->flags & MeshFlag_Skeletal)
	{
		mesh->armatures   = MakeDynArray<Armature>();
	}

	mesh->vertexBuffer = 0;
	mesh->invalidated = false;
}

void DeleteMesh(TriangleMesh* mesh)
{
	Renderer* renderer = game->renderSystem.renderer;

	// if the mesh hasnt been GPU'd it wont have a vertex buffer
	if (mesh->vertexBuffer) renderer->FreeBuffer(mesh->vertexBuffer); 

	if (mesh->flags & MeshFlag_Skeletal)
	{
		For (mesh->armatures)
			FreeString(&it->name);

		FreeDynArray(&mesh->armatures);
	}
	FreeDynArray(&mesh->vertices);
}

void UpdateGpuMesh(TriangleMesh* mesh, Renderer* renderer)
{
	if (mesh->vertexBuffer != 0)
	{
		auto buffer = renderer->LookupBuffer(mesh->vertexBuffer);
		if (buffer != NULL)
		{
			if (buffer->size / sizeof(MeshVertex) != mesh->vertices.size)
			{
				renderer->FreeBuffer(mesh->vertexBuffer);
				mesh->vertexBuffer = 0;
			}
		}
	}

	u64 meshBytes = mesh->vertices.size * sizeof(MeshVertex);
	if (mesh->vertexBuffer == 0 && mesh->vertices.size > 0)
	{
		mesh->vertexBuffer = renderer->CreateBuffer(mesh->vertices.data, meshBytes, RENDERBUFFER_FLAGS_VERTEX | RENDERBUFFER_FLAGS_USAGE_DYNAMIC);
		return;
	}

	if (mesh->invalidated)
	{
		mesh->invalidated = false;
		void* mem = renderer->MapBuffer(mesh->vertexBuffer);

		Memcpy(mem, mesh->vertices.data, meshBytes);

		renderer->UnmapBuffer(mesh->vertexBuffer);
	}
}