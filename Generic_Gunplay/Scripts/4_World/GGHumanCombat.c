class GGVehicleCombat
{
	static bool IsInVehicle(PlayerBase player)
	{
		return player && player.IsInTransport();
	}
}

modded class DayZPlayerImplement
{
	protected int m_GGShootFromCameraState = -1;

	override void SwitchOptics(ItemOptics optic, bool state)
	{
		PlayerBase player = PlayerBase.Cast(this);
		Weapon_Base weapon;
		if (player) weapon = Weapon_Base.Cast(player.GetItemInHands());
		if (!optic || !weapon || !weapon.GGShouldApplyGunplay())
		{
			super.SwitchOptics(optic, state);
			return;
		}

		weapon.GGMarkStatsDirty();
		weapon.GGQueueModifierRefresh();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).RemoveByName(optic, "OnOpticEnter");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).RemoveByName(optic, "EnterOptics");
		if (!optic.IsSightOnly() && state)
		{
			if (optic.HasEnergyManager()) optic.GetCompEM().SwitchOn();
			int delay = weapon.GetGGOpticEnterActionDelay();
			string opticMessage = "Entering optic. weapon=" + weapon.GetType() + " optic=" + optic.GetType() + " actionDelayMs=" + delay.ToString();
			GGDebug.Log(7, "OPTICS", opticMessage);
			GGDebug.ClientState(7, "OPTICS", "switch", "enter_" + optic.GetType(), opticMessage);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLaterByName(optic, "EnterOptics", delay, false);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLaterByName(optic, "OnOpticEnter", delay, false);
		}
		else if (!optic.IsSightOnly())
		{
			string exitMessage = "Leaving optic. weapon=" + weapon.GetType() + " optic=" + optic.GetType();
			GGDebug.Log(7, "OPTICS", exitMessage);
			GGDebug.ClientState(7, "OPTICS", "switch", "exit_" + optic.GetType(), exitMessage);
			optic.ExitOptics();
			optic.OnOpticExit();
			if (optic.HasEnergyManager()) optic.GetCompEM().SwitchOff();
		}
		if (m_CameraOptics != state) SetOptics(state);
	}

	override bool IsShootingFromCamera()
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.EnableHipFireAlignment) return GGReleaseShootingOriginOverride();
		PlayerBase player = PlayerBase.Cast(this);
		if (!player || GGVehicleCombat.IsInVehicle(player)) return GGReleaseShootingOriginOverride();
		int crosshairMode = player.GetGGResolvedCrosshairMode();
		if (crosshairMode == 0) return GGApplyShootingOrigin(true);
		Weapon_Base weapon = Weapon_Base.Cast(player.GetItemInHands());
		if (!weapon || !weapon.GGShouldApplyGunplay()) return GGReleaseShootingOriginOverride();
		if (GGIsInAimingView(player, weapon)) return GGApplyShootingOrigin(true);
		if (crosshairMode == 2 && !GGHasHipfireLaser(weapon)) return GGApplyShootingOrigin(true);
		return GGApplyShootingOrigin(false);
	}

	protected bool GGApplyShootingOrigin(bool shootFromCamera)
	{
		int requestedState;
		if (shootFromCamera)
			requestedState = 1;
		else
			requestedState = 0;

		if (m_GGShootFromCameraState != requestedState)
		{
			OverrideShootFromCamera(shootFromCamera);
			m_GGShootFromCameraState = requestedState;
			string originName = "weapon";
			if (shootFromCamera) originName = "camera";
			string originMessage = "Shooting origin override changed to " + originName;
			GGDebug.State(7, "SHOOT_ORIGIN", "override", originName, originMessage);
			GGDebug.ClientState(7, "SHOOT_ORIGIN", "override", originName, originMessage);
		}
		else if (!shootFromCamera && m_IsShootingFromCamera)
		{
			OverrideShootFromCamera(false);
		}

		if (!shootFromCamera) return false;

		return m_IsShootingFromCamera;
	}

	protected bool GGReleaseShootingOriginOverride()
	{
		if (m_GGShootFromCameraState != -1)
		{
			OverrideShootFromCamera(true);
			m_GGShootFromCameraState = -1;
			GGDebug.State(7, "SHOOT_ORIGIN", "override", "vanilla", "Shooting origin override released to vanilla");
			GGDebug.ClientState(7, "SHOOT_ORIGIN", "override", "vanilla", "Shooting origin override released to vanilla");
		}
		return super.IsShootingFromCamera();
	}

	int GGGetCameraShootingOriginState()
	{
		if (m_GGShootFromCameraState < 0) return -1;

		if (m_GGShootFromCameraState == 1 && !m_IsShootingFromCamera) return 0;
		return m_GGShootFromCameraState;
	}

	protected bool GGIsInAimingView(PlayerBase player, Weapon_Base weapon)
	{
		if (m_bADS || m_CameraIronsight || m_CameraOptics) return true;
		if (player && (player.IsInIronsights() || player.IsInOptics())) return true;
		if (weapon && weapon.IsInOptics()) return true;
		return false;
	}

	protected bool GGHasHipfireLaser(Weapon_Base weapon)
	{
		return GGWeaponAttachmentQueries.HasLaser(weapon);
	}
}
