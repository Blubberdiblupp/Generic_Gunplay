modded class Weapon_Base
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage && damageType == DamageType.FIRE_ARM) return false;
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override void SetHealth(float health)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage && health < GetHealth())
		{
			PlayerBase player = PlayerBase.Cast(GetHierarchyRootPlayer());
			if (player && GetHierarchyRoot() == player.GetHumanInventory().GetEntityInHands()) return;
		}
		super.SetHealth(health);
	}

	override void EEKilled(Object killer)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage)
		{
			SetHealth(GetMaxHealth());
			return;
		}
		super.EEKilled(killer);
	}
}

modded class MagazineStorage : Magazine
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage && damageType == DamageType.FIRE_ARM) return false;
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
}
