#include "anim.h"


void InitAnimator(Animator* animator, Renderer* renderer, TriangleMesh* mesh)
{
	animator->mesh = mesh;
	animator->armatureFrames = MakeDynArray<ArmatureFrame>();
	animator->armatureBufferData = {};
	animator->armatureBuffer = renderer->CreateBuffer(animator->armatureFrames.data, sizeof(ArmatureBuffer), RENDERBUFFER_FLAGS_CONSTANT | RENDERBUFFER_FLAGS_USAGE_STREAM);
	Assert(mesh->flags & MeshFlag_Skeletal);

	For (mesh->armatures)
	{
		ArmatureFrame frame = {
			.id   = it->id,
			.pose = it->pose
		};

		ArrayAdd(&animator->armatureFrames, frame);
	}	
}

void DeleteAnimator(Animator* animator, Renderer* renderer)
{
	renderer->FreeBuffer(animator->armatureBuffer);
	FreeDynArray(&animator->armatureFrames);
}

intern Mat4 EvaluateBonePose(TriangleMesh* mesh, int id, Array<ArmatureFrame> armatureFrames)
{
	ArmatureFrame* frame = NULL;
	For (armatureFrames)
	{
		if (it->id == id)
			frame = it;
	}

	Mat4 pose = CreateMatrix(1);

	if (frame)
	{
		// Figure out which armature this frame is referring to
		Armature* armature = NULL;
		For (mesh->armatures)
		{
			if (it->id == frame->id)
			{
				armature = it;
				break;
			}
		}

		if (!armature) return pose;

		Mat4 parent = CreateMatrix(1);

		// If we have a parent armature take its pose into account when calculating our own
		if (armature->parentId != -1)
		{
			parent = EvaluateBonePose(mesh, armature->parentId, armatureFrames);
		}

		pose = Mul(parent, XformMatrix(frame->pose));
	}

	return pose;
}


intern Mat4 EvaluateBoneDefaultPose(TriangleMesh* mesh, int id)
{
	Mat4 pose = CreateMatrix(1);

	// Figure out which armature this frame is referring to
	Armature* armature = NULL;
	For (mesh->armatures)
	{
		if (it->id == id)
		{
			armature = it;
			break;
		}
	}

	if (!armature) return pose;

	Mat4 parent = CreateMatrix(1);

	// If we have a parent armature take its pose into account when calculating our own
	if (armature->parentId != -1)
	{
		parent = EvaluateBoneDefaultPose(mesh, armature->parentId);
	}

	pose = Mul(parent, XformMatrix(armature->pose));

	return pose;
}

void UpdateGpuAnimator(Animator* animator, Renderer* renderer)
{
	int highestBoneId = 0;
	ForIt (animator->armatureFrames, armatureFrame)
	{
		if (armatureFrame->id >= ANIMATOR_MAX_BONES)
		{
			// ~Refactor @@QOL Maybe meshes should have names so that the debug info shows which mesh it is?
			LogWarn("[animator] Skeleton mesh has too many bones! Skipping...");
			continue;
		}

		if (highestBoneId < armatureFrame->id)
			highestBoneId = armatureFrame->id;


		// Determine the bone's pose matrix
		Mat4 bonePose = EvaluateBonePose(animator->mesh, armatureFrame->id, animator->armatureFrames);
		Mat4 defaultPose = EvaluateBoneDefaultPose(animator->mesh, armatureFrame->id); // CreateMatrix(1);

		// For (animator->mesh->armatures)
		// {
		// 	if (it->id == armatureFrame->id)
		// 	{

		// 		int idArmature = it->id;
		// 		bool found = true;
		// 		while (found)
		// 		{
		// 			found = false;
		// 			ForIt (animator->mesh->armatures, arm)
		// 			{
		// 				if (arm->id == idArmature)
		// 				{
		// 					defaultPose = Mul(XformMatrix(arm->pose), defaultPose);
		// 					idArmature = arm->parentId;
		// 					if (idArmature != 0) found = true;
		// 					break;
		// 				}
		// 			}
		// 		}

		// 		break;
		// 	}
		// }

		animator->armatureBufferData.bones[armatureFrame->id] = Mul(bonePose, Inverse(defaultPose));
	}
	// The bones are not nessecarily a packed array (at the moment, with the used model format)
	// so we need some array that can hold the total amount of indices we need
	animator->armatureBufferData.boneCount = highestBoneId + 1;

	void* gpuData = renderer->MapBuffer(animator->armatureBuffer);
	Memcpy(gpuData, &animator->armatureBufferData, sizeof(ArmatureBuffer));
	renderer->UnmapBuffer(animator->armatureBuffer);
}

///////////////////////////////

void InitAnimation(Animation* animation, float frameRate, int frameCount)
{
	Assert(animation != NULL);

	animation->frameRate = frameRate;
	animation->frames = MakeArray<AnimationFrame>(frameCount);
}

void DeleteAnimation(Animation* animation)
{
	Assert(animation != NULL);

	For (animation->frames)
	{
		FreeArray(&it->armatures);
	}
	FreeArray(&animation->frames);
}