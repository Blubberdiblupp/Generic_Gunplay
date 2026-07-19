class GGResolvedWeaponStats : Managed
{
	float BaseRecoil;
	float BaseSway;
	float BaseADS;
	float BasePrecision;
	float Recoil;
	float Sway;
	float EffectiveSway;
	float EffectiveSwaySpeed;
	float ADS;
	float Precision;
	float HipFire;
	string FireMode;
	ref array<string> AppliedItems;

	void GGResolvedWeaponStats()
	{
		BaseRecoil = 1.0;
		BaseSway = 1.0;
		BaseADS = 1.0;
		BasePrecision = 1.0;
		Recoil = 1.0;
		Sway = 1.0;
		EffectiveSway = 1.0;
		EffectiveSwaySpeed = 1.0;
		ADS = 1.0;
		Precision = 1.0;
		HipFire = 1.0;
		FireMode = "";
		AppliedItems = new array<string>;
	}
}

class GGWeaponStatsManager
{
	protected ref GGResolvedWeaponStats m_Stats;

	void GGWeaponStatsManager()
	{
		m_Stats = new GGResolvedWeaponStats();
	}

	void Calculate(Weapon_Base weapon)
	{
		m_Stats = new GGResolvedWeaponStats();
		if (!weapon) return;

		string modeName = weapon.GetCurrentModeName(weapon.GetCurrentMuzzle());
		if (!CalculateBase(weapon.GetType(), modeName)) return;

		ApplyAttachmentTree(weapon, weapon.GetType());

		Magazine magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
		if (magazine) ApplyMagazineType(magazine.GetType());
		Finish(weapon.GetType());
	}

	void CalculateByType(string weaponType, array<string> attachmentTypes = null, string modeName = "")
	{
		m_Stats = new GGResolvedWeaponStats();
		if (!CalculateBase(weaponType, modeName)) return;
		if (attachmentTypes)
		{
			foreach (string attachmentType : attachmentTypes)
				ApplyItemType(attachmentType);
		}
		Finish(weaponType);
	}

	protected bool CalculateBase(string weaponType, string modeName)
	{
		GGConfigManager config = GetGGConfigManager();
		GGWeaponConfig weaponConfig = config.GetWeapon(weaponType);
		GGSettings settings = config.GetSettings();
		if (!weaponConfig || !settings) return false;

		m_Stats.BaseRecoil = weaponConfig.DetectedRecoil;
		m_Stats.BaseSway = weaponConfig.DetectedSway;
		m_Stats.BaseADS = weaponConfig.DetectedADSSpeed;
		m_Stats.BasePrecision = weaponConfig.DetectedPrecision;
		m_Stats.Recoil = m_Stats.BaseRecoil * settings.GlobalRecoilMultiplier * weaponConfig.RecoilMultiplier;
		m_Stats.Sway = m_Stats.BaseSway * settings.GlobalSwayMultiplier * weaponConfig.SwayMultiplier;
		m_Stats.ADS = m_Stats.BaseADS * settings.GlobalAimSpeedMultiplier * weaponConfig.ADSMultiplier;
		m_Stats.Precision = m_Stats.BasePrecision * settings.GlobalPrecisionMultiplier * weaponConfig.PrecisionMultiplier;
		m_Stats.HipFire = settings.GlobalHipFireMultiplier * weaponConfig.HipFireMultiplier;
		m_Stats.FireMode = modeName;

		GGFireModeConfig mode = config.GetFireMode(weaponConfig, modeName);
		if (mode)
		{
			m_Stats.Recoil *= mode.RecoilMultiplier;
			m_Stats.Sway *= mode.SwayMultiplier;
			m_Stats.ADS *= mode.ADSMultiplier;
			m_Stats.Precision *= mode.PrecisionMultiplier;
			m_Stats.HipFire *= mode.HipFireMultiplier;
		}
		return true;
	}

	protected void ApplyItemType(string itemType)
	{
		if (itemType == "") return;
		GGConfigManager config = GetGGConfigManager();
		GGTierDefinition effect = config.GetAttachmentEffect(itemType);
		if (!effect) effect = config.GetMagazineEffect(itemType);
		if (!effect) return;
		ApplyEffect(effect);
		m_Stats.AppliedItems.Insert(itemType + "=" + effect.TierKey);
	}

	protected void ApplyAttachmentType(string itemType)
	{
		if (itemType == "") return;
		GGTierDefinition effect = GetGGConfigManager().GetAttachmentEffect(itemType);
		if (!effect) return;
		ApplyEffect(effect);
		m_Stats.AppliedItems.Insert(itemType + "=" + effect.TierKey);
	}

	protected void ApplyMagazineType(string itemType)
	{
		if (itemType == "") return;
		GGTierDefinition effect = GetGGConfigManager().GetMagazineEffect(itemType);
		if (!effect) return;
		ApplyEffect(effect);
		m_Stats.AppliedItems.Insert(itemType + "=" + effect.TierKey);
	}

	protected void ApplyAttachmentTree(EntityAI parent, string weaponType, int depth = 0)
	{
		if (!parent || !parent.GetInventory() || depth >= 32) return;
		for (int i = 0; i < parent.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachment = parent.GetInventory().GetAttachmentFromIndex(i);
			if (!attachment) continue;
			if (!GetGGConfigManager().IsAttachmentAllowed(weaponType, attachment.GetType())) continue;
			ApplyAttachmentType(attachment.GetType());
			ApplyAttachmentTree(attachment, weaponType, depth + 1);
		}
	}

	protected void ApplyEffect(GGTierDefinition effect)
	{
		m_Stats.Recoil *= effect.Recoil;
		m_Stats.Sway *= effect.Sway;
		m_Stats.ADS *= effect.ADS;
		m_Stats.Precision *= effect.Precision;
		m_Stats.HipFire *= effect.HipFire;
	}

	protected void Finish(string weaponType)
	{
		m_Stats.Recoil = GGUtil.Clamp(m_Stats.Recoil, 0.001, 5.0);
		m_Stats.Sway = GGUtil.Clamp(m_Stats.Sway, 0.001, 5.0);
		m_Stats.ADS = GGUtil.Clamp(m_Stats.ADS, 0.05, 5.0);
		m_Stats.Precision = GGUtil.Clamp(m_Stats.Precision, 0.05, 5.0);
		m_Stats.HipFire = GGUtil.Clamp(m_Stats.HipFire, 0.05, 5.0);
		m_Stats.EffectiveSway = GGUtil.Clamp(m_Stats.Sway / m_Stats.Precision, 0.15, 1.0);
		m_Stats.EffectiveSwaySpeed = GGUtil.Clamp(m_Stats.Sway / m_Stats.Precision, 0.35, 1.0);

		GGSettings settings = GetGGConfigManager().GetSettings();
		if (settings && settings.DebugMode)
		{
			string debugLine = "Resolved " + weaponType;
			debugLine += " mode=" + m_Stats.FireMode;
			debugLine += " recoil=" + m_Stats.Recoil.ToString();
			debugLine += " sway=" + m_Stats.EffectiveSway.ToString();
			debugLine += " swaySpeed=" + m_Stats.EffectiveSwaySpeed.ToString();
			debugLine += " ADS=" + m_Stats.ADS.ToString();
			debugLine += " precision=" + m_Stats.Precision.ToString();
			debugLine += " hipfire=" + m_Stats.HipFire.ToString();
			GGUtil.Log(debugLine);
		}
	}

	GGResolvedWeaponStats GetStats()
	{
		return m_Stats;
	}
}

class GGWeaponAttachmentQueries
{
	static bool HasLaser(EntityAI parent)
	{
		Weapon_Base weapon = Weapon_Base.Cast(parent);
		if (!weapon) return false;
		return HasAllowedLaser(parent, weapon.GetType(), 0);
	}

	protected static bool HasAllowedLaser(EntityAI parent, string weaponType, int depth)
	{
		if (!parent || !parent.GetInventory() || depth >= 32) return false;
		for (int i = 0; i < parent.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachment = parent.GetInventory().GetAttachmentFromIndex(i);
			if (!attachment) continue;
			if (!GetGGConfigManager().IsAttachmentAllowed(weaponType, attachment.GetType())) continue;
			GGAttachmentConfig config = GetGGConfigManager().GetAttachment(attachment.GetType());
			if (config && config.Category == "Laser") return true;
			string itemType = GGUtil.Key(attachment.GetType());
			if (itemType.Contains("laser")) return true;
			if (itemType.Contains("anpeq")) return true;
			if (itemType.Contains("peq")) return true;
			if (itemType.Contains("ngal")) return true;
			if (itemType.Contains("dbal")) return true;
			if (itemType.Contains("mawl")) return true;
			if (HasAllowedLaser(attachment, weaponType, depth + 1)) return true;
		}
		return false;
	}
}
