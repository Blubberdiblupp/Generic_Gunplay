modded class ItemBase
{
	protected ref Timer m_GGSNAFULaserWatchTimer;
	protected ref Timer m_GGSNAFULaserUpdateTimer;
	protected EntityAI m_GGSNAFULaserDot;
	protected string m_GGSNAFULaserDotType;
	protected bool m_GGSNAFUOriginalTimerStopped;

	override void EEInit()
	{
		super.EEInit();
		if (!GGNeedsSNAFULaserCompatibility()) return;

		m_GGSNAFULaserWatchTimer = new Timer;
		m_GGSNAFULaserWatchTimer.Run(0.25, this, "GGRefreshSNAFULaserCompatibility", null, true);
		GGRefreshSNAFULaserCompatibility();
		GGDebug.ClientOnce(7, "LASER", "snafu_compatibility", "SNAFU PEQ compatibility watcher activated");
	}

	override void EEDelete(EntityAI parent)
	{
		if (m_GGSNAFULaserWatchTimer) m_GGSNAFULaserWatchTimer.Stop();
		if (m_GGSNAFULaserUpdateTimer) m_GGSNAFULaserUpdateTimer.Stop();
		GGClearSNAFULaserDot();
		super.EEDelete(parent);
	}

	protected bool GGNeedsSNAFULaserCompatibility()
	{
		if (!g_Game || GetType() != "SNAFU_PEQ1_Laser") return false;
		if (g_Game.IsServer() && g_Game.IsMultiplayer()) return false;
		if (g_Game.ConfigIsExisting("CfgPatches SNAFU_Gunplay")) return false;
		return g_Game.ConfigIsExisting("CfgVehicles SNAFU_PEQ1_Base");
	}

	void GGRefreshSNAFULaserCompatibility()
	{
		if (!g_Game) return;
		bool working = HasEnergyManager() && GetCompEM().IsWorking();
		if (!working)
		{
			m_GGSNAFUOriginalTimerStopped = false;
			if (m_GGSNAFULaserUpdateTimer) m_GGSNAFULaserUpdateTimer.Stop();
			GGClearSNAFULaserDot();
			return;
		}

		if (!m_GGSNAFUOriginalTimerStopped)
		{
			g_Game.GameScript.CallFunction(this, "StopPeriodicMeasurement", null, 0);
			m_GGSNAFUOriginalTimerStopped = true;
		}

		if (!m_GGSNAFULaserUpdateTimer) m_GGSNAFULaserUpdateTimer = new Timer;
		if (!m_GGSNAFULaserUpdateTimer.IsRunning())
			m_GGSNAFULaserUpdateTimer.Run(0.025, this, "GGUpdateSNAFULaserCompatibility", null, true);
	}

	void GGUpdateSNAFULaserCompatibility()
	{
		if (!g_Game) return;
		if (!HasEnergyManager() || !GetCompEM().IsWorking())
		{
			GGRefreshSNAFULaserCompatibility();
			return;
		}
		if (GGDebug.Enabled(10))
			GGDebug.ClientCount(10, "LASER", "snafu_updates", 10000);
		GGMeasureSNAFULaser();
	}

	protected void GGMeasureSNAFULaser()
	{
		vector from = ModelToWorld(GetMemoryPointPos("beamStart"));
		vector direction;
		if (!GGGetSNAFULaserDirection(direction))
		{
			GGClearSNAFULaserDot();
			return;
		}

		vector contactPosition;
		vector contactDirection;
		int contactComponent;
		vector to = from + (direction * 600.0);
		if (GGDebug.Enabled(9))
			GGDebug.ClientCount(9, "LASER", "snafu_raycasts", 10000);
		bool hit = DayZPhysics.RaycastRV(from, to, contactPosition, contactDirection, contactComponent, null, null, GetHierarchyRootPlayer(), false, false, ObjIntersectIFire);
		if (!hit)
		{
			GGClearSNAFULaserDot();
			return;
		}

		float distance = vector.Distance(contactPosition, from);
		string dotType;
		if (distance < 6.0) dotType = "SNAFU_Laser_Dot1";
		else if (distance < 15.0) dotType = "SNAFU_Laser_Dot2";
		else if (distance < 1000.0) dotType = "SNAFU_Laser_Dot3";
		if (dotType == "")
		{
			GGClearSNAFULaserDot();
			return;
		}

		if (!GGEnsureSNAFULaserDot(dotType, contactPosition)) return;
		m_GGSNAFULaserDot.SetPosition(contactPosition);
	}

	protected bool GGEnsureSNAFULaserDot(string dotType, vector position)
	{
		if (m_GGSNAFULaserDot && m_GGSNAFULaserDotType == dotType) return true;
		GGClearSNAFULaserDot();

		m_GGSNAFULaserDot = EntityAI.Cast(g_Game.CreateObject(dotType, position, true, false, true));
		if (!m_GGSNAFULaserDot) return false;
		m_GGSNAFULaserDotType = dotType;

		string laserColor = "#(argb,8,8,3)color(1,0,0,1.0,co)";
		string laserMaterial = "dz\\weapons\\projectiles\\data\\tracer_red.rvmat";
		g_Game.GameScript.CallFunction(this, "LaserColor", laserColor, 0);
		g_Game.GameScript.CallFunction(this, "LaserMaterial", laserMaterial, 0);
		m_GGSNAFULaserDot.SetObjectTexture(0, laserColor);
		m_GGSNAFULaserDot.SetObjectMaterial(0, laserMaterial);
		return true;
	}

	protected void GGClearSNAFULaserDot()
	{
		if (m_GGSNAFULaserDot) m_GGSNAFULaserDot.Delete();
		m_GGSNAFULaserDot = null;
		m_GGSNAFULaserDotType = "";
	}

	protected bool GGGetSNAFULaserDirection(out vector direction)
	{
		vector startWorld = ModelToWorld(GetMemoryPointPos("beamStart"));
		vector endWorld = ModelToWorld(GetMemoryPointPos("beamEnd"));
		direction = endWorld - startWorld;
		if (direction.LengthSq() < 0.0001) direction = GetDirection();
		if (direction.LengthSq() < 0.0001) return false;
		direction.Normalize();

		Weapon_Base parentWeapon = GGGetSNAFULaserParentWeapon();
		if (parentWeapon && vector.Dot(direction, parentWeapon.GetDirection()) < 0.0)
		{
			direction = direction * -1.0;
			GGDebug.ClientRateLimited(7, "LASER", "snafu_direction_flip", 5000, "Corrected reversed SNAFU PEQ laser direction. weapon=" + parentWeapon.GetType());
		}
		return true;
	}

	protected Weapon_Base GGGetSNAFULaserParentWeapon()
	{
		EntityAI parent = GetHierarchyParent();
		for (int depth = 0; parent && depth < 32; depth++)
		{
			Weapon_Base weapon = Weapon_Base.Cast(parent);
			if (weapon) return weapon;
			parent = parent.GetHierarchyParent();
		}
		return null;
	}
}
