#pragma once

#include "entity.h"

constexpr int Terrain_ChunkSize = 64;

struct TerrainChunkIdx
{
	int x, z;
};


struct Terrain : public Entity
{
	BEGIN_DATADESC(Terrain)
	DEFINE_FIELD(FIELD_INT, chunkX)
	DEFINE_FIELD(FIELD_INT, chunkZ)
	END_DATADESC()

	int sizeX;
	int sizeZ;
	SArray<float> heights;
	bool init = false;

	// used for finding neighbors and stuff (mainly in the editor)
	int chunkX; 
	int chunkZ;

	String path;

	bool invalidated = false;
	bool fromScratch = false;

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};



void ValidateTerrains(World* world);

void TerrainWriteHeight(World* world, int x, int z, float height);
float TerrainReadHeight(World* world, int x, int z);
float TerrainSampleHeight(World* world, float x, float z);

Array<float> TerrainReadHeightArea(World* world, int x, int z, int xSize, int zSize);
void TerrainWriteHeightArea(World* world, int x, int z, int xSize, int zSize, Array<float> area);

EntityId LookupTerrain(World* world, int chunkX, int chunkZ);
Terrain* LookupTerrainActor(World* world, int chunkX, int chunkZ);

Terrain* CreateTerrain(World* world, int chunkX, int chunkZ);
void SaveTerrain(World* world);