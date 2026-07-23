modded class DayZPlayerImplementAiming
{
	protected bool m_GGHoldBreathFreezeActive;
	protected float m_GGHoldBreathFreezeX;
	protected float m_GGHoldBreathFreezeY;

	override void OnRaiseBegin(DayZPlayerImplement player)
	{
		super.OnRaiseBegin(player);
		if (!player) return;
		Weapon_Base weapon = Weapon_Base.Cast(player.GetHumanInventory().GetEntityInHands());
		if (!weapon || !weapon.GGShouldApplyGunplay()) return;
		weapon.GGMarkStatsDirty();
		weapon.GGQueueModifierRefresh();
		float transitionTime = weapon.GetGGOpticEnterDelay() / 1000.0;
		string transitionState = transitionTime.ToString();
		string transitionMessage = "ADS transition registered. weapon=" + weapon.GetType() + " seconds=" + transitionState;
		GGDebug.State(6, "ADS", GGUtil.Key(weapon.GetType()), transitionState, transitionMessage);
		GGDebug.ClientState(6, "ADS", GGUtil.Key(weapon.GetType()), transitionState, transitionMessage);
		DayZPlayerCameras.RegisterTransitionTime(DayZPlayerCameras.DAYZCAMERA_1ST, DayZPlayerCameras.DAYZCAMERA_IRONSIGHTS, transitionTime, false);
		DayZPlayerCameras.RegisterTransitionTime(DayZPlayerCameras.DAYZCAMERA_1ST, DayZPlayerCameras.DAYZCAMERA_OPTICS, transitionTime, false);
		DayZPlayerCameras.RegisterTransitionTime(DayZPlayerCameras.DAYZCAMERA_IRONSIGHTS, DayZPlayerCameras.DAYZCAMERA_OPTICS, transitionTime, true);
	}

	override bool ProcessAimFilters(float pDt, SDayZPlayerAimingModel pModel, int stance_index)
	{
		vector originalSway = m_SwayModifier;
		ApplyHipFireAimModifier();
		bool result = ProcessGenericFilters(pDt, pModel, stance_index);
		m_SwayModifier = originalSway;
		return result;
	}

	protected void ApplyHipFireAimModifier()
	{
		if (!m_PlayerPb || m_PlayerPb.IsInIronsights() || m_PlayerPb.IsInOptics()) return;
		Weapon_Base weapon = Weapon_Base.Cast(m_PlayerPb.GetItemInHands());
		if (!weapon || !weapon.GGShouldApplyGunplay()) return;
		float hipFire = GGUtil.Clamp(weapon.GetGGHipFireModifier(), 0.05, 5.0);
		m_SwayModifier[0] = m_SwayModifier[0] * hipFire;
		m_SwayModifier[1] = m_SwayModifier[1] * hipFire;
		float hipFireSpeed = GGUtil.Clamp(hipFire, 0.35, 2.0);
		m_SwayModifier[2] = m_SwayModifier[2] * hipFireSpeed;
		if (GGDebug.Enabled(6))
		{
			string state = hipFire.ToString() + "|" + hipFireSpeed.ToString();
			string message = "Applied hipfire aim filter. weapon=" + weapon.GetType();
			message += " sway=" + hipFire.ToString();
			message += " speed=" + hipFireSpeed.ToString();
			GGDebug.State(6, "HIPFIRE", GGUtil.Key(weapon.GetType()), state, message);
			GGDebug.ClientState(6, "HIPFIRE", GGUtil.Key(weapon.GetType()), state, message);
		}
	}

	protected bool ProcessGenericFilters(float dt, SDayZPlayerAimingModel model, int stanceIndex)
	{
		if (!ShouldFreezeHoldBreathSway())
		{
			if (m_GGHoldBreathFreezeActive)
			{
				GGDebug.State(6, "HOLD_BREATH", "freeze", "inactive", "Hold-breath sway freeze released");
				GGDebug.ClientState(6, "HOLD_BREATH", "freeze", "inactive", "Hold-breath sway freeze released");
			}
			m_GGHoldBreathFreezeActive = false;
			return super.ProcessAimFilters(dt, model, stanceIndex);
		}
		if (!m_GGHoldBreathFreezeActive)
		{
			vector currentSway = m_SwayModifier;
			m_SwayModifier[2] = 1.0;
			bool firstResult = super.ProcessAimFilters(dt, model, stanceIndex);
			m_SwayModifier = currentSway;
			m_GGHoldBreathFreezeX = model.m_fAimXHandsOffset;
			m_GGHoldBreathFreezeY = model.m_fAimYHandsOffset;
			m_GGHoldBreathFreezeActive = true;
			string freezeMessage = "Hold-breath sway frozen at current aim point. x=" + m_GGHoldBreathFreezeX.ToString() + " y=" + m_GGHoldBreathFreezeY.ToString();
			GGDebug.State(6, "HOLD_BREATH", "freeze", "active", freezeMessage);
			GGDebug.ClientState(6, "HOLD_BREATH", "freeze", "active", freezeMessage);
			return firstResult;
		}

		bool oldAimNoiseAllowed = IsAimNoiseAllowed();
		vector oldSway = m_SwayModifier;
		float oldHorizontalNoise = m_HorizontalNoiseXAxisOffset;
		float oldBreathingX = m_BreathingXAxisOffset;
		float oldBreathingY = m_BreathingYAxisOffset;
		SetAimNoiseAllowed(false);
		m_SwayModifier[2] = 1.0;
		m_HorizontalNoiseXAxisOffset = 0.0;
		m_BreathingXAxisOffset = 0.0;
		m_BreathingYAxisOffset = 0.0;
		bool result = super.ProcessAimFilters(dt, model, stanceIndex);
		model.m_fAimXHandsOffset += m_GGHoldBreathFreezeX;
		model.m_fAimYHandsOffset += m_GGHoldBreathFreezeY;
		float absAimY = Math.AbsFloat(model.m_fCurrentAimY);
		model.m_fAimYHandsOffset = Math.Clamp(model.m_fAimYHandsOffset, absAimY - 89.9, 89.9 - absAimY);
		SetAimNoiseAllowed(oldAimNoiseAllowed);
		m_SwayModifier = oldSway;
		m_HorizontalNoiseXAxisOffset = oldHorizontalNoise;
		m_BreathingXAxisOffset = oldBreathingX;
		m_BreathingYAxisOffset = oldBreathingY;
		return result;
	}

	protected bool ShouldFreezeHoldBreathSway()
	{
		if (!m_PlayerPb || !m_PlayerPb.IsHoldingBreath()) return false;
		if (!(m_PlayerPb.IsInIronsights() || m_PlayerPb.IsInOptics())) return false;
		Weapon_Base weapon = Weapon_Base.Cast(m_PlayerPb.GetItemInHands());
		return weapon && weapon.GGShouldApplyGunplay();
	}
}
