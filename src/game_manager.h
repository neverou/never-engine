#pragma once


#include "combat_manager.h"

struct GameManager
{
	EntityId playerId;


	CombatManager combat;
};

void InitGameManager(GameManager* gameManager);
void DestroyGameManager(GameManager* gameManager);