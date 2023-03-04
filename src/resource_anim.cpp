AnimResource* LoadAnim(StringView name)
{
	{
		auto* existing = SearchResource(&resourceManager.anims, name);
		if (existing) {
			return existing;
		}
	}

	TextFileHandler handler;
	if (!OpenFileHandler(name, &handler))
    {
		LogWarn("[res] Tried to read anim that doesn't exist! (%s)", name.data);
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
		        LogWarn("[res] Animation does not have a valid header! (%s)", name.data);
                return NULL;
            }
        }
        else
        {
            LogWarn("[res] Animation is empty (missing header)! (%s)", name.data);
            return NULL;
        }
    }



	int time = 0;
	auto frames = MakeDynArray<AnimationFrame>();
	defer(FreeDynArray(&frames));

	auto armatures = MakeDynArray<Armature>();
	defer(FreeDynArray(&armatures));


    ModelLoadingMode mode = ModelLoad_None;
	while (true)
    {
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
				ArrayAdd(&armatures, armature);
            } break;

            case ModelLoad_Triangles:
            {
                if (Equals(line, "end")) { mode = ModelLoad_None; break; }
            } break;

            case ModelLoad_Skeleton:
            {
                if (Equals(line, "end")) { mode = ModelLoad_None; break; }

                auto parts = BreakString(line, ' ');

                if (Equals(parts[0], "time"))
                {
                	bool success;
                    time = ParseS32(parts[1], &success);

                    if (time >= frames.size)
                    {
                    	AnimationFrame frame;
                    	frame.armatures = MakeArray<ArmatureFrame>(armatures.size);
                    	ArrayAdd(&frames, frame);
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

                    ArmatureFrame frame = { .id = boneId, .pose = pose };
                    frames[frames.size - 1].armatures[boneId] = frame;
                }

            } break;
        }
	}

	AnimResource anim;
	anim.name = CopyString(name);
	
	InitAnimation(&anim.animation, 60, frames.size);
	ForIdx (frames, idx)
	{
		anim.animation.frames[idx] = frames[idx];
	}

	Log("[res] loaded animation (%s)", name.data);

	return BucketArrayAdd(&resourceManager.anims, anim);
}