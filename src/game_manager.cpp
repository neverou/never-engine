#include "game_manager.h"

void InitGameManager(GameManager* gameManager)
{
	InitCombatManager(&gameManager->combat);
}

void DestroyGameManager(GameManager* gameManager)
{
	DestroyCombatManager(&gameManager->combat);
}