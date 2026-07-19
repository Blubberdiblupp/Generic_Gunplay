class GGVehicleCombat
{
	static bool IsInVehicle(PlayerBase player)
	{
		return player && player.IsInTransport();
	}
}

modded class DayZPlayerImplement
{
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
		if (weapon.GetPropertyModifierObject()) weapon.GetPropertyModifierObject().UpdateModifiers();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).RemoveByName(optic, "OnOpticEnter");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).RemoveByName(optic, "EnterOptics");
		if (!optic.IsSightOnly() && state)
		{
			if (optic.HasEnergyManager()) optic.GetCompEM().SwitchOn();
			int delay = weapon.GetGGOpticEnterActionDelay();
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLaterByName(optic, "EnterOptics", delay, false);
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLaterByName(optic, "OnOpticEnter", delay, false);
		}
		else if (!optic.IsSightOnly())
		{
			optic.ExitOptics();
			optic.OnOpticExit();
			if (optic.HasEnergyManager()) optic.GetCompEM().SwitchOff();
		}
		if (m_CameraOptics != state) SetOptics(state);
	}

	override bool IsShootingFromCamera()
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.EnableHipFireAlignment) return super.IsShootingFromCamera();
		PlayerBase player = PlayerBase.Cast(this);
		if (!player || GGVehicleCombat.IsInVehicle(player)) return super.IsShootingFromCamera();
		int crosshairMode = player.GetGGResolvedCrosshairMode();
		if (crosshairMode == 0) return super.IsShootingFromCamera();
		Weapon_Base weapon = Weapon_Base.Cast(player.GetItemInHands());
		if (!weapon || !weapon.GGShouldApplyGunplay()) return super.IsShootingFromCamera();
		if (GGIsInAimingView(player, weapon)) return super.IsShootingFromCamera();
		if (crosshairMode == 2 && !GGHasHipfireLaser(weapon)) return super.IsShootingFromCamera();
		return false;
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
