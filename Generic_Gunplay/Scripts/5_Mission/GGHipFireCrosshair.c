modded class CrossHairSelector
{
	override protected void SelectCrossHair()
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		int mode = GetGGConfigManager().GetEffectiveCrosshairMode();
		if (m_Player) mode = m_Player.GetGGResolvedCrosshairMode();
		if (!settings || mode == 0 || !settings.EnableHipFireAlignment)
		{
			super.SelectCrossHair();
			return;
		}
		if (!m_AM) return;

		Weapon_Base weapon = Weapon_Base.Cast(m_Player.GetItemInHands());
		if (!weapon || !weapon.GGShouldApplyGunplay())
		{
			super.SelectCrossHair();
			return;
		}

		HumanInputController controller = m_Player.GetInputController();
		ActionBase action = m_AM.GetRunningAction();
		if (m_Player.IsFireWeaponRaised() && controller && !controller.CameraIsFreeLook() && !m_Player.GetCommand_Melee2())
		{
			ShowCrossHair(null);
			return;
		}
		if (action && action.GetActionCategory() == AC_CONTINUOUS && m_AM.GetActionState(action) != UA_NONE)
		{
			ShowCrossHair(null);
			return;
		}
		if (m_Player.IsRaised() || m_Player.GetCommand_Melee2() || m_Player.GetCommand_Unconscious() || GetGame().GetUIManager().GetMenu() != null)
		{
			ShowCrossHair(null);
			return;
		}
		ShowCrossHair(GetCrossHairByName("dot"));
	}
}

class GGHipFireCrosshair : Managed
{
	protected const float BASE_SIZE = 64.0;
	protected const float MIN_SIZE = 40.0;
	protected const float MAX_SIZE = 88.0;
	protected const float PROJECT_DISTANCE = 100.0;
	protected const float SMOOTHNESS = 0.035;
	protected const float RAYCAST_INTERVAL = 0.025;

	protected Widget m_Root;
	protected Widget m_DynamicRoot;
	protected ImageWidget m_DynamicImage;
	protected PlayerBase m_Player;
	protected Weapon_Base m_RaycastWeapon;
	protected vector m_ScreenPosition;
	protected vector m_CachedWorldPosition;
	protected float m_VelocityX[1];
	protected float m_VelocityY[1];
	protected float m_RaycastElapsed;
	protected bool m_HasPosition;
	protected bool m_HasWorldPosition;

	void GGHipFireCrosshair()
	{
		m_Root = GetGame().GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGHipFireCrosshair.layout");
		if (!m_Root) return;
		m_DynamicRoot = m_Root.FindAnyWidget("GGHipFireCrosshairRoot");
		m_DynamicImage = ImageWidget.Cast(m_Root.FindAnyWidget("GGHipFireCrosshairImage"));
		if (!m_DynamicImage) return;
		m_DynamicImage.LoadImageFile(0, "set:dayz_crosshairs image:crossT_128x128");
		m_DynamicImage.SetImage(0);
		m_DynamicImage.SetColor(ARGB(170, 255, 255, 255));
		m_DynamicImage.Show(false);
	}

	void ~GGHipFireCrosshair()
	{
		if (m_Root) m_Root.Unlink();
	}

	void OnUpdate(float timeslice)
	{
		if (!m_Root || !m_DynamicRoot || !m_DynamicImage) return;
		Weapon_Base weapon;
		if (!CanShow(weapon))
		{
			Hide();
			return;
		}

		m_RaycastElapsed += timeslice;
		bool refreshRaycast = !m_HasWorldPosition;
		if (m_RaycastWeapon != weapon) refreshRaycast = true;
		if (m_RaycastElapsed >= RAYCAST_INTERVAL) refreshRaycast = true;
		if (refreshRaycast)
		{
			vector worldPosition;
			if (!GetWeaponProjectedPosition(weapon, worldPosition))
			{
				Hide();
				return;
			}
			m_CachedWorldPosition = worldPosition;
			m_RaycastWeapon = weapon;
			m_RaycastElapsed = 0.0;
			m_HasWorldPosition = true;
		}

		vector targetPosition = GetGame().GetScreenPosRelative(m_CachedWorldPosition);
		if (!m_HasPosition)
		{
			m_ScreenPosition = targetPosition;
			m_HasPosition = true;
		}
		else
		{
			m_ScreenPosition[0] = Math.SmoothCD(m_ScreenPosition[0], targetPosition[0], m_VelocityX, SMOOTHNESS, 1000.0, timeslice);
			m_ScreenPosition[1] = Math.SmoothCD(m_ScreenPosition[1], targetPosition[1], m_VelocityY, SMOOTHNESS, 1000.0, timeslice);
		}

		float size = Math.Clamp(BASE_SIZE * Math.Clamp(weapon.GetGGHipFireModifier(), 0.45, 1.45), MIN_SIZE, MAX_SIZE);
		m_DynamicImage.SetSize(size, size);
		m_DynamicImage.SetPos(-size * 0.5, -size * 0.5);
		m_DynamicRoot.SetPos(m_ScreenPosition[0], m_ScreenPosition[1]);
		m_DynamicImage.Show(true);
	}

	protected bool CanShow(out Weapon_Base weapon)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		int mode = GetGGConfigManager().GetEffectiveCrosshairMode();
		if (!settings || mode == 0 || !settings.EnableHipFireAlignment) return false;
		if (!g_Game.GetProfileOption(EDayZProfilesOptions.CROSSHAIR) || g_Game.GetWorld().IsCrosshairDisabled()) return false;

		m_Player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!m_Player || !m_Player.IsPlayerSelected() || !m_Player.IsFireWeaponRaised() || m_Player.IsInTransport()) return false;
		mode = m_Player.GetGGResolvedCrosshairMode();
		if (mode == 0) return false;
		HumanInputController controller = m_Player.GetInputController();
		if (!controller || controller.CameraIsFreeLook()) return false;
		if (m_Player.IsInIronsights()) return false;
		if (m_Player.IsInOptics()) return false;
		if (m_Player.GetCommand_Melee2()) return false;
		if (GetGame().IsInventoryOpen()) return false;
		if (GetGame().GetUIManager().GetMenu() != null) return false;
		if (!Class.CastTo(weapon, m_Player.GetItemInHands()) || !weapon.GGShouldApplyGunplay() || weapon.IsInOptics()) return false;
		if (mode == 2 && !HasHipfireLaser(weapon)) return false;
		return true;
	}

	protected bool HasHipfireLaser(Weapon_Base weapon)
	{
		return GGWeaponAttachmentQueries.HasLaser(weapon);
	}

	protected bool GetWeaponProjectedPosition(Weapon_Base weapon, out vector worldPosition)
	{
		vector barrelPosition = weapon.GetSelectionPositionLS("konec hlavne");
		vector muzzlePosition = weapon.GetSelectionPositionLS("usti hlavne");
		vector beginPoint = weapon.ModelToWorld(barrelPosition);
		vector endPoint = weapon.ModelToWorld(muzzlePosition);
		vector direction = endPoint - beginPoint;
		if (direction.LengthSq() < 0.0001) return false;
		direction.Normalize();
		vector traceEnd = endPoint + (direction * PROJECT_DISTANCE);
		vector contactDirection;
		int contactComponent;
		if (DayZPhysics.RaycastRV(endPoint, traceEnd, worldPosition, contactDirection, contactComponent, null, weapon, m_Player, false, false, ObjIntersectFire)) return true;
		worldPosition = traceEnd;
		return true;
	}

	protected void Hide()
	{
		m_HasPosition = false;
		m_HasWorldPosition = false;
		m_RaycastElapsed = 0.0;
		m_RaycastWeapon = null;
		if (m_DynamicImage) m_DynamicImage.Show(false);
	}
}

modded class IngameHud
{
	protected ref GGHipFireCrosshair m_GGHipFireCrosshair;

	void IngameHud()
	{
		m_GGHipFireCrosshair = new GGHipFireCrosshair();
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_GGHipFireCrosshair) m_GGHipFireCrosshair.OnUpdate(timeslice);
	}
}
