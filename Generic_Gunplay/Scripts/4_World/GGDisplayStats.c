class GGDisplayLine : Managed
{
	string Label;
	string Value;
	int Color;

	void GGDisplayLine(string label = "", string value = "", int color = 0xFFFFFFFF)
	{
		Label = label;
		Value = value;
		Color = color;
	}
}

class GGDisplayData : Managed
{
	string Title;
	ref array<ref GGDisplayLine> Lines;

	void GGDisplayData(string title = "GUNPLAY STATS")
	{
		Title = title;
		Lines = new array<ref GGDisplayLine>;
	}

	void Add(string label, string value, int color = 0xFFFFFFFF)
	{
		if (Lines.Count() < 8) Lines.Insert(new GGDisplayLine(label, value, color));
	}
}

class GGDisplayStats
{
	protected static const int COLOR_NEUTRAL = 0xFFFFFFFF;
	protected static const int COLOR_GOOD = 0xFF22DD66;
	protected static const int COLOR_BAD = 0xFFFF5555;

	static GGDisplayData GetDisplay(EntityAI item, string itemType = "", array<string> attachmentTypes = null)
	{
		GGConfigManager config = GetGGConfigManager();
		GGSettings settings = config.GetSettings();
		if (!settings) return null;

		string resolvedType = itemType;
		if (item) resolvedType = item.GetType();
		if (resolvedType == "") return null;

		GGWeaponConfig weaponConfig = config.GetWeapon(resolvedType);
		if (weaponConfig && settings.EnableWeaponStats)
			return GetWeaponDisplay(Weapon_Base.Cast(item), weaponConfig, attachmentTypes, settings);

		GGMagazineConfig magazineConfig = config.GetMagazine(resolvedType);
		if (magazineConfig)
		{
			if (magazineConfig.IsLooseAmmo && settings.EnableAmmoStats)
				return GetAmmoDisplay(config.GetAmmo(magazineConfig.AmmoClass), settings);
			if (settings.EnableMagazineStats)
				return GetMagazineDisplay(magazineConfig, settings);
		}

		GGAttachmentConfig attachmentConfig = config.GetAttachment(resolvedType);
		if (attachmentConfig && settings.EnableAttachmentStats)
			return GetAttachmentDisplay(attachmentConfig, settings);

		GGArmorConfig armorConfig = config.GetArmor(resolvedType);
		if (armorConfig && settings.EnableArmorStats)
			return GetArmorDisplay(armorConfig, settings);

		GGAmmoConfig ammoConfig = config.GetAmmo(resolvedType);
		if (ammoConfig && settings.EnableAmmoStats)
			return GetAmmoDisplay(ammoConfig, settings);

		return null;
	}

	protected static GGDisplayData GetWeaponDisplay(Weapon_Base weapon, GGWeaponConfig weaponConfig, array<string> attachmentTypes, GGSettings settings)
	{
		GGResolvedWeaponStats stats;
		if (weapon)
		{
			stats = weapon.GetGGResolvedStats();
		}
		else
		{
			GGWeaponStatsManager manager = new GGWeaponStatsManager();
			manager.CalculateByType(weaponConfig.ClassName, attachmentTypes);
			stats = manager.GetStats();
		}
		if (!stats) return null;

		string title = "WEAPON";
		if (stats.FireMode != "") title = title + " | " + stats.FireMode;
		GGDisplayData data = new GGDisplayData(title);
		GGStatVisibility visible = settings.VisibleStats;
		if (visible.Recoil) AddMultiplier(data, "Recoil:", stats.Recoil, false);
		if (visible.Sway) AddMultiplier(data, "Sway:", stats.EffectiveSway, false);
		if (visible.ADS) data.Add("ADS time:", FormatSeconds(stats.ADS), GetDeltaColor(stats.ADS, true));
		if (visible.Precision) AddMultiplier(data, "Aim stability:", stats.Precision, true);
		if (visible.HipFire) AddMultiplier(data, "Hipfire:", stats.HipFire, false);

		if (visible.RPM)
		{
			string modeSummary = BuildModeSummary(weaponConfig, settings, stats.FireMode);
			if (modeSummary != "") data.Add("Fire mode RPM:", modeSummary);
		}

		if (visible.Dispersion)
		{
			string dispersion = GetDispersionText(weaponConfig, settings, stats.FireMode);
			if (dispersion != "") data.Add("Dispersion:", dispersion);
		}

		if (visible.MuzzleVelocity)
		{
			int velocity = GetWeaponMuzzleVelocity(weapon, weaponConfig, settings);
			if (velocity > 0) data.Add("Muzzle velocity:", velocity.ToString() + " m/s");
		}
		return data;
	}

	protected static GGDisplayData GetAttachmentDisplay(GGAttachmentConfig attachment, GGSettings settings)
	{
		GGTierDefinition effect = GetGGConfigManager().GetAttachmentEffect(attachment.ClassName);
		if (!effect) return null;
		GGDisplayData data = new GGDisplayData(BuildTierTitle(attachment.Category, effect.Tier));
		AddEffectLines(data, effect, settings.VisibleStats, false);
		return data;
	}

	protected static GGDisplayData GetMagazineDisplay(GGMagazineConfig magazine, GGSettings settings)
	{
		GGTierDefinition effect = GetGGConfigManager().GetMagazineEffect(magazine.ClassName);
		if (!effect) return null;
		GGDisplayData data = new GGDisplayData(BuildTierTitle("MAGAZINE", effect.Tier));
		if (settings.VisibleStats.MagazineCapacity)
			data.Add("Capacity:", magazine.DetectedCapacity.ToString() + " rounds");
		AddEffectLines(data, effect, settings.VisibleStats, false);
		return data;
	}

	protected static GGDisplayData GetAmmoDisplay(GGAmmoConfig ammo, GGSettings settings)
	{
		if (!ammo) return null;
		float initSpeed = ammo.DetectedInitSpeed;
		float typicalSpeed = ammo.DetectedTypicalSpeed;
		float airFriction = ammo.DetectedAirFriction;
		float healthDamage = ammo.DetectedHealthDamage;
		float bloodDamage = ammo.DetectedBloodDamage;
		float shockDamage = ammo.DetectedShockDamage;
		float hit = ammo.DetectedHit;

		GGDisplayData data = new GGDisplayData("AMMUNITION");
		if (settings.VisibleStats.AmmoBallistics)
		{
			if (initSpeed > 0.0) data.Add("Initial velocity:", Math.Round(initSpeed).ToString() + " m/s");
			if (typicalSpeed > 0.0) data.Add("Typical speed:", Math.Round(typicalSpeed).ToString() + " m/s");
			data.Add("Air friction:", FormatNumber(airFriction));
		}
		if (settings.VisibleStats.AmmoDamage)
		{
			data.Add("Health damage:", FormatNumber(healthDamage));
			data.Add("Blood damage:", FormatNumber(bloodDamage));
			data.Add("Shock damage:", FormatNumber(shockDamage));
			if (hit > 0.0) data.Add("Legacy hit:", FormatNumber(hit));
		}
		return data;
	}

	protected static GGDisplayData GetArmorDisplay(GGArmorConfig armor, GGSettings settings)
	{
		float projectile = armor.DetectedProjectileReduction;
		float melee = armor.DetectedMeleeReduction;
		float infected = armor.DetectedInfectedReduction;
		float frag = armor.DetectedFragReduction;

		GGDisplayData data = new GGDisplayData("ARMOR | " + GetArmorTier(projectile, settings));
		if (settings.VisibleStats.Armor)
		{
			AddReduction(data, "Projectile:", projectile);
			AddReduction(data, "Melee:", melee);
			AddReduction(data, "Infected:", infected);
			AddReduction(data, "Frag grenade:", frag);
		}
		return data;
	}

	protected static void AddEffectLines(GGDisplayData data, GGTierDefinition effect, GGStatVisibility visible, bool weapon)
	{
		if (visible.Recoil) AddMultiplier(data, "Recoil:", effect.Recoil, false);
		if (visible.Sway) AddMultiplier(data, "Sway:", effect.Sway, false);
		if (visible.ADS) AddMultiplier(data, "ADS speed:", effect.ADS, true);
		if (visible.Precision) AddMultiplier(data, "Aim stability:", effect.Precision, true);
		if (visible.HipFire) AddMultiplier(data, "Hipfire:", effect.HipFire, false);
	}

	protected static void AddMultiplier(GGDisplayData data, string label, float multiplier, bool higherIsBetter)
	{
		data.Add(label, FormatDelta(multiplier), GetDeltaColor(multiplier, higherIsBetter));
	}

	protected static void AddReduction(GGDisplayData data, string label, float reduction)
	{
		int color = COLOR_BAD;
		if (reduction > 0.0) color = COLOR_GOOD;
		data.Add(label, Math.Round(reduction).ToString() + "%", color);
	}

	protected static string BuildTierTitle(string category, string tier)
	{
		string title = category;
		title.ToUpper();
		if (tier != "" && tier != "Neutral") title = title + " | " + tier;
		return title;
	}

	protected static string GetArmorTier(float projectile, GGSettings settings)
	{
		if (projectile >= settings.ArmorTier3Minimum) return "T3";
		if (projectile >= settings.ArmorTier2Minimum) return "T2";
		if (projectile >= settings.ArmorTier1Minimum) return "T1";
		return "T0";
	}

	protected static string BuildModeSummary(GGWeaponConfig weapon, GGSettings settings, string selectedMode)
	{
		if (!weapon || !weapon.FireModes) return "";
		string summary;
		int added;
		foreach (GGFireModeConfig mode : weapon.FireModes)
		{
			if (!mode) continue;
			if (selectedMode != "" && GGUtil.Key(mode.ModeName) != GGUtil.Key(selectedMode)) continue;
			float reloadTime = GetEffectiveReloadTime(mode, settings);
			if (reloadTime <= 0.0) continue;
			if (summary != "") summary = summary + " | ";
			summary = summary + ShortModeName(mode.ModeName) + " " + Math.Round(60.0 / reloadTime).ToString();
			added++;
			if (selectedMode != "" || added >= 3) break;
		}
		return summary;
	}

	protected static string GetDispersionText(GGWeaponConfig weapon, GGSettings settings, string selectedMode)
	{
		if (!weapon || !weapon.FireModes) return "";
		float best = 1000000.0;
		foreach (GGFireModeConfig mode : weapon.FireModes)
		{
			if (!mode) continue;
			if (selectedMode != "" && GGUtil.Key(mode.ModeName) != GGUtil.Key(selectedMode)) continue;
			float dispersion = mode.DetectedDispersion;
			if (dispersion >= 0.0 && dispersion < best) best = dispersion;
		}
		if (best == 1000000.0) return "";
		return FormatNumber(best);
	}

	protected static float GetEffectiveReloadTime(GGFireModeConfig mode, GGSettings settings)
	{
		return mode.DetectedReloadTime;
	}

	protected static string ShortModeName(string modeName)
	{
		string lower = GGUtil.Key(modeName);
		if (lower.Contains("full") || lower.Contains("auto")) return "Auto";
		if (lower.Contains("burst")) return "Burst";
		if (lower.Contains("semi")) return "Semi";
		if (lower.Contains("single")) return "Single";
		if (lower.Contains("double")) return "Double";
		return modeName;
	}

	protected static int GetWeaponMuzzleVelocity(Weapon_Base weapon, GGWeaponConfig weaponConfig, GGSettings settings)
	{
		string ammoType;
		if (weapon)
		{
			ammoType = weapon.GetChamberAmmoTypeName(weapon.GetCurrentMuzzle());
			if (ammoType == "")
			{
				Magazine currentMagazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
				if (currentMagazine)
				{
					GGMagazineConfig currentMagazineConfig = GetGGConfigManager().GetMagazine(currentMagazine.GetType());
					if (currentMagazineConfig) ammoType = currentMagazineConfig.AmmoClass;
				}
			}
		}

		float ammoSpeed;
		if (ammoType != "") ammoSpeed = GetAmmoInitSpeed(ammoType, settings);
		if (ammoSpeed <= 0.0) ammoSpeed = GetBestCompatibleAmmoSpeed(weaponConfig.ClassName, settings);
		if (ammoSpeed <= 0.0) return 0;

		float weaponMultiplier = weaponConfig.DetectedInitSpeedMultiplier;
		if (weaponMultiplier <= 0.0) weaponMultiplier = 1.0;
		return Math.Round(ammoSpeed * weaponMultiplier);
	}

	protected static float GetBestCompatibleAmmoSpeed(string weaponType, GGSettings settings)
	{
		TStringArray chamberable = new TStringArray;
		g_Game.ConfigGetTextArray("CfgWeapons " + weaponType + " chamberableFrom", chamberable);
		float best;
		foreach (string magazineType : chamberable)
		{
			string ammoType;
			GGMagazineConfig magazine = GetGGConfigManager().GetMagazine(magazineType);
			if (magazine) ammoType = magazine.AmmoClass;
			if (ammoType == "") AmmoTypesAPI.GetAmmoType(magazineType, ammoType);
			if (ammoType == "" && GetGGConfigManager().GetAmmo(magazineType)) ammoType = magazineType;
			float speed = GetAmmoInitSpeed(ammoType, settings);
			if (speed > best) best = speed;
		}
		return best;
	}

	protected static float GetAmmoInitSpeed(string ammoType, GGSettings settings)
	{
		if (ammoType == "") return 0.0;
		GGAmmoConfig ammo = GetGGConfigManager().GetAmmo(ammoType);
		if (!ammo) return 0.0;
		return ammo.DetectedInitSpeed;
	}

	protected static string FormatSeconds(float aimSpeed)
	{
		if (aimSpeed <= 0.05) aimSpeed = 1.0;
		float seconds = GGUtil.Clamp(0.7 / aimSpeed, 0.15, 1.4);
		return FormatNumber(seconds) + " s";
	}

	protected static string FormatDelta(float multiplier)
	{
		if (Math.AbsFloat(multiplier - 1.0) < 0.001) return "0%";
		int rounded = Math.Round((multiplier - 1.0) * 100.0);
		if (rounded > 0) return "+" + rounded.ToString() + "%";
		return rounded.ToString() + "%";
	}

	protected static string FormatNumber(float value)
	{
		float rounded = Math.Round(value * 1000.0) / 1000.0;
		return rounded.ToString();
	}

	protected static int GetDeltaColor(float multiplier, bool higherIsBetter)
	{
		if (Math.AbsFloat(multiplier - 1.0) < 0.001) return COLOR_NEUTRAL;
		bool improved = multiplier > 1.0;
		if (!higherIsBetter) improved = multiplier < 1.0;
		if (improved) return COLOR_GOOD;
		return COLOR_BAD;
	}
}
