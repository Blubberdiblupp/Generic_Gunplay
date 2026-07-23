class GGDisplayStat
{
	static const int NONE = 0;
	static const int RECOIL = 1;
	static const int SWAY = 2;
	static const int ADS = 3;
	static const int PRECISION = 4;
	static const int DISPERSION = 5;
	static const int HIPFIRE = 6;
	static const int RPM = 7;
	static const int MUZZLE_VELOCITY = 8;
	static const int MAGAZINE_CAPACITY = 9;
	static const int AMMO_BALLISTICS = 10;
	static const int AMMO_DAMAGE = 11;
	static const int ARMOR = 12;
}

class GGDisplayLine : Managed
{
	string Label;
	string Value;
	int Color;
	int Stat;
	string SecondaryLabel;
	string SecondaryValue;
	int SecondaryColor;
	int SecondaryStat;

	void GGDisplayLine(string label = "", string value = "", int color = 0xFFFFFFFF, int stat = 0)
	{
		Label = label;
		Value = value;
		Color = color;
		Stat = stat;
		SecondaryColor = 0xFFFFFFFF;
		SecondaryStat = GGDisplayStat.NONE;
	}

	void SetSecondary(string label, string value, int color, int stat)
	{
		SecondaryLabel = label;
		SecondaryValue = value;
		SecondaryColor = color;
		SecondaryStat = stat;
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

	void Add(string label, string value, int color = 0xFFFFFFFF, int stat = 0)
	{
		if (Lines.Count() < 8) Lines.Insert(new GGDisplayLine(label, value, color, stat));
	}

	void AddCombined(string label, string value, int color, int stat, string secondaryLabel, string secondaryValue, int secondaryColor, int secondaryStat)
	{
		if (Lines.Count() >= 8) return;
		GGDisplayLine line = new GGDisplayLine(label, value, color, stat);
		line.SetSecondary(secondaryLabel, secondaryValue, secondaryColor, secondaryStat);
		Lines.Insert(line);
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
		if (!settings || !settings.VisibleStats) return null;
		GGStatVisibility visible = settings.VisibleStats;

		string resolvedType = itemType;
		if (item) resolvedType = item.GetType();
		if (resolvedType == "") return null;

		GGWeaponConfig weaponConfig = config.GetWeapon(resolvedType);
		if (weaponConfig && settings.EnableWeaponStats)
			return GetWeaponDisplay(Weapon_Base.Cast(item), weaponConfig, attachmentTypes, settings, visible);

		GGMagazineConfig magazineConfig = config.GetMagazine(resolvedType);
		if (magazineConfig)
		{
			if (magazineConfig.IsLooseAmmo && settings.EnableAmmoStats)
				return GetAmmoDisplay(config.GetAmmo(magazineConfig.AmmoClass), settings, visible);
			if (settings.EnableMagazineStats)
				return GetMagazineDisplay(magazineConfig, visible);
		}

		GGAttachmentConfig attachmentConfig = config.GetAttachment(resolvedType);
		if (attachmentConfig && settings.EnableAttachmentStats)
			return GetAttachmentDisplay(attachmentConfig, visible);

		GGArmorConfig armorConfig = config.GetArmor(resolvedType);
		if (armorConfig && settings.EnableArmorStats)
			return GetArmorDisplay(armorConfig, settings, visible);

		GGAmmoConfig ammoConfig = config.GetAmmo(resolvedType);
		if (ammoConfig && settings.EnableAmmoStats)
			return GetAmmoDisplay(ammoConfig, settings, visible);

		return null;
	}

	protected static GGDisplayData GetWeaponDisplay(Weapon_Base weapon, GGWeaponConfig weaponConfig, array<string> attachmentTypes, GGSettings settings, GGStatVisibility visible)
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

		GGDisplayData data = new GGDisplayData("Weapon Total");
		if (visible.Recoil) AddMultiplier(data, "Recoil:", stats.Recoil, false, GGDisplayStat.RECOIL);
		if (visible.Sway) AddMultiplier(data, "Sway:", stats.Sway, false, GGDisplayStat.SWAY);
		if (visible.ADS) data.Add("ADS Time:", FormatSeconds(stats.ADS), GetDeltaColor(stats.ADS, true), GGDisplayStat.ADS);
		if (visible.Precision) AddMultiplier(data, "Precision:", stats.Precision, true, GGDisplayStat.PRECISION);
		if (visible.HipFire) AddMultiplier(data, "Hipfire:", stats.HipFire, false, GGDisplayStat.HIPFIRE);
		if (visible.Dispersion)
		{
			string dispersion = GetDispersionText(weaponConfig, settings, stats.FireMode);
			if (dispersion != "") data.Add("Dispersion:", dispersion, COLOR_NEUTRAL, GGDisplayStat.DISPERSION);
		}

		int rpm;
		string rpmText = "-";
		if (visible.RPM)
		{
			rpm = GetHighestWeaponRPM(weaponConfig);
			if (rpm > 0) rpmText = rpm.ToString();
		}

		int velocity;
		string velocityText;
		if (visible.MuzzleVelocity)
		{
			velocity = GetWeaponMuzzleVelocity(weapon, weaponConfig, settings);
			velocityText = velocity.ToString() + " m/s";
		}

		if (visible.RPM && visible.MuzzleVelocity)
			data.AddCombined("RPM:", rpmText, COLOR_NEUTRAL, GGDisplayStat.RPM, "Velocity:", velocityText, COLOR_NEUTRAL, GGDisplayStat.MUZZLE_VELOCITY);
		else if (visible.RPM)
			data.Add("RPM:", rpmText, COLOR_NEUTRAL, GGDisplayStat.RPM);
		else if (visible.MuzzleVelocity)
			data.Add("Velocity:", velocityText, COLOR_NEUTRAL, GGDisplayStat.MUZZLE_VELOCITY);
		return data;
	}

	protected static GGDisplayData GetAttachmentDisplay(GGAttachmentConfig attachment, GGStatVisibility visible)
	{
		GGTierDefinition effect = GetGGConfigManager().GetAttachmentEffect(attachment.ClassName);
		if (!effect) return null;
		GGDisplayData data = new GGDisplayData(BuildTierTitle(effect.Category, effect.Tier));
		AddEffectLines(data, effect, visible);
		return data;
	}

	protected static GGDisplayData GetMagazineDisplay(GGMagazineConfig magazine, GGStatVisibility visible)
	{
		GGTierDefinition effect = GetGGConfigManager().GetMagazineEffect(magazine.ClassName);
		if (!effect) return null;
		GGDisplayData data = new GGDisplayData(BuildTierTitle(effect.Category, effect.Tier));
		if (visible.MagazineCapacity)
			data.Add("Capacity:", magazine.DetectedCapacity.ToString() + " rnd", COLOR_NEUTRAL, GGDisplayStat.MAGAZINE_CAPACITY);
		if (visible.Recoil) AddMultiplier(data, "Recoil:", effect.Recoil, false, GGDisplayStat.RECOIL);
		if (visible.ADS) AddMultiplier(data, "ADS:", effect.ADS, true, GGDisplayStat.ADS);
		if (visible.Sway) AddMultiplier(data, "Sway:", effect.Sway, false, GGDisplayStat.SWAY);
		if (visible.HipFire) AddMultiplier(data, "Hipfire:", effect.HipFire, false, GGDisplayStat.HIPFIRE);
		return data;
	}

	protected static GGDisplayData GetAmmoDisplay(GGAmmoConfig ammo, GGSettings settings, GGStatVisibility visible)
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
		if (visible.AmmoBallistics)
		{
			if (initSpeed > 0.0) data.Add("Initial velocity:", Math.Round(initSpeed).ToString() + " m/s", COLOR_NEUTRAL, GGDisplayStat.AMMO_BALLISTICS);
			if (typicalSpeed > 0.0) data.Add("Typical speed:", Math.Round(typicalSpeed).ToString() + " m/s", COLOR_NEUTRAL, GGDisplayStat.AMMO_BALLISTICS);
			data.Add("Air friction:", FormatNumber(airFriction), COLOR_NEUTRAL, GGDisplayStat.AMMO_BALLISTICS);
		}
		if (visible.AmmoDamage)
		{
			data.Add("Health damage:", FormatNumber(healthDamage), COLOR_NEUTRAL, GGDisplayStat.AMMO_DAMAGE);
			data.Add("Blood damage:", FormatNumber(bloodDamage), COLOR_NEUTRAL, GGDisplayStat.AMMO_DAMAGE);
			data.Add("Shock damage:", FormatNumber(shockDamage), COLOR_NEUTRAL, GGDisplayStat.AMMO_DAMAGE);
			if (hit > 0.0) data.Add("Legacy hit:", FormatNumber(hit), COLOR_NEUTRAL, GGDisplayStat.AMMO_DAMAGE);
		}
		return data;
	}

	protected static GGDisplayData GetArmorDisplay(GGArmorConfig armor, GGSettings settings, GGStatVisibility visible)
	{
		float projectile = armor.DetectedProjectileReduction;
		float melee = armor.DetectedMeleeReduction;
		float infected = armor.DetectedInfectedReduction;
		float frag = armor.DetectedFragReduction;

		GGDisplayData data = new GGDisplayData(GetArmorTier(projectile, settings));
		if (visible.Armor)
		{
			AddReduction(data, "Projectile:", projectile, settings);
			AddReduction(data, "Melee:", melee, settings);
			AddReduction(data, "Infected:", infected, settings);
			AddReduction(data, "Frag:", frag, settings);
		}
		return data;
	}

	protected static void AddEffectLines(GGDisplayData data, GGTierDefinition effect, GGStatVisibility visible)
	{
		if (visible.Recoil) AddMultiplier(data, "Recoil:", effect.Recoil, false, GGDisplayStat.RECOIL);
		if (visible.Sway) AddMultiplier(data, "Sway:", effect.Sway, false, GGDisplayStat.SWAY);
		if (visible.ADS) AddMultiplier(data, "ADS:", effect.ADS, true, GGDisplayStat.ADS);
		if (visible.Precision) AddMultiplier(data, "Precision:", effect.Precision, true, GGDisplayStat.PRECISION);
		if (visible.HipFire) AddMultiplier(data, "Hipfire:", effect.HipFire, false, GGDisplayStat.HIPFIRE);
	}

	protected static void AddMultiplier(GGDisplayData data, string label, float multiplier, bool higherIsBetter, int stat)
	{
		data.Add(label, FormatDelta(multiplier), GetDeltaColor(multiplier, higherIsBetter), stat);
	}

	protected static void AddReduction(GGDisplayData data, string label, float reduction, GGSettings settings)
	{
		int color = COLOR_BAD;
		if (reduction >= settings.ArmorTier2Minimum) color = COLOR_GOOD;
		else if (reduction >= settings.ArmorTier1Minimum) color = COLOR_NEUTRAL;
		data.Add(label, Math.Round(reduction).ToString() + "%", color, GGDisplayStat.ARMOR);
	}

	protected static string BuildTierTitle(string category, string tier)
	{
		return category + " " + tier;
	}

	protected static string GetArmorTier(float projectile, GGSettings settings)
	{
		if (projectile >= settings.ArmorTier3Minimum) return "Armor Tier 3";
		if (projectile >= settings.ArmorTier2Minimum) return "Armor Tier 2";
		if (projectile >= settings.ArmorTier1Minimum) return "Armor Tier 1";
		return "Clothing 0";
	}

	protected static int GetHighestWeaponRPM(GGWeaponConfig weapon)
	{
		if (!weapon || !weapon.FireModes) return 0;
		int highest;
		foreach (GGFireModeConfig mode : weapon.FireModes)
		{
			if (!mode || mode.DetectedReloadTime <= 0.0) continue;
			highest = Math.Max(highest, Math.Round(60.0 / mode.DetectedReloadTime));
		}
		return highest;
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
		float weaponMultiplier = weaponConfig.DetectedInitSpeedMultiplier;
		if (weaponMultiplier <= 0.0) weaponMultiplier = 1.0;
		if (ammoSpeed <= 0.0) ammoSpeed = 900.0;
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
		float seconds = 0.7 / aimSpeed;
		return string.Format("%1s", Math.Round(seconds * 100.0) / 100.0);
	}

	protected static string FormatDelta(float multiplier)
	{
		if (Math.AbsFloat(multiplier - 1.0) < 0.001) return "0%";
		int rounded = Math.Round((multiplier - 1.0) * 100.0);
		if (rounded > 0) return string.Format("+%1%%", rounded);
		return string.Format("%1%%", rounded);
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
