modded class ItemBase
{
	protected ref Timer m_GGSNAFULaserTimer;

	override void EEInit()
	{
		super.EEInit();
		if (!GGNeedsSNAFULaserCompatibility()) return;

		m_GGSNAFULaserTimer = new Timer;
		m_GGSNAFULaserTimer.Run(0.025, this, "GGUpdateSNAFULaserCompatibility", null, true);
	}

	override void EEDelete(EntityAI parent)
	{
		if (m_GGSNAFULaserTimer) m_GGSNAFULaserTimer.Stop();
		super.EEDelete(parent);
	}

	protected bool GGNeedsSNAFULaserCompatibility()
	{
		if (!g_Game || GetType() != "SNAFU_PEQ1_Laser") return false;
		if (g_Game.IsServer() && g_Game.IsMultiplayer()) return false;
		if (g_Game.ConfigIsExisting("CfgPatches SNAFU_Gunplay")) return false;
		return g_Game.ConfigIsExisting("CfgVehicles SNAFU_PEQ1_Base");
	}

	void GGUpdateSNAFULaserCompatibility()
	{
		if (!g_Game) return;
		if (!HasEnergyManager() || !GetCompEM().IsWorking()) return;

		g_Game.GameScript.CallFunction(this, "StopPeriodicMeasurement", null, 0);

		GGMeasureSNAFULaser();
	}

	protected void GGMeasureSNAFULaser()
	{
		vector from = ModelToWorld(GetMemoryPointPos("beamStart"));
		vector direction;
		if (!GGGetSNAFULaserDirection(direction)) return;

		vector contactPosition;
		vector contactDirection;
		int contactComponent;
		vector to = from + (direction * 600.0);
		if (!DayZPhysics.RaycastRV(from, to, contactPosition, contactDirection, contactComponent, null, null, GetHierarchyRootPlayer(), false, false, ObjIntersectIFire)) return;

		float distance = vector.Distance(contactPosition, from);
		string dotType;
		if (distance < 6.0) dotType = "SNAFU_Laser_Dot1";
		else if (distance < 15.0) dotType = "SNAFU_Laser_Dot2";
		else if (distance < 1000.0) dotType = "SNAFU_Laser_Dot3";
		if (dotType == "") return;

		EntityAI laserDot = EntityAI.Cast(g_Game.CreateObject(dotType, contactPosition, true, false, true));
		if (!laserDot) return;

		string laserColor = "#(argb,8,8,3)color(1,0,0,1.0,co)";
		string laserMaterial = "dz\\weapons\\projectiles\\data\\tracer_red.rvmat";
		g_Game.GameScript.CallFunction(this, "LaserColor", laserColor, 0);
		g_Game.GameScript.CallFunction(this, "LaserMaterial", laserMaterial, 0);
		laserDot.SetObjectTexture(0, laserColor);
		laserDot.SetObjectMaterial(0, laserMaterial);
		laserDot.SetPosition(contactPosition);
		laserDot.Delete();
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
			direction = direction * -1.0;
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
