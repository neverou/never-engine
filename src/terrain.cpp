#include "terrain.h"

#include "logger.h"
#include "entityutil.h"
#include "resource.h"
#include "game.h"
#include "maths.h"


intern void AddTerrain(World* world, EntityId id, int chunkX, int chunkZ)
{
	ArrayAdd(&world->terrainKeys, TerrainChunkIdx { chunkX, chunkZ });
	ArrayAdd(&world->terrains, id);
}


intern void RemoveTerrain(World* world, EntityId id)
{
	ForIdx (world->terrains, idx)
	{
		if (world->terrains[idx] == id)
		{
			ArrayRemoveAt(&world->terrains, idx);
			ArrayRemoveAt(&world->terrainKeys, idx);
		}
	}
}


EntityId LookupTerrain(World* world, int chunkX, int chunkZ)
{
	ForIdx (world->terrainKeys, idx)
	{
		TerrainChunkIdx chunkIdx = world->terrainKeys[idx];
		if (chunkIdx.x == chunkX && chunkIdx.z == chunkZ)
		{
			return world->terrains[idx];
		}
	}
	return 0;
}

Terrain* LookupTerrainActor(World* world, int chunkX, int chunkZ)
{
	EntityId id = LookupTerrain(world, chunkX, chunkZ);
	auto* terrain = (Terrain*)GetWorldEntity(world, id);
	return terrain;
}





intern void ConnectTerrainEdges(World* world, int chunkX, int chunkZ, bool topLevel)
{
	auto* chunk = LookupTerrainActor(world, chunkX, chunkZ);
	Assert(chunk);
    
	auto* chunk10 = LookupTerrainActor(world, chunkX + 1, chunkZ);
	auto* chunk01 = LookupTerrainActor(world, chunkX, chunkZ + 1);
	auto* chunk11 = LookupTerrainActor(world, chunkX + 1, chunkZ + 1);
    
    
	for (int i = 0; i < Terrain_ChunkSize; i++)
	{
		int j = Terrain_ChunkSize;
		if (chunk01) chunk->heights[i + j * (Terrain_ChunkSize + 1)] = chunk01->heights[i];
		if (chunk10) chunk->heights[j + i * (Terrain_ChunkSize + 1)] = chunk10->heights[i * (Terrain_ChunkSize + 1)];
	}
	if (chunk11) chunk->heights[(Terrain_ChunkSize + 1) * (Terrain_ChunkSize + 1) - 1] = chunk11->heights[0];
    
	if (topLevel)
	{
		auto chunkS10 = LookupTerrain(world, chunkX - 1, chunkZ);
		auto chunkS01 = LookupTerrain(world, chunkX, chunkZ - 1);
		auto chunkS11 = LookupTerrain(world, chunkX - 1, chunkZ - 1);
        
		if (chunkS10) ConnectTerrainEdges(world, chunkX - 1, chunkZ, false);
		if (chunkS01) ConnectTerrainEdges(world, chunkX, chunkZ - 1, false);
		if (chunkS11) ConnectTerrainEdges(world, chunkX - 1, chunkZ - 1, false);
	}
}

#include "allocators.h"

intern void UpdateTerrain(World* world, Terrain* terrain)
{
	terrain->invalidated = false;
    
	ArrayClear(&terrain->mesh->vertices);
    
	ConnectTerrainEdges(world, terrain->chunkX, terrain->chunkZ, true);
    
	int heightMapSizeX = terrain->sizeX + 1;

	auto vertices = MakeArray<MeshVertex>((terrain->sizeX + 1) * (terrain->sizeZ + 1), Frame_Arena);

	for (int z = 0; z <= terrain->sizeZ; z++)
	{
		for (int x = 0; x <= terrain->sizeX; x++)
		{
			float h00 = terrain->heights[x + heightMapSizeX * z];
			MeshVertex v00 = { v3(x, h00, z), v3(0, 1, 0), v2(1.0 / 8.0) * v2(x + terrain->chunkX * Terrain_ChunkSize, z + terrain->chunkZ * Terrain_ChunkSize) };
			
			float h10 = 0;
			float h01 = 0;
            
			if (x < terrain->sizeX && z < terrain->sizeZ)
			{
				h10 = terrain->heights[(x + 1) + heightMapSizeX * z];
                h01 = terrain->heights[x       + heightMapSizeX * (z + 1)];
			}
			else
			{
				// ~Todo read from neighboring heightmaps!
				h10 = h01 = h00;

				// ~FixMe terrain edges dont work!!
				// Terrain* terrainRight = LookupTerrainActor(world, terrain->chunkX + 1, terrain->chunkZ);
				// Terrain* terrainUp    = LookupTerrainActor(world, terrain->chunkX, terrain->chunkZ + 1);
                
				// Assert(terrainRight != terrain);
				// Assert(terrainUp != terrain);
                
				// if (terrainRight)
				// 	Log("Right!");
				// if (terrainUp)
				// 	Log("Up!");
                
				// h10 = !terrainRight ? h00 : terrainRight->heights[heightMapSizeX * z];
				// h01 = !terrainUp    ? h00 :    terrainUp->heights[x];
			}
            
			
			v00.normal = Normalize(Cross(v3(x,     h01, z + 1) - v00.position, v3(x + 1, h10, z) - v00.position));
            
			vertices[x + (terrain->sizeX + 1) * z] = v00;
		}
	}
    
    
	auto verticesPostNormals = MakeArray<MeshVertex>((terrain->sizeX + 1) * (terrain->sizeZ + 1), Frame_Arena);
    
	for (int z = 0; z <= terrain->sizeZ; z++)
		for (int x = 0; x <= terrain->sizeX; x++)
	    {
	        
	        MeshVertex v = vertices[x + (terrain->sizeX + 1) * z];
	        
	        int bf = 0;
	        Vec3 normalAccum = v3(0);
	        for (int j = -1; j <= 1; j++)
	            for (int i = -1; i <= 1; i++)
	        {
	            if ((x + i) >= 0 && (x + i) <= terrain->sizeX &&
	                (z + j) >= 0 && (z + j) <= terrain->sizeZ)
	            {
	                MeshVertex vn = vertices[(x + i) + (terrain->sizeX + 1) * (z + j)];
	                normalAccum = normalAccum + vn.normal;
	                bf++;
	            }
	        }
	        
	        v.normal = normalAccum / v3(bf);
	        
	        verticesPostNormals[x + (terrain->sizeX + 1) * z] = v;
	    }
	    
	for (int z = 0; z < terrain->sizeZ; z++)
		for (int x = 0; x < terrain->sizeX; x++)
    {
        MeshVertex v00 = verticesPostNormals[x + (terrain->sizeX + 1) * z];
        MeshVertex v10 = verticesPostNormals[(x + 1) + (terrain->sizeX + 1) * z];
        MeshVertex v11 = verticesPostNormals[(x + 1) + (terrain->sizeX + 1) * (z + 1)];
        MeshVertex v01 = verticesPostNormals[x + (terrain->sizeX + 1) * (z + 1)];
        
        ArrayAdd(&terrain->mesh->vertices, v00);
        ArrayAdd(&terrain->mesh->vertices, v01);
        ArrayAdd(&terrain->mesh->vertices, v10);
        
        ArrayAdd(&terrain->mesh->vertices, v01);
        ArrayAdd(&terrain->mesh->vertices, v11);
        ArrayAdd(&terrain->mesh->vertices, v10);
    }
    
	terrain->mesh->invalidated = true;
}





void ValidateTerrains(World* world)
{
	For (world->terrains)
	{
		auto* terrain = (Terrain*)GetWorldEntity(world, *it);
		if (terrain)
		{
			if (terrain->invalidated)
			{
				UpdateTerrain(world, terrain);
			}
		}
		else
			LogWarn("Error iterating terrain while validating! id=%lu", *it);
	}	
}





void Terrain::Init(World* world)
{
	this->mesh = NULL;
	this->material = LoadMaterial("terrain/terrain.mat"); // ~Hack use shader catalog instead
    
	this->init = false;
	this->path = CopyString(TPrint("map/%d_%d.terrain", this->chunkX, this->chunkZ));
    
	// create a mesh from the terrain data
	TextFileHandler handler;
    
	// ~Refactor @@FileExists
	bool fileExists = false;
	{
		File file;
		fileExists = Open(&file, this->path, FILE_READ);
		if (fileExists)
			Close(&file);
	}

	if (!fileExists || this->fromScratch)
	{
		File file;
		if (Open(&file, this->path, FILE_WRITE | FILE_CREATE | FILE_TRUNCATE)) 
		{
			String buffer = MakeString();
			defer(FreeString(&buffer));

			String header = TPrint("%d %d\n", Terrain_ChunkSize, Terrain_ChunkSize);
			buffer.Concat(header);

			for (int j = 0; j <= Terrain_ChunkSize; j++)
			{
				for (int i = 0; i <= Terrain_ChunkSize; i++)
				{
					buffer.Concat("0");
					if (i < Terrain_ChunkSize)
						buffer.Concat(" ");
				}
				if (j < Terrain_ChunkSize)
					buffer.Concat("\n");
			}
			Write(&file, buffer.data, buffer.length);
			Close(&file);
		}
		else
		{
			LogWarn("Failed to create default terrain file!");
		}
	}

	if (OpenFileHandler(this->path, &handler))
	{
		// ~Hack: only add the terrain if the chunk file loads,
		// we only need to do this because an issue at @world->TerrainSpawnIssue 
		AddTerrain(world, this->id, this->chunkX, this->chunkZ);

		// Go to the correct position
		this->xform = CreateXform();
		this->xform.position.x = this->chunkX * Terrain_ChunkSize;
		this->xform.position.z = this->chunkZ * Terrain_ChunkSize;


		bool found = true;
		String line = ConsumeNextLine(&handler, &found);
		auto sizeParts = BreakString(line, ' ');

		this->sizeX = atoi(sizeParts[0].data);
		this->sizeZ = atoi(sizeParts[1].data);

		int heightMapSizeX = this->sizeX + 1;
		int heightMapSizeZ = this->sizeZ + 1;

		this->heights = MakeArray<float>(heightMapSizeX * heightMapSizeZ);
		this->init = true;


		int row = 0;
		while (true) {
			String line = ConsumeNextLine(&handler, &found);
			if (!found) break;
            
			if (row > heightMapSizeZ)
				break;
            
            
			auto parts = BreakString(line, ' ');
			if (parts.size != heightMapSizeX)
			{
				LogWarn("Terrain %s possibly corrupted, X target %d is actually %d in the file!", this->path.data, heightMapSizeX, parts.size);
				LogWarn("%d: %s", row, line.data);
				if (parts.size < heightMapSizeX)
					goto fail;
			}
            
			for (int i = 0; i < heightMapSizeX; i++)
			{
				this->heights[i + row * heightMapSizeX] = atof(parts[i].data); 
			}
            
			row++;
		}
		CloseFileHandler(&handler);



		// make a mesh
		this->mesh = (TriangleMesh*)Alloc(sizeof(TriangleMesh));
		InitMesh(this->mesh, MeshFlag_None);
		

		Assert(this->sizeX == Terrain_ChunkSize);
		Assert(this->sizeZ == Terrain_ChunkSize);
        
		// create the vertices
		for (int z = 0; z < this->sizeZ; z++)
		{
			for (int x = 0; x < this->sizeX; x++)
			{
				MeshVertex v = {};
                
				for (int i = 0; i < 6; i++)
                    ArrayAdd(&this->mesh->vertices, v);
			}
		}
        
		this->invalidated = true;
        
		UpdateTerrain(world, this);
        
        
		InitPhysics(this, world, Rigidbody_Static);
		//AddBoxCollider(this->rigidbody, CreateXform(), v3(Terrain_ChunkSize, 1, Terrain_ChunkSize));
		AddConcaveMeshCollider(this->rigidbody, CreateXform(), this->mesh);
	}
	else
	{
		fail:
		LogWarn("Failed to load terrain chunk %s!", this->path.data);
	}
}

void Terrain::Start(World* world)
{
	
}

void Terrain::Update(World* world)
{
	
}

void Terrain::Destroy(World* world)
{
	FreeString(&this->path);
    
	if (this->init) FreeArray(&this->heights);
	if (this->mesh) DeleteMesh(this->mesh);
    
	RemoveTerrain(world, this->id);
}

RegisterEntity(Terrain);



void TerrainWriteHeight(World* world, int x, int z, float height)
{
	int chunkX = (int)Floor(x / float(Terrain_ChunkSize));
	int chunkZ = (int)Floor(z / float(Terrain_ChunkSize));
    
	EntityId id = LookupTerrain(world, chunkX, chunkZ);
	if (id)
	{
		auto* actor = (Terrain*)GetWorldEntity(world, id);
        
		int inChunkX = x - chunkX * Terrain_ChunkSize;
		int inChunkZ = z - chunkZ * Terrain_ChunkSize;
        
		actor->heights[inChunkX + inChunkZ * (Terrain_ChunkSize + 1)] = height;
		
		actor->invalidated = true;
	}
}

float TerrainReadHeight(World* world, int x, int z)
{
	int chunkX = (int)Floor(x / float(Terrain_ChunkSize));
	int chunkZ = (int)Floor(z / float(Terrain_ChunkSize));
    
	EntityId id = LookupTerrain(world, chunkX, chunkZ);
	if (id)
	{
		auto* actor = (Terrain*)GetWorldEntity(world, id);
        
		int inChunkX = x - chunkX * Terrain_ChunkSize;
		int inChunkZ = z - chunkZ * Terrain_ChunkSize;
        
		return actor->heights[inChunkX + inChunkZ * (Terrain_ChunkSize + 1)];
	}
	else
	{
		return 0;
	}
}







float TerrainSampleHeight(World* world, float x, float z)
{
	int x0 = (int)Floor(x);
	int x1 = (int)Floor(x) + 1;
	int z0 = (int)Floor(z);
	int z1 = (int)Floor(z) + 1;
    
	float h00 = TerrainReadHeight(world, x0, z0);
	float h10 = TerrainReadHeight(world, x1, z0);
	float h01 = TerrainReadHeight(world, x0, z1);
	float h11 = TerrainReadHeight(world, x1, z1);
    
	return Lerp(
                Lerp(h00, h10, Fract(x)),
                Lerp(h01, h11, Fract(x)),
                Fract(z)
                );
}



intern inline void GatherTerrainActors(World* world, Terrain** pChunks, 
								  int minChunkX, int maxChunkX,
								  int minChunkZ, int maxChunkZ,
								  int xChunks, int zChunks)
{
	for (int i = minChunkX; i <= maxChunkX; i++)
	{
		for (int j = minChunkZ; j <= maxChunkZ; j++)
		{
			EntityId id = LookupTerrain(world, i, j);
			pChunks[(i - minChunkX) + (j - minChunkZ) * xChunks] = (Terrain*)GetWorldEntity(world, id);
		}
	}
}

Array<float> TerrainReadHeightArea(World* world, int x, int z, int xSize, int zSize) {
	SArray<float> area = MakeArray<float>(xSize * zSize, Frame_Arena);
    
    // Initialize everything to 0,
    // Avoids reading weird values if theres no terrain there (causes issues w/ the flatten brush)
    For (area) *it = 0;

	int minChunkX = (int)Floor(x / float(Terrain_ChunkSize));
	int minChunkZ = (int)Floor(z / float(Terrain_ChunkSize));
    
	int maxChunkX = (int)Floor((x + xSize - 1) / float(Terrain_ChunkSize));
	int maxChunkZ = (int)Floor((z + zSize - 1) / float(Terrain_ChunkSize));
    
	int xChunks = (maxChunkX - minChunkX) + 1;
	int zChunks = (maxChunkZ - minChunkZ) + 1;
	Terrain** pChunks = (Terrain**)StackAlloc(sizeof(Terrain*) * xChunks * zChunks);
    GatherTerrainActors(world, pChunks, minChunkX, maxChunkX, minChunkZ, maxChunkZ, xChunks, zChunks);
    
	for (int i = x; i < x + xSize; i++) {
		for (int j = z; j < z + zSize; j++) {
			int chunkX = (int)Floor(i / float(Terrain_ChunkSize));
			int chunkZ = (int)Floor(j / float(Terrain_ChunkSize));

			int inChunkX = i - chunkX * Terrain_ChunkSize;
			int inChunkZ = j - chunkZ * Terrain_ChunkSize;

			Assert(inChunkX >= 0);
			Assert(inChunkZ >= 0);
			Assert(inChunkX < Terrain_ChunkSize);
			Assert(inChunkZ < Terrain_ChunkSize);

			Terrain* chunk = pChunks[(chunkX - minChunkX) + (chunkZ - minChunkZ) * xChunks];
			if (!chunk) continue;
			area[(i - x) + (j - z) * xSize] = chunk->heights[inChunkX + inChunkZ * (Terrain_ChunkSize + 1)];
		}
	}
    
	return area;
}

void TerrainWriteHeightArea(World* world, int x, int z, int xSize, int zSize, Array<float> area)
{
	Assert(xSize * zSize == area.size);
    
	int minChunkX = (int)Floor(x / float(Terrain_ChunkSize));
	int minChunkZ = (int)Floor(z / float(Terrain_ChunkSize));
    
	int maxChunkX = (int)Floor((x + xSize) / float(Terrain_ChunkSize));
	int maxChunkZ = (int)Floor((z + zSize) / float(Terrain_ChunkSize));
    
	int xChunks = (maxChunkX - minChunkX) + 1;
	int zChunks = (maxChunkZ - minChunkZ) + 1;
	Terrain** pChunks = (Terrain**)StackAlloc(sizeof(Terrain*) * xChunks * zChunks);
    GatherTerrainActors(world, pChunks, minChunkX, maxChunkX, minChunkZ, maxChunkZ, xChunks, zChunks);

	for (int i = x; i < x + xSize; i++) {
		for (int j = z; j < z + zSize; j++) {
			int chunkX = (int)Floor(i / float(Terrain_ChunkSize));
			int chunkZ = (int)Floor(j / float(Terrain_ChunkSize));
            
			int inChunkX = i - chunkX * Terrain_ChunkSize;
			int inChunkZ = j - chunkZ * Terrain_ChunkSize;
            
			Assert(inChunkX >= 0);
			Assert(inChunkZ >= 0);
			Assert(inChunkX < Terrain_ChunkSize);
			Assert(inChunkZ < Terrain_ChunkSize);
            
			Terrain* chunk = pChunks[(chunkX - minChunkX) + (chunkZ - minChunkZ) * xChunks];
            
			if (!chunk) continue;
			chunk->heights[inChunkX + inChunkZ * (Terrain_ChunkSize + 1)] = area[(i - x) + (j - z) * xSize];
			chunk->invalidated = true;
		}
	}
}


Terrain* CreateTerrain(World* world, int chunkX, int chunkZ) {
	// ~Hack ~FixMe Create a way to set vars before SpawnEntity!!
	Terrain* terrain = (Terrain*)SpawnEntity("Terrain", world, CreateXform()); // @RuntimeSpawnEntity
	terrain->chunkX = chunkX;
	terrain->chunkZ = chunkZ;
	terrain->fromScratch = true;
	ReloadEntity(world, terrain);
	return terrain;
}

void SaveTerrain(World* world)
{
	For (world->terrains)
	{
		auto* terrain = (Terrain*)GetWorldEntity(world, *it);
        
		File file;
		if (Open(&file, terrain->path, FILE_WRITE | FILE_CREATE | FILE_TRUNCATE))
		{
			String buffer = MakeString();
			defer(FreeString(&buffer));

			StringView header = TPrint("%d %d\n", Terrain_ChunkSize, Terrain_ChunkSize);
			buffer.Concat(header);			
            
			for (int j = 0; j <= Terrain_ChunkSize; j++)
			{
				for (int i = 0; i <= Terrain_ChunkSize; i++)
				{
					String number = TPrint("%f", terrain->heights[i + j * (Terrain_ChunkSize + 1)]);
					buffer.Concat(number);

					// Put a space between(!) all elements
					if (i < Terrain_ChunkSize) buffer.Concat(" ");
				}

				// Put a new line for all except the last line
				if (j < Terrain_ChunkSize) buffer.Concat("\n");
			}
			Write(&file, buffer.data, buffer.length);
			Close(&file);
		}
		else
		{
			LogWarn("Failed to save terrain %d, %d %s", terrain->chunkX, terrain->chunkZ, terrain->path.data);
		}
	}
}