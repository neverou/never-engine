#include "combat_manager.h"

void InitCombatManager(CombatManager* combatManager)
{
	combatManager->entities = MakeDynArray<EntityId>();
}

void DestroyCombatManager(CombatManager* combatManager)
{
	FreeDynArray(&combatManager->entities);
}



void RegisterCombat(CombatManager* combatManager, Entity* entity)
{
	Assert(entity != NULL);

	// Make sure the entity isnt already registered for combat
	Assert((entity->features & EntityFeature_Combat) == 0);
	Assert(ArrayFind(&combatManager->entities, entity->id) == -1);

	entity->features |= EntityFeature_Combat;
	ArrayAdd(&combatManager->entities, entity->id);
}

void UnregisterCombat(CombatManager* combatManager, Entity* entity)
{
	Assert(entity != NULL);
	Assert((entity->features & EntityFeature_Combat) != 0);

	u64 idx = ArrayFind(&combatManager->entities, entity->id);
	if (idx != -1)
	{
		ArrayRemoveAt(&combatManager->entities, idx);
		entity->features &= ~EntityFeature_Combat; // Remove the combat flag on the entity

		Assert((entity->features & EntityFeature_Combat) == 0);
	}
	else
		LogWarn("[combat] Failed to unregister combat for entity (%lu), because they were not in the combat array!", entity->id);
}



// ~Note we may want like a "DamageType" that specifies the type of damage applied (assuming we care)
bool Damage(CombatManager* cm, Entity* source, Entity* target, int hitpoints)
{
	Assert(cm     != NULL);
	Assert(target != NULL);

	// If the target entity doesn't have combat enabled then damaging it wont do anything!
	if ((target->features & EntityFeature_Combat) == 0) return false;

	AssertMsg(hitpoints >= 0, "Cannot apply a negative amount of damage!"); // use the Heal() function instead!

	target->combatInfo.health -= hitpoints;

	return true;
}