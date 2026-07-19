modded class Weapon_Base
{
	protected ref GGWeaponStatsManager m_GGStatsManager;
	protected bool m_GGStatsDirty = true;
	protected string m_GGLastFireMode;
	protected int m_GGConfigRevision = -1;

	void Weapon_Base()
	{
		m_GGStatsManager = new GGWeaponStatsManager();
		m_GGLastFireMode = "";
	}

	override void EEItemAttached(EntityAI item, string slot_name)
	{
		m_GGStatsDirty = true;
		super.EEItemAttached(item, slot_name);
	}

	override void EEItemDetached(EntityAI item, string slot_name)
	{
		m_GGStatsDirty = true;
		super.EEItemDetached(item, slot_name);
	}

	override void OnVariablesSynchronized()
	{
		m_GGStatsDirty = true;
		super.OnVariablesSynchronized();
	}

	override void OnFireModeChange(int fireMode)
	{
		super.OnFireModeChange(fireMode);
		m_GGStatsDirty = true;
		GGEnsureStats();
		if (GGShouldApplyGunplay() && GetPropertyModifierObject()) GetPropertyModifierObject().UpdateModifiers();
		PlayerBase player = PlayerBase.Cast(GetHierarchyRootPlayer());
		if (player && GGShouldApplyGunplay())
		{
			float transitionTime = GetGGOpticEnterDelay() / 1000.0;
			DayZPlayerCameras.RegisterTransitionTime(DayZPlayerCameras.DAYZCAMERA_1ST, DayZPlayerCameras.DAYZCAMERA_IRONSIGHTS, transitionTime, false);
			DayZPlayerCameras.RegisterTransitionTime(DayZPlayerCameras.DAYZCAMERA_1ST, DayZPlayerCameras.DAYZCAMERA_OPTICS, transitionTime, false);
			DayZPlayerCameras.RegisterTransitionTime(DayZPlayerCameras.DAYZCAMERA_IRONSIGHTS, DayZPlayerCameras.DAYZCAMERA_OPTICS, transitionTime, true);
		}
	}

	override RecoilBase SpawnRecoilObject()
	{
		GGEnsureStats();
		if (GGShouldApplyGunplay() && GetPropertyModifierObject()) GetPropertyModifierObject().UpdateModifiers();
		return super.SpawnRecoilObject();
	}

	void GGMarkStatsDirty()
	{
		m_GGStatsDirty = true;
	}

	void GGEnsureStats()
	{
		int configRevision = GetGGConfigManager().GetRuntimeRevision();
		if (configRevision != m_GGConfigRevision)
		{
			m_GGConfigRevision = configRevision;
			m_GGStatsDirty = true;
		}

		string currentMode = GetCurrentModeName(GetCurrentMuzzle());
		if (currentMode != m_GGLastFireMode)
		{
			m_GGLastFireMode = currentMode;
			m_GGStatsDirty = true;
		}
		if (!m_GGStatsDirty) return;
		if (!m_GGStatsManager) m_GGStatsManager = new GGWeaponStatsManager();
		m_GGStatsManager.Calculate(this);
		m_GGStatsDirty = false;
	}

	bool GGShouldApplyGunplay()
	{
		return GetGGConfigManager().GetWeapon(GetType()) != null;
	}

	GGResolvedWeaponStats GetGGResolvedStats()
	{
		GGEnsureStats();
		if (m_GGStatsManager) return m_GGStatsManager.GetStats();
		return null;
	}

	float GetGGWeaponRecoilModifier()
	{
		if (!GGShouldApplyGunplay()) return 1.0;
		GGResolvedWeaponStats stats = GetGGResolvedStats();
		if (stats) return stats.Recoil;
		return 1.0;
	}

	float GetGGAimingSwayModifier()
	{
		if (!GGShouldApplyGunplay()) return 1.0;
		GGResolvedWeaponStats stats = GetGGResolvedStats();
		if (!stats) return 1.0;
		return stats.EffectiveSway;
	}

	float GetGGAimingSwaySpeedModifier()
	{
		if (!GGShouldApplyGunplay()) return 1.0;
		GGResolvedWeaponStats stats = GetGGResolvedStats();
		if (!stats) return 1.0;
		return stats.EffectiveSwaySpeed;
	}

	float GetGGAimSpeedModifier()
	{
		if (!GGShouldApplyGunplay()) return 1.0;
		GGResolvedWeaponStats stats = GetGGResolvedStats();
		if (stats) return stats.ADS;
		return 1.0;
	}

	float GetGGHipFireModifier()
	{
		if (!GGShouldApplyGunplay()) return 1.0;
		GGResolvedWeaponStats stats = GetGGResolvedStats();
		if (stats) return stats.HipFire;
		return 1.0;
	}

	int GetGGOpticEnterDelay()
	{
		float aimSpeed = GetGGAimSpeedModifier();
		if (aimSpeed <= 0.05) aimSpeed = 1.0;
		return Math.Round(GGUtil.Clamp(700.0 / aimSpeed, 150.0, 1400.0));
	}

	int GetGGOpticEnterActionDelay()
	{
		return Math.Max(0, GetGGOpticEnterDelay() - 700);
	}
}

modded class ItemBase
{
	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);
		GGInvalidateOwningWeaponStats();
	}

	override void EEItemDetached(EntityAI item, string slot_name)
	{
		super.EEItemDetached(item, slot_name);
		GGInvalidateOwningWeaponStats();
	}

	protected void GGInvalidateOwningWeaponStats()
	{
		EntityAI parent = GetHierarchyParent();
		for (int depth = 0; parent && depth < 32; depth++)
		{
			Weapon_Base weapon = Weapon_Base.Cast(parent);
			if (weapon)
			{
				weapon.GGMarkStatsDirty();
				if (weapon.GetPropertyModifierObject()) weapon.GetPropertyModifierObject().UpdateModifiers();
				return;
			}
			parent = parent.GetHierarchyParent();
		}
	}
}
