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
	protected const int STYLE_COUNT = 6;

	protected ref array<Widget> m_StyleLayouts;
	protected ref array<Widget> m_StyleRoots;
	protected ref array<ImageWidget> m_StyleImages;
	protected Widget m_DynamicRoot;
	protected ImageWidget m_DynamicImage;
	protected PlayerBase m_Player;
	protected Weapon_Base m_RaycastWeapon;
	protected vector m_ScreenPosition;
	protected vector m_CachedWorldPosition;
	protected float m_VelocityX[1];
	protected float m_VelocityY[1];
	protected float m_RaycastElapsed;
	protected int m_AppliedColor = -1;
	protected int m_AppliedOpacity = -1;
	protected int m_AppliedStyle = -1;
	protected float m_AppliedSize = -1.0;
	protected bool m_HasPosition;
	protected bool m_HasWorldPosition;
	protected bool m_IsVisible;

	void GGHipFireCrosshair()
	{
		m_StyleLayouts = new array<Widget>();
		m_StyleRoots = new array<Widget>();
		m_StyleImages = new array<ImageWidget>();

		if (!AddCrosshairStyle("Generic_Gunplay/GUI/layouts/GGHipFireCrosshair.layout", "GGHipFireCrosshairRoot", "GGHipFireCrosshairImage")) return;
		if (!AddCrosshairStyle("Generic_Gunplay/GUI/layouts/GGHipFireCrosshairDot.layout", "GGHipFireCrosshairDotRoot", "GGHipFireCrosshairDotImage")) return;
		if (!AddCrosshairStyle("Generic_Gunplay/GUI/layouts/GGHipFireCrosshairOpenCross.layout", "GGHipFireCrosshairOpenCrossRoot", "GGHipFireCrosshairOpenCrossImage")) return;
		if (!AddCrosshairStyle("Generic_Gunplay/GUI/layouts/GGHipFireCrosshairTacticalT.layout", "GGHipFireCrosshairTacticalTRoot", "GGHipFireCrosshairTacticalTImage")) return;
		if (!AddCrosshairStyle("Generic_Gunplay/GUI/layouts/GGHipFireCrosshairBrackets.layout", "GGHipFireCrosshairBracketsRoot", "GGHipFireCrosshairBracketsImage")) return;
		if (!AddCrosshairStyle("Generic_Gunplay/GUI/layouts/GGHipFireCrosshairCrossDot.layout", "GGHipFireCrosshairCrossDotRoot", "GGHipFireCrosshairCrossDotImage")) return;

		UpdateAppearance();
		HideStyleWidgets();
		GGDebug.ClientOnce(7, "CROSSHAIR", "layouts_ready", "All six dynamic crosshair layouts initialized");
	}

	void ~GGHipFireCrosshair()
	{
		if (!m_StyleLayouts) return;
		for (int index = 0; index < m_StyleLayouts.Count(); index++)
		{
			Widget layoutRoot = m_StyleLayouts.Get(index);
			if (layoutRoot) layoutRoot.Unlink();
		}
	}

	void OnUpdate(float timeslice)
	{
		if (!IsReady()) return;
		if (GGDebug.Enabled(10))
			GGDebug.ClientCount(10, "CROSSHAIR", "hud_updates", 10000);
		UpdateAppearance();
		Weapon_Base weapon;
		if (!CanShow(weapon))
		{
			GGDebug.ClientState(7, "CROSSHAIR", "visibility", "hidden", "Dynamic crosshair hidden");
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
		if (!ApplyStyle(size))
		{
			Hide();
			return;
		}
		if (GGDebug.Enabled(7))
		{
			string visibleState = "visible|" + weapon.GetType();
			visibleState += "|" + size.ToString();
			string visibleMessage = "Dynamic crosshair visible. weapon=" + weapon.GetType();
			visibleMessage += " size=" + size.ToString();
			GGDebug.ClientState(7, "CROSSHAIR", "visibility", visibleState, visibleMessage);
		}
		m_DynamicRoot.SetPos(m_ScreenPosition[0], m_ScreenPosition[1]);
	}

	protected void UpdateAppearance()
	{
		if (!IsReady()) return;
		GGClientSettings clientSettings = GetGGConfigManager().GetClientSettings();
		int colorIndex = 0;
		int opacityIndex = 2;
		if (clientSettings)
		{
			colorIndex = Math.Clamp(clientSettings.CrosshairColor, 0, 5);
			opacityIndex = Math.Clamp(clientSettings.CrosshairOpacity, 0, 3);
		}
		if (colorIndex == m_AppliedColor && opacityIndex == m_AppliedOpacity) return;

		int alpha = 191;
		switch (opacityIndex)
		{
			case 0: alpha = 64; break;
			case 1: alpha = 128; break;
			case 2: alpha = 191; break;
			case 3: alpha = 255; break;
		}

		int red = 255;
		int green = 255;
		int blue = 255;
		switch (colorIndex)
		{
			case 1: red = 255; green = 70; blue = 70; break;
			case 2: red = 70; green = 255; blue = 110; break;
			case 3: red = 70; green = 235; blue = 255; break;
			case 4: red = 255; green = 225; blue = 60; break;
			case 5: red = 255; green = 145; blue = 40; break;
		}

		int color = ARGB(alpha, red, green, blue);
		for (int index = 0; index < m_StyleImages.Count(); index++)
		{
			ImageWidget image = m_StyleImages.Get(index);
			if (image) image.SetColor(color);
		}
		m_AppliedColor = colorIndex;
		m_AppliedOpacity = opacityIndex;
		GGDebug.ClientState(7, "CROSSHAIR", "appearance", colorIndex.ToString() + "|" + opacityIndex.ToString(), "Crosshair appearance changed. color=" + colorIndex.ToString() + " opacity=" + opacityIndex.ToString());
	}

	protected bool ApplyStyle(float size)
	{
		GGClientSettings clientSettings = GetGGConfigManager().GetClientSettings();
		int style = 0;
		if (clientSettings) style = Math.Clamp(clientSettings.CrosshairStyle, 0, 5);
		if (GGDebug.Enabled(7))
			GGDebug.ClientState(7, "CROSSHAIR", "style", style.ToString(), "Crosshair style selected");

		if (style != m_AppliedStyle || !m_DynamicRoot || !m_DynamicImage)
		{
			if (m_DynamicImage) m_DynamicImage.Show(false);
			m_DynamicRoot = m_StyleRoots.Get(style);
			m_DynamicImage = m_StyleImages.Get(style);
			m_AppliedStyle = style;
			m_AppliedSize = -1.0;
			m_IsVisible = false;
		}
		if (!m_DynamicRoot || !m_DynamicImage) return false;
		if (!GGUtil.NearlyEqual(size, m_AppliedSize, 0.01))
		{
			m_DynamicImage.SetSize(size, size);
			m_DynamicImage.SetPos(-size * 0.5, -size * 0.5);
			m_AppliedSize = size;
		}
		if (!m_IsVisible)
		{
			m_DynamicImage.Show(true);
			m_IsVisible = true;
		}
		return true;
	}

	protected void HideStyleWidgets()
	{
		if (m_StyleImages)
		{
			for (int index = 0; index < m_StyleImages.Count(); index++)
			{
				ImageWidget image = m_StyleImages.Get(index);
				if (image) image.Show(false);
			}
		}
		m_DynamicRoot = null;
		m_DynamicImage = null;
		m_AppliedStyle = -1;
		m_AppliedSize = -1.0;
		m_IsVisible = false;
	}

	protected bool AddCrosshairStyle(string layoutPath, string rootName, string imageName)
	{
		Widget layoutRoot = GetGame().GetWorkspace().CreateWidgets(layoutPath);
		if (!layoutRoot) return false;
		Widget styleRoot = layoutRoot.FindAnyWidget(rootName);
		ImageWidget styleImage = ImageWidget.Cast(layoutRoot.FindAnyWidget(imageName));
		if (!styleRoot || !styleImage)
		{
			layoutRoot.Unlink();
			return false;
		}

		styleImage.SetImage(0);
		styleImage.Show(false);
		m_StyleLayouts.Insert(layoutRoot);
		m_StyleRoots.Insert(styleRoot);
		m_StyleImages.Insert(styleImage);
		return true;
	}

	protected bool IsReady()
	{
		if (!m_StyleLayouts || !m_StyleRoots || !m_StyleImages) return false;
		if (m_StyleLayouts.Count() != STYLE_COUNT) return false;
		if (m_StyleRoots.Count() != STYLE_COUNT) return false;
		if (m_StyleImages.Count() != STYLE_COUNT) return false;
		return true;
	}

	protected bool CanShow(out Weapon_Base weapon)
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.EnableHipFireAlignment) return false;
		if (!g_Game.GetProfileOption(EDayZProfilesOptions.CROSSHAIR) || g_Game.GetWorld().IsCrosshairDisabled()) return false;

		m_Player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!m_Player || !m_Player.IsPlayerSelected() || !m_Player.IsFireWeaponRaised() || m_Player.IsInTransport()) return false;
		int mode = m_Player.GetGGResolvedCrosshairMode();
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
		if (GGDebug.Enabled(9))
			GGDebug.ClientCount(9, "CROSSHAIR", "raycasts", 10000);
		if (DayZPhysics.RaycastRV(endPoint, traceEnd, worldPosition, contactDirection, contactComponent, null, weapon, m_Player, false, false, ObjIntersectFire)) return true;
		worldPosition = traceEnd;
		return true;
	}

	protected void Hide()
	{
		bool alreadyHidden = !m_IsVisible;
		if (alreadyHidden && !m_HasPosition && !m_HasWorldPosition && !m_RaycastWeapon) return;
		m_HasPosition = false;
		m_HasWorldPosition = false;
		m_RaycastElapsed = 0.0;
		m_RaycastWeapon = null;
		if (m_DynamicImage && m_IsVisible) m_DynamicImage.Show(false);
		m_IsVisible = false;
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
