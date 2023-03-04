enum ModelLoadingMode
{
    ModelLoad_None = 0,
    ModelLoad_Nodes,
    ModelLoad_Triangles,
    ModelLoad_Skeleton,
};

void ModelParseTriangleVert(Array<String> triPart, DynArray<MeshVertex>* verts)
{
	MeshVertex vert;

    int parent = atoi(triPart[0].data);
    vert.position = v3(-atof(triPart[1].data), atof(triPart[2].data), atof(triPart[3].data));
    vert.normal   = v3(-atof(triPart[4].data), atof(triPart[5].data), atof(triPart[6].data));
    vert.uv       = v2(atof(triPart[7].data), atof(triPart[8].data));

	int links = atoi(triPart[9].data);


	for (int i = 0; i < 4; i++) vert.boneIds[i] = 0;
	for (int i = 0; i < 4; i++) vert.weights[i] = 0.0;

	for (int i = 0; i < links && i < 4; i++)
	{
		int boneId   = atoi(triPart[10 + i * 2].data);
		float weight = atof(triPart[11 + i * 2].data);

		vert.boneIds[i] = boneId;
		vert.weights[i] = weight;
	}

    ArrayAdd(verts, vert);
}

ModelResource* LoadModel(StringView name)
{
	{
		auto* existing = SearchResource(&resourceManager.models, name);
		if (existing) {
			return existing;
		}
	}

	TextFileHandler handler;
	if (!OpenFileHandler(name, &handler)) {
		LogWarn("[res] Tried to read model that doesn't exist! (%s)", name.data);
		return NULL;
	}
	defer(CloseFileHandler(&handler));

    {
        bool found = false;
        String line = ConsumeNextLine(&handler, &found);
        if (found)
        {
            auto parts = BreakString(line, ' ');
            if (!(parts.size == 2 && (Equals(parts[0], "version") && Equals(parts[1], "1"))))
            {
		        LogWarn("[res] Model does not have a valid header! (%s)", name.data);
                return NULL;
            }
        }
        else
        {
            LogWarn("[res] Model is empty (missing header)! (%s)", name.data);
            return NULL;
        }
    }

	TriangleMesh mesh;
	InitMesh(&mesh, MeshFlag_Skeletal);
	// ~Todo when we have a binary format we'll store whether or not theres skeletal mesh info

    ModelLoadingMode mode = ModelLoad_None;
	while (true) {
		bool found = false;
		String line = ConsumeNextLine(&handler, &found);
		if (!found) break;

        switch (mode)
        {
            case ModelLoad_None:
            {
                if      (Equals(line, "nodes"))
                    mode = ModelLoad_Nodes;
                else if (Equals(line, "triangles"))
                    mode = ModelLoad_Triangles;
                else if (Equals(line, "skeleton"))
                    mode = ModelLoad_Skeleton;
            } break;

            case ModelLoad_Nodes:
            {
                if (Equals(line, "end")) { mode = ModelLoad_None; break; }
            
                auto parts = BreakString(line, ' ');

				int boneId      = atoi(parts[0].data);
				String boneName = Substr(parts[1], 1, parts[1].length - 2);
				int parentId    = atoi(parts[2].data);

				Armature armature { };
				armature.id       = boneId;
				armature.parentId = parentId;
				armature.name 	  = CopyString(boneName);
				armature.pose 	  = CreateXform();
				ArrayAdd(&mesh.armatures, armature);
            } break;

            case ModelLoad_Triangles:
            {
                if (Equals(line, "end")) { mode = ModelLoad_None; break; }
            
                String material = line;

                String t0 = ConsumeNextLine(&handler, &found);
                if (!found) break;

                String t1 = ConsumeNextLine(&handler, &found);
                if (!found) break;

                String t2 = ConsumeNextLine(&handler, &found);
                if (!found) break;

                auto tp0 = BreakString(t0, ' ');
                auto tp1 = BreakString(t1, ' ');
                auto tp2 = BreakString(t2, ' ');

                ModelParseTriangleVert(tp0, &mesh.vertices);
                ModelParseTriangleVert(tp2, &mesh.vertices);
                ModelParseTriangleVert(tp1, &mesh.vertices);
            } break;

            case ModelLoad_Skeleton:
            {
                if (Equals(line, "end")) { mode = ModelLoad_None; break; }

                auto parts = BreakString(line, ' ');

                if (Equals(parts[0], "time"))
                {
                    if (atoi(parts[1].data) != 0)
                    {
                        LogWarn("[res] More than 1 frame of pose in model file %s!!!", name.data);
                        break;
                    }
                }
                else
                {
                    int boneId = atoi(parts[0].data);
                    Vec3 pos = v3(-atof(parts[1].data), atof(parts[2].data), atof(parts[3].data));
                    Mat4 rot = Mul(
                                    RotationMatrixAxisAngle(v3(0,0,-1), atof(parts[6].data) / PI * 180),
                                Mul(
                                    RotationMatrixAxisAngle(v3(0,1,0), atof(parts[5].data) / PI * 180),
                                    RotationMatrixAxisAngle(v3(-1,0,0), atof(parts[4].data) / PI * 180)
                                )
                            );

                    Xform pose = CreateXform();
                    pose.position = pos;
                    pose.rotation = rot;

                    For (mesh.armatures)
                    {
                        if (it->id == boneId)
                        {
                            it->pose = pose;
                            break;
                        }
                    }
                }

            } break;
        }
	}

	ModelResource model;
	model.name = CopyString(name);
	model.mesh = mesh;

	Log("[res] loaded model (%s)", name.data);

	return BucketArrayAdd(&resourceManager.models, model);
}


