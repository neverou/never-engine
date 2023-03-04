#pragma once

struct CombatManager
{
	DynArray<EntityId> entities;
};



void InitCombatManager(CombatManager* combatManager);
void DestroyCombatManager(CombatManager* combatManager);

void RegisterCombat(CombatManager* combatManager, Entity* entity);
void UnregisterCombat(CombatManager* combatManager, Entity* entity);


// ~Note Combat manager will also deal with hitboxes and attacking mechanics later on

// Returns true if damage was successfully applied
bool Damage(CombatManager* cm, Entity* source, Entity* target, int hitpoints);