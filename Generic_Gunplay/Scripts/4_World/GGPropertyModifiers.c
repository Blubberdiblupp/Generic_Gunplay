modded class PropertyModifiers
{
	override void UpdateModifiers()
	{
		super.UpdateModifiers();
		Weapon_Base weapon = Weapon_Base.Cast(m_OwnerItem);
		if (!weapon || !weapon.GGShouldApplyGunplay()) return;

		float recoil = weapon.GetGGWeaponRecoilModifier();
		float sway = weapon.GetGGAimingSwayModifier();
		float swaySpeed = weapon.GetGGAimingSwaySpeedModifier();
		if (recoil > 0.0)
		{
			m_RecoilModifiers[0] = recoil;
			m_RecoilModifiers[1] = recoil;
			m_RecoilModifiers[2] = recoil;
		}
		if (sway > 0.0)
		{
			m_SwayModifiers[0] = sway;
			m_SwayModifiers[1] = sway;
			m_SwayModifiers[2] = swaySpeed;
		}
		if (GGDebug.Enabled(6))
		{
			string state = recoil.ToString() + "|" + sway.ToString() + "|" + swaySpeed.ToString();
			string message = "Applied property modifiers. weapon=" + weapon.GetType();
			message += " recoil=" + recoil.ToString();
			message += " sway=" + sway.ToString();
			message += " swaySpeed=" + swaySpeed.ToString();
			GGDebug.State(6, "MODIFIERS", GGUtil.Key(weapon.GetType()), state, message);
			GGDebug.ClientState(6, "MODIFIERS", GGUtil.Key(weapon.GetType()), state, message);
		}
	}
}
