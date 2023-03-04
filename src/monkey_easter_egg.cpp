#include "monkey_easter_egg.h"
#include "game.h"
#include "resource.h"
#include "entityutil.h"


void MonkeyEasterEgg::Init(World* world)
{
	ModelResource* model = LoadModel("models/Test.smd");
	if (model) this->mesh = &model->mesh;
	else this->mesh = NULL;

	this->material = LoadMaterial("models/water.mat");
	
	// Init Anim
	{
		InitAnimation(this);
	}

	this->anim = LoadAnim("models/anims/ArmatureAction.003.smd");


	{
		auto audioTest = LoadAudio(this->audioClip);
        
		if (audioTest)
		{
			AudioPlayer player{};
			player.buffer = audioTest->buffer;
			player.volume = 10;
			player.sourcePosition = this->xform.position;
			player.flags = AudioPlayer_Flags_Loop;
			this->audioPlayer = BucketArrayAdd(&world->audioPlayers, player);
		}

	}
}


void MonkeyEasterEgg::Start(World* world)
{

}

void MonkeyEasterEgg::Update(World* world)
{
    // static float rotation = 0;
    // rotation += deltaTime * 90;
    // this->xform.rotation = RotationMatrixAxisAngle(v3(0, 1, 0), rotation);

    local_persist int a = 0;
    local_persist int frame = 0;

    if (a % 3 == 0)
    {
    	a = 0;
		frame++;
		if (frame >= this->anim->animation.frames.size) frame = 0;

    	For (this->animator.armatureFrames)
    	{
    		it->pose = this->anim->animation.frames[frame].armatures[it->id].pose;
    	}
    }
    a++;
}

void MonkeyEasterEgg::Destroy(World* world)
{
    BucketArrayRemove(&world->audioPlayers, this->audioPlayer);
}


RegisterEntity(MonkeyEasterEgg);