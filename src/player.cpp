#include "player.h"
#include "logger.h"

#include "resource.h"

#include "game.h"

#include "audio.h"
#include "entityutil.h"

intern struct
{
    float horizontal;
    float vertical;
    // moving direction

    bool moveLeft;
    bool moveRight;
    bool moveForward;
    bool moveBackward;
    bool moveUp;
    bool moveDown;
	bool sprint;
	bool jump;

	bool reload;
} playerInput;

#include "terrain.h"
#include "maths.h"

#include "gizmos.h"


intern void PlayerHandleEvent(const Event* event, void* data) {
	if (event->type == Event_Mouse && event->mouse.type == MOUSE_MOVE) {
		playerInput.horizontal += event->mouse.moveDeltaX / 16.0;
		playerInput.vertical   += event->mouse.moveDeltaY / 16.0;
	}

	if (event->type == Event_Keyboard) {
		bool keyState = event->key.type == KEY_EVENT_DOWN;

		switch (event->key.key) {
			case K_LSHIFT: {
				playerInput.sprint = keyState;
				break;
			}
			case K_SPAKE: { // wth lol
				if(keyState && !event->key.isRepeat) playerInput.jump = true;
				break;
			}
			case 'w': {
				playerInput.moveForward = keyState;
				break;
			}
			case 'a': {
				playerInput.moveLeft = keyState;
				break;
			}
			case 's': {
				playerInput.moveBackward = keyState;
				break;
			}
			case 'd': {
				playerInput.moveRight = keyState;
				break;
			}
			case 'e': {
				playerInput.moveUp = keyState;
				break;
			}
			case 'q': {
				playerInput.moveDown = keyState;
				break;
			}
		}
    }
}

local_persist GuiFont font; // ~Temp ~Refactor

void Player::Init(World* world)
{
	// ~CleanUp ~Refactor
	LoadGuiFont(&game->gui, "fonts/NotoSans-Regular.ttf", &font);

	// Setup event handler for input
	{
		local_persist bool eventRegistered = false;
		if (!eventRegistered) {
			eventRegistered = true;
			game->eventBus.AddEventListener(PlayerHandleEvent);
		}
	}


	// TODO move this into the world code
	SetGravity(&world->physicsWorld, v3(0.0f, -9.81f, 0.0f));


	// Init the visual
	EntityRegEntry playerVisualReg;
	GetEntityEntry(PlayerVisual, &playerVisualReg); // ~Refactor make this a macro that just evaluates to the EntityRegEntry
	
	auto playerVisualEnt = SpawnEntity(playerVisualReg, world, CreateXform());
	playerVisualEnt->flags |= Entity_Runtime;
	playerVisualEnt->parent = this->id;
	this->playerVisual = playerVisualEnt->id;


	// Init gameplay
	world->gameManager.playerId = this->id;


	RegisterCombat(&world->gameManager.combat, this);
	this->combatInfo.maxHealth = 100;
	this->combatInfo.health    = 100;


	// character controls
	
	InitCharacter(this);
}

void Player::Destroy(World* world)
{
	UnregisterCombat(&world->gameManager.combat, this);
	
	DestroyCharacter(this);

	// @@MarkForDeletion @Todo Fix DeleteEntity not working bc the frame bla bla bla
	// MarkForDeletion(GetWorldEntity(world, this->playerVisual));
	// MarkForDeletion(GetWorldEntity(world, this->camera));
}

void Player::Start(World* world)
{
	EntityRegEntry cameraReg;
	GetEntityEntry(Camera, &cameraReg);
	this->camera = SpawnEntity(cameraReg, world, this->xform)->id;
}

void Player::Update(World* world)
{
	{ // player debug menu
		local_persist bool showPlayerCtrl = true;

		Panel panel = MakePanel(44, 300, 200);
		
		PanelRow(&panel, 32);
		Gui_TitleBar(&game->gui, "Player Ctrl", panel.atX, panel.atY, panel.width, 32);

		if (showPlayerCtrl) 
		{
			PanelRow(&panel, 50);
			if (Gui_Button(&game->gui, "Reset Pos", panel.atX, panel.atY, panel.width, 50))
			{
				this->xform.position = v3(0, 5, 0);
			}

			// PanelRow(&panel, 32);
			// Gui_DrawRect(&game->gui, panel.atX, panel.atY, panel.width, 32, v4(0.3));
		}

		PanelRow(&panel, 32);
		if (Gui_Button(&game->gui, showPlayerCtrl ? "^^" : "[expand]", panel.atX, panel.atY, panel.width, 32)) showPlayerCtrl = !showPlayerCtrl;
		CompletePanel(&panel);
	}


	Gui_DrawText(&game->gui, TPrint("Health: %d", this->combatInfo.health), 400, 200, GuiTextAlign_Down | GuiTextAlign_Left, &font, v4(0, 1, 0, 0));


	// make the visual face the moving direction
	Entity* playerVisualEnt = GetWorldEntity(world, this->playerVisual);
	playerVisualEnt->xform.rotation = Mul(RotationMatrixAxisAngle(v3(0, 1, 0), 180), RotationMatrixAxisAngle(v3(0, 1, 0), this->rotation.x));

	// camera orbit
	Camera* camera = (Camera*)GetWorldEntity(&game->world, this->camera);
	camera->parent = this->id;
	SetAudioListener(this->camera);


	camera->xform.rotation = Mul(RotationMatrixAxisAngle(v3(0, 1, 0), this->rotation.x), RotationMatrixAxisAngle(v3(1, 0, 0), this->rotation.y));
	camera->xform.rotation = Mul(Transpose(this->xform.rotation), camera->xform.rotation);

	const Vec3 orbitOffset = v3(0, 0, -5);
	const Vec3 offset = v3(0, 1, 0);
	camera->xform.position = Mul(orbitOffset, 1, camera->xform.rotation) + offset;


	// mouse look
	this->rotation.x += playerInput.horizontal * 5;
	this->rotation.y -= playerInput.vertical * 5;
	this->rotation.y = Clamp(this->rotation.y, -90, 90);
	playerInput.vertical = playerInput.horizontal = 0;



	Mat4 bodyRotation = RotationMatrixAxisAngle(v3(0, 1, 0), this->rotation.x);

	



	{


		float moveSpeed = playerInput.sprint ? this->sprintSpeed : this->speed;

		Vec3 moveDir = v3(
			((s32)playerInput.moveRight - (s32)playerInput.moveLeft) * moveSpeed,
			((s32)playerInput.moveUp - (s32)playerInput.moveDown) * moveSpeed,
			((s32)playerInput.moveForward - (s32)playerInput.moveBackward) * moveSpeed);


		if (playerInput.moveUp)
		{
			Assert(Damage(&world->gameManager.combat, NULL, this, 1));
		}


		Vec3 forward = OrientForward(bodyRotation);
		Vec3 right = OrientRight(bodyRotation);
		Vec3 up = OrientUp(bodyRotation);

		Vec3 targetVelocity = v3(0);
		float jumpForce = 0.0;


		switch (this->state)
		{
			case PlayerState_Ground:
			{
				targetVelocity = (v3(moveDir.x) * right) + (v3(moveDir.z) * forward);



				if (playerInput.jump && characterController.grounded) jumpForce = this->jumpSpeed;

				


				if (velocity.y < 0) velocity.y = 0;

				if (!characterController.grounded) this->state = PlayerState_Air;
				break;
			}

			case PlayerState_Air:
			{
				targetVelocity = (v3(moveDir.x) * right) + (v3(moveDir.z) * forward);

				velocity += v3(0, -10, 0) * v3(deltaTime);

				if (characterController.grounded)
					this->state = PlayerState_Ground;
				break;
			}
		}

		playerInput.jump = false;


		velocity = velocity + ((targetVelocity - velocity) * v3(this->acceleration) * v3(1,0,1)) * v3(deltaTime) + v3(0, jumpForce, 0);

	    Vec3 movement = this->velocity * v3(deltaTime);

		StepCharacter(world, this, movement);
	}


}



void PlayerVisual::Init(World* world) {
	// Init graphics
	this->material = LoadMaterial("models/player.mat");

    // @MemoryManage deal with resource memory handling
	auto model = LoadModel("models/link.smd");
	if (model)
		this->mesh = &model->mesh;
	else
		this->mesh = NULL;
}

void PlayerVisual::Start(World* world) {
	
}

void PlayerVisual::Update(World* world) {
	
}

void PlayerVisual::Destroy(World* world) {
	
}

RegisterEntity(Player);
RegisterEntity(PlayerVisual);