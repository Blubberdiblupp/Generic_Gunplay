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
	protected ref map<string, bool> m_AppliedEffectGroups;

	void GGWeaponStatsManager()
	{
		ResetCalculation();
	}

	void Calculate(Weapon_Base weapon)
	{
		int debugStarted = GGDebug.BeginTiming(9);
		ResetCalculation();
		if (!weapon)
		{
			GGDebug.EndTiming(9, "PERFORMANCE", "Weapon stat calculation", debugStarted, "aborted: no weapon");
			return;
		}

		string modeName = weapon.GetCurrentModeName(weapon.GetCurrentMuzzle());
		if (!CalculateBase(weapon.GetType(), modeName))
		{
			GGDebug.Once(4, "STATS", "missing_weapon_" + GGUtil.Key(weapon.GetType()), "No runtime weapon config found for " + weapon.GetType());
			GGDebug.EndTiming(9, "PERFORMANCE", "Weapon stat calculation", debugStarted, "aborted: missing config for " + weapon.GetType());
			return;
		}

		ApplyAttachmentTree(weapon, weapon.GetType());

		Magazine magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
		if (magazine) ApplyMagazineType(magazine.GetType());
		Finish(weapon.GetType());
		GGDebug.EndTiming(9, "PERFORMANCE", "Weapon stat calculation", debugStarted, "weapon=" + weapon.GetType() + " effects=" + m_Stats.AppliedItems.Count().ToString());
	}

	void CalculateByType(string weaponType, array<string> attachmentTypes = null, string modeName = "")
	{
		int debugStarted = GGDebug.BeginTiming(9);
		ResetCalculation();
		if (!CalculateBase(weaponType, modeName))
		{
			GGDebug.Once(4, "STATS", "missing_weapon_" + GGUtil.Key(weaponType), "No runtime weapon config found for " + weaponType);
			GGDebug.EndTiming(9, "PERFORMANCE", "Typed weapon stat calculation", debugStarted, "aborted: missing config for " + weaponType);
			return;
		}
		if (attachmentTypes)
		{
			for (int i = attachmentTypes.Count() - 1; i >= 0; i--)
				ApplyItemType(attachmentTypes[i]);
		}
		Finish(weaponType);
		GGDebug.EndTiming(9, "PERFORMANCE", "Typed weapon stat calculation", debugStarted, "weapon=" + weaponType + " effects=" + m_Stats.AppliedItems.Count().ToString());
	}

	protected void ResetCalculation()
	{
		m_Stats = new GGResolvedWeaponStats();
		m_AppliedEffectGroups = new map<string, bool>;
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
		if (GGDebug.Enabled(5))
		{
			string baseLine = "weapon=" + weaponType + " mode=" + modeName;
			baseLine += " detected(recoil=" + m_Stats.BaseRecoil.ToString();
			baseLine += " sway=" + m_Stats.BaseSway.ToString();
			baseLine += " ADS=" + m_Stats.BaseADS.ToString();
			baseLine += " precision=" + m_Stats.BasePrecision.ToString() + ")";
			baseLine += " composed(recoil=" + m_Stats.Recoil.ToString();
			baseLine += " sway=" + m_Stats.Sway.ToString();
			baseLine += " ADS=" + m_Stats.ADS.ToString();
			baseLine += " precision=" + m_Stats.Precision.ToString();
			baseLine += " hipfire=" + m_Stats.HipFire.ToString() + ")";
			GGDebug.Log(5, "STATS", baseLine);
		}
		return true;
	}

	protected void ApplyItemType(string itemType)
	{
		if (itemType == "") return;
		GGConfigManager config = GetGGConfigManager();
		GGTierDefinition effect = config.GetAttachmentEffect(itemType);
		if (effect)
		{
			TryApplyEffect(effect, itemType, false);
			return;
		}

		effect = config.GetMagazineEffect(itemType);
		if (effect)
			TryApplyEffect(effect, itemType, true);
		else
			GGDebug.Once(4, "STATS", "missing_item_" + GGUtil.Key(itemType), "No attachment or magazine effect found for " + itemType);
	}

	protected void ApplyAttachmentType(string itemType)
	{
		if (itemType == "") return;
		GGTierDefinition effect = GetGGConfigManager().GetAttachmentEffect(itemType);
		if (!effect) return;
		TryApplyEffect(effect, itemType, false);
	}

	protected void ApplyMagazineType(string itemType)
	{
		if (itemType == "") return;
		GGTierDefinition effect = GetGGConfigManager().GetMagazineEffect(itemType);
		if (!effect) return;
		TryApplyEffect(effect, itemType, true);
	}

	protected void ApplyAttachmentTree(EntityAI parent, string weaponType, int depth = 0)
	{
		if (!parent || !parent.GetInventory() || depth >= 32) return;
		GGDebug.Count(10, "STATS", "attachment_tree_nodes", 10000);
		for (int i = 0; i < parent.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachment = parent.GetInventory().GetAttachmentFromIndex(i);
			if (!attachment) continue;
			ApplyAttachmentTree(attachment, weaponType, depth + 1);
			if (!GetGGConfigManager().IsAttachmentAllowed(weaponType, attachment.GetType()))
			{
				string blockedKey = "blocked_effect_" + GGUtil.Key(weaponType);
				blockedKey += "_" + GGUtil.Key(attachment.GetType());
				string blockedMessage = "Skipped blocked attachment effect. weapon=" + weaponType;
				blockedMessage += " attachment=" + attachment.GetType();
				GGDebug.Once(8, "POLICY", blockedKey, blockedMessage);
				continue;
			}
			ApplyAttachmentType(attachment.GetType());
		}
	}

	protected bool TryApplyEffect(GGTierDefinition effect, string itemType, bool magazine)
	{
		if (!effect) return false;

		string group = GetEffectGroup(effect, magazine);
		if (group != "")
		{
			bool ignored;
			if (m_AppliedEffectGroups.Find(group, ignored))
			{
				if (GGDebug.Enabled(5))
				{
					string duplicateMessage = "Skipped duplicate effect group=" + group;
					duplicateMessage += " item=" + itemType;
					duplicateMessage += " tier=" + effect.TierKey;
					GGDebug.Log(5, "STATS", duplicateMessage);
				}
				return false;
			}
			m_AppliedEffectGroups.Set(group, true);
		}

		ApplyEffect(effect);
		m_Stats.AppliedItems.Insert(itemType + "=" + effect.TierKey);
		if (GGDebug.Enabled(5))
		{
			string effectLine = "Applied item=" + itemType + " category=" + effect.Category;
			effectLine += " tier=" + effect.TierKey;
			effectLine += " multipliers(recoil=" + effect.Recoil.ToString();
			effectLine += " sway=" + effect.Sway.ToString();
			effectLine += " ADS=" + effect.ADS.ToString();
			effectLine += " precision=" + effect.Precision.ToString();
			effectLine += " hipfire=" + effect.HipFire.ToString() + ")";
			GGDebug.Log(5, "STATS", effectLine);
		}
		return true;
	}

	protected string GetEffectGroup(GGTierDefinition effect, bool magazine)
	{
		if (magazine) return "magazine";

		string category = GGUtil.Key(effect.Category);
		if (category == "muzzle" || category == "suppressor") return "muzzle";
		if (category == "magazine") return "magazine";
		if (category == "neutral") return "";
		return category;
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

		if (GGDebug.Enabled(1))
		{
			string debugLine = "Resolved " + weaponType;
			debugLine += " mode=" + m_Stats.FireMode;
			debugLine += " recoil=" + m_Stats.Recoil.ToString();
			debugLine += " sway=" + m_Stats.EffectiveSway.ToString();
			debugLine += " swaySpeed=" + m_Stats.EffectiveSwaySpeed.ToString();
			debugLine += " ADS=" + m_Stats.ADS.ToString();
			debugLine += " precision=" + m_Stats.Precision.ToString();
			debugLine += " hipfire=" + m_Stats.HipFire.ToString();
			debugLine += " effects=" + m_Stats.AppliedItems.Count().ToString();
			GGDebug.Log(1, "STATS", debugLine);
			string clientState = m_Stats.FireMode + "|" + m_Stats.Recoil.ToString();
			clientState += "|" + m_Stats.EffectiveSway.ToString();
			clientState += "|" + m_Stats.EffectiveSwaySpeed.ToString();
			clientState += "|" + m_Stats.ADS.ToString();
			clientState += "|" + m_Stats.Precision.ToString();
			clientState += "|" + m_Stats.HipFire.ToString();
			GGDebug.ClientState(1, "STATS", GGUtil.Key(weaponType), clientState, debugLine);
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
		return weapon.GGHasLaserAttachmentCached();
	}

	static bool ScanHasLaser(Weapon_Base weapon)
	{
		if (!weapon) return false;
		return HasAllowedLaser(weapon, weapon.GetType(), 0);
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
