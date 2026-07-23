modded class Weapon_Base
{
	protected ref GGWeaponStatsManager m_GGStatsManager;
	protected bool m_GGStatsDirty = true;
	protected string m_GGLastFireMode;
	protected int m_GGConfigRevision = -1;
	protected int m_GGAttachmentTreeRevision;
	protected int m_GGLaserCacheAttachmentRevision = -1;
	protected int m_GGLaserCacheConfigRevision = -1;
	protected bool m_GGLaserCacheValue;
	protected bool m_GGModifierRefreshQueued;

	void Weapon_Base()
	{
		m_GGStatsManager = new GGWeaponStatsManager();
		m_GGLastFireMode = "";
		m_GGAttachmentTreeRevision = 0;
	}

	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);
		GGMarkAttachmentTreeDirty();
		if (item)
		{
			string message = "Attached " + item.GetType() + " to " + GetType() + " slot=" + slot_name;
			GGDebug.Log(8, "ATTACHMENT", message);
			GGDebug.ClientState(8, "ATTACHMENT", "weapon_" + GGUtil.Key(GetType()) + "_slot_" + GGUtil.Key(slot_name), item.GetType(), message);
		}
	}

	override void EEItemDetached(EntityAI item, string slot_name)
	{
		super.EEItemDetached(item, slot_name);
		GGMarkAttachmentTreeDirty();
		string itemType = "unknown";
		if (item) itemType = item.GetType();
		string message = "Detached " + itemType + " from " + GetType() + " slot=" + slot_name;
		GGDebug.Log(8, "ATTACHMENT", message);
		GGDebug.ClientState(8, "ATTACHMENT", "weapon_" + GGUtil.Key(GetType()) + "_slot_" + GGUtil.Key(slot_name), "empty", message);
	}

	override void OnFireModeChange(int fireMode)
	{
		super.OnFireModeChange(fireMode);
		m_GGStatsDirty = true;
		string modeMessage = "Fire mode changed. weapon=" + GetType() + " index=" + fireMode.ToString();
		GGDebug.Log(7, "WEAPON", modeMessage);
		GGDebug.ClientState(7, "WEAPON", "firemode_" + GGUtil.Key(GetType()), fireMode.ToString(), modeMessage);
		GGEnsureStats();
		GGQueueModifierRefresh();
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
		if (GGDebug.Enabled(10))
		{
			GGDebug.Count(10, "WEAPON", "recoil_objects_" + GGUtil.Key(GetType()), 10000);
			GGDebug.ClientCount(10, "WEAPON", "recoil_objects_" + GGUtil.Key(GetType()), 10000);
		}
		return super.SpawnRecoilObject();
	}

	void GGMarkStatsDirty()
	{
		m_GGStatsDirty = true;
		GGDebug.Count(9, "CACHE", "weapon_stats_invalidations", 10000);
		GGDebug.ClientCount(9, "CACHE", "weapon_stats_invalidations", 10000);
	}

	void GGMarkAttachmentTreeDirty()
	{
		m_GGAttachmentTreeRevision++;
		m_GGLaserCacheAttachmentRevision = -1;
		GGMarkStatsDirty();
		GGQueueModifierRefresh();
	}

	int GetGGAttachmentTreeRevision()
	{
		return m_GGAttachmentTreeRevision;
	}

	bool GGHasLaserAttachmentCached()
	{
		int configRevision = GetGGConfigManager().GetRuntimeRevision();
		bool attachmentCacheValid = m_GGLaserCacheAttachmentRevision == m_GGAttachmentTreeRevision;
		bool configCacheValid = m_GGLaserCacheConfigRevision == configRevision;
		if (attachmentCacheValid && configCacheValid) return m_GGLaserCacheValue;

		m_GGLaserCacheValue = GGWeaponAttachmentQueries.ScanHasLaser(this);
		m_GGLaserCacheAttachmentRevision = m_GGAttachmentTreeRevision;
		m_GGLaserCacheConfigRevision = configRevision;
		return m_GGLaserCacheValue;
	}

	void GGQueueModifierRefresh()
	{
		if (m_GGModifierRefreshQueued || !g_Game || !GGShouldApplyGunplay()) return;
		m_GGModifierRefreshQueued = true;
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(GGApplyQueuedModifierRefresh);
	}

	protected void GGApplyQueuedModifierRefresh()
	{
		m_GGModifierRefreshQueued = false;
		if (!GGShouldApplyGunplay()) return;
		GGEnsureStats();
		if (GetPropertyModifierObject()) GetPropertyModifierObject().UpdateModifiers();
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
		GGDebug.Log(8, "CACHE", "Recalculating weapon stats. weapon=" + GetType() + " revision=" + m_GGConfigRevision.ToString() + " mode=" + currentMode);
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
				weapon.GGMarkAttachmentTreeDirty();
				return;
			}
			parent = parent.GetHierarchyParent();
		}
	}
}
