#pragma once
#include "world.h"
struct MonkeyEasterEgg : public Entity {
	BEGIN_DATADESC(MonkeyEasterEgg)
	DEFINE_FIELD(FIELD_STRING, audioClip)
	END_DATADESC()

	AnimResource* anim;


	AudioPlayer* audioPlayer;
	String audioClip;

	void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};