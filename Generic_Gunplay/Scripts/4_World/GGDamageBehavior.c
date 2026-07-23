modded class Weapon_Base
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage && damageType == DamageType.FIRE_ARM)
		{
			GGDebug.Count(8, "DAMAGE", "weapon_firearm_damage_blocked", 10000);
			return false;
		}
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override void SetHealth(float health)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage && health < GetHealth())
		{
			PlayerBase player = PlayerBase.Cast(GetHierarchyRootPlayer());
			if (player && GetHierarchyRoot() == player.GetHumanInventory().GetEntityInHands())
			{
				GGDebug.Count(8, "DAMAGE", "held_weapon_health_loss_blocked", 10000);
				return;
			}
		}
		super.SetHealth(health);
	}

	override void EEKilled(Object killer)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && !settings.EnableWeaponGeometryDamage)
		{
			GGDebug.Count(8, "DAMAGE", "weapon_kill_blocked", 10000);
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
		if (settings && !settings.EnableWeaponGeometryDamage && damageType == DamageType.FIRE_ARM)
		{
			GGDebug.Count(8, "DAMAGE", "magazine_firearm_damage_blocked", 10000);
			return false;
		}
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
}
