class GGLegacyTierAssignment : Managed
{
	string ClassName;
	string TierKey;
}

class GGLegacyTierMultiplier : Managed
{
	string TierKey;
	float Recoil;
	float Sway;
	float ADS;
	float Precision;
	float HipFire;
}

class GGLegacyItemStats : Managed
{
	string ClassName;
	string Category;
	string Tier;
	float Recoil;
	float Sway;
	float ADS;
	float Precision;
	float HipFire;
}

class GGLegacyConfig : Managed
{
	int ConfigVersion;
	int CrosshairMode;
	bool EnableTooltipStats;
	bool EnableInspectStats;
	bool EnableExpansionMarketStats;
	bool EnableHipFireNerf;
	bool EnableWeaponGeometryDamage;
	bool DebugMode;
	float GlobalRecoilMultiplier;
	float GlobalSwayMultiplier;
	float GlobalAimSpeedMultiplier;
	float GlobalHipFireMultiplier;
	float GlobalPrecisionMultiplier;
	int HighCapMagazineThreshold;
	ref array<ref GGLegacyTierAssignment> AttachmentTierOverrides;
	ref array<ref GGLegacyTierMultiplier> TierMultiplierOverrides;
	ref array<ref GGLegacyItemStats> AttachmentStatOverrides;

	void GGLegacyConfig()
	{
		ConfigVersion = 1;
		CrosshairMode = 1;
		EnableTooltipStats = true;
		EnableInspectStats = true;
		EnableExpansionMarketStats = true;
		EnableHipFireNerf = true;
		EnableWeaponGeometryDamage = true;
		DebugMode = false;
		GlobalRecoilMultiplier = 1.08;
		GlobalSwayMultiplier = 1.08;
		GlobalAimSpeedMultiplier = 0.95;
		GlobalHipFireMultiplier = 1.0;
		GlobalPrecisionMultiplier = 1.0;
		HighCapMagazineThreshold = 30;
		AttachmentTierOverrides = new array<ref GGLegacyTierAssignment>;
		TierMultiplierOverrides = new array<ref GGLegacyTierMultiplier>;
		AttachmentStatOverrides = new array<ref GGLegacyItemStats>;
	}
}

class GGLegacyMigration
{
	static bool Import(GGSettings settings, GGItemsConfig items)
	{
		if (!g_Game || !g_Game.IsServer() || !settings || !items) return false;

		ref array<string> paths = new array<string>;
		paths.Insert("$profile:\\Buca_Gunplay\\Buca_Gunplay.json");
		paths.Insert("$profile:\\DeltaForce_Gunplay\\DeltaForce_Gunplay.json");
		paths.Insert("$profile:\\Mortys_Gunplay\\Mortys_Gunplay.json");
		paths.Insert("$profile:\\AJ_Gunplay\\AJ_Gunplay.json");
		paths.Insert("$profile:\\SNAFU_Gunplay\\SNAFU_Gunplay.json");

		ref array<ref GGLegacyConfig> loaded = new array<ref GGLegacyConfig>;
		ref array<string> loadedPaths = new array<string>;
		foreach (string path : paths)
		{
			if (!FileExist(path)) continue;
			GGLegacyConfig legacy = new GGLegacyConfig();
			string error;
			if (!JsonFileLoader<GGLegacyConfig>.LoadFile(path, legacy, error))
			{
				GGUtil.Warning("Legacy config was not imported because it is invalid: " + path + " - " + error);
				continue;
			}
			loaded.Insert(legacy);
			loadedPaths.Insert(path);
		}

		if (loaded.Count() == 0) return false;
		ref map<string, ref GGAttachmentConfig> attachmentMap = new map<string, ref GGAttachmentConfig>;
		foreach (GGAttachmentConfig attachment : items.Attachments)
		{
			if (attachment) attachmentMap.Set(GGUtil.Key(attachment.ClassName), attachment);
		}
		ref map<string, ref GGMagazineConfig> magazineMap = new map<string, ref GGMagazineConfig>;
		foreach (GGMagazineConfig magazine : items.Magazines)
		{
			if (magazine) magazineMap.Set(GGUtil.Key(magazine.ClassName), magazine);
		}

		ref map<string, ref GGLegacyTierMultiplier> tierMultipliers = new map<string, ref GGLegacyTierMultiplier>;
		foreach (GGLegacyConfig source : loaded)
		{
			if (!source) continue;
			if (source.AttachmentTierOverrides)
			{
				foreach (GGLegacyTierAssignment assignment : source.AttachmentTierOverrides)
				{
					if (!assignment) continue;
					GGAttachmentConfig assigned;
					if (attachmentMap.Find(GGUtil.Key(assignment.ClassName), assigned) && assignment.TierKey != "")
					{
						assigned.TierKey = assignment.TierKey;
						assigned.Category = CategoryFromTier(assignment.TierKey);
						assigned.NeedsReview = false;
					}
					GGMagazineConfig assignedMagazine;
					if (magazineMap.Find(GGUtil.Key(assignment.ClassName), assignedMagazine) && assignment.TierKey != "")
						assignedMagazine.TierKey = assignment.TierKey;
				}
			}
			if (source.TierMultiplierOverrides)
			{
				foreach (GGLegacyTierMultiplier multiplier : source.TierMultiplierOverrides)
				{
					if (multiplier && multiplier.TierKey != "") tierMultipliers.Set(GGUtil.Key(multiplier.TierKey), multiplier);
				}
			}
		}

		ref array<ref GGTierDefinition> defaultTiers = new array<ref GGTierDefinition>;
		GGTierCatalog.FillMissing(defaultTiers);
		foreach (GGTierDefinition definition : settings.TierDefinitions)
		{
			if (!definition) continue;
			GGLegacyTierMultiplier oldMultiplier;
			if (!tierMultipliers.Find(GGUtil.Key(definition.TierKey), oldMultiplier)) continue;
			GGTierDefinition defaultDefinition = FindTier(defaultTiers, definition.TierKey);
			if (defaultDefinition)
			{
				definition.Category = defaultDefinition.Category;
				definition.Tier = defaultDefinition.Tier;
				definition.Recoil = defaultDefinition.Recoil * SafeMultiplier(oldMultiplier.Recoil);
				definition.Sway = defaultDefinition.Sway * SafeMultiplier(oldMultiplier.Sway);
				definition.ADS = defaultDefinition.ADS * SafeMultiplier(oldMultiplier.ADS);
				definition.Precision = defaultDefinition.Precision * SafeMultiplier(oldMultiplier.Precision);
				definition.HipFire = defaultDefinition.HipFire * SafeMultiplier(oldMultiplier.HipFire);
			}
		}

		foreach (GGLegacyConfig statSource : loaded)
		{
			if (!statSource) continue;
			if (statSource.AttachmentStatOverrides)
			{
				foreach (GGLegacyItemStats oldStats : statSource.AttachmentStatOverrides)
				{
					if (!oldStats) continue;
					GGAttachmentConfig item;
					if (attachmentMap.Find(GGUtil.Key(oldStats.ClassName), item))
					{
						item.UseCustomStats = !IsDefaultLegacyStats(oldStats);
						if (item.UseCustomStats)
						{
							item.Category = oldStats.Category;
							CopyStatsToAttachment(oldStats, item);
						}
					}

					GGMagazineConfig magazineItem;
					if (magazineMap.Find(GGUtil.Key(oldStats.ClassName), magazineItem))
					{
						magazineItem.UseCustomStats = !IsDefaultLegacyStats(oldStats);
						if (magazineItem.UseCustomStats) CopyStatsToMagazine(oldStats, magazineItem);
					}
				}
			}
		}

		GGLegacyConfig highest = loaded.Get(loaded.Count() - 1);
		if (highest)
		{
			settings.CrosshairMode = Math.Clamp(highest.CrosshairMode, 0, 2);
			settings.EnableTooltipStats = highest.EnableTooltipStats;
			settings.EnableInspectStats = highest.EnableInspectStats;
			settings.EnableExpansionMarketStats = highest.EnableExpansionMarketStats;
			settings.EnableHipFireAlignment = highest.EnableHipFireNerf;
			settings.EnableWeaponGeometryDamage = highest.EnableWeaponGeometryDamage;
			if (highest.DebugMode)
				settings.DebugMode = 1;
			else
				settings.DebugMode = 0;
			settings.GlobalRecoilMultiplier = SafeMultiplier(highest.GlobalRecoilMultiplier);
			settings.GlobalSwayMultiplier = SafeMultiplier(highest.GlobalSwayMultiplier);
			settings.GlobalAimSpeedMultiplier = SafeMultiplier(highest.GlobalAimSpeedMultiplier);
			settings.GlobalHipFireMultiplier = SafeMultiplier(highest.GlobalHipFireMultiplier);
			settings.GlobalPrecisionMultiplier = SafeMultiplier(highest.GlobalPrecisionMultiplier);
			if (highest.HighCapMagazineThreshold > 0) settings.HighCapMagazineThreshold = highest.HighCapMagazineThreshold;
		}

		GGUtil.Log("Imported " + loaded.Count().ToString() + " legacy profile config(s). Higher-priority values override lower-priority duplicates.");
		return true;
	}

	protected static bool IsDefaultLegacyStats(GGLegacyItemStats stats)
	{
		if (!stats) return false;
		ref array<ref GGTierDefinition> defaults = new array<ref GGTierDefinition>;
		GGTierCatalog.FillMissing(defaults);
		foreach (GGTierDefinition definition : defaults)
		{
			if (!definition || stats.Category != definition.Category || stats.Tier != definition.Tier) continue;
			if (!GGUtil.NearlyEqual(stats.Recoil, definition.Recoil, 0.0001)) continue;
			if (!GGUtil.NearlyEqual(stats.Sway, definition.Sway, 0.0001)) continue;
			if (!GGUtil.NearlyEqual(stats.ADS, definition.ADS, 0.0001)) continue;
			if (!GGUtil.NearlyEqual(stats.Precision, definition.Precision, 0.0001)) continue;
			if (!GGUtil.NearlyEqual(stats.HipFire, definition.HipFire, 0.0001)) continue;
			return true;
		}
		return false;
	}

	protected static void CopyStatsToAttachment(GGLegacyItemStats source, GGAttachmentConfig target)
	{
		target.CustomTierLabel = source.Tier;
		target.Recoil = source.Recoil;
		target.Sway = source.Sway;
		target.ADS = source.ADS;
		target.Precision = source.Precision;
		target.HipFire = source.HipFire;
	}

	protected static void CopyStatsToMagazine(GGLegacyItemStats source, GGMagazineConfig target)
	{
		target.CustomTierLabel = source.Tier;
		target.Recoil = source.Recoil;
		target.Sway = source.Sway;
		target.ADS = source.ADS;
		target.Precision = source.Precision;
		target.HipFire = source.HipFire;
	}

	protected static float SafeMultiplier(float value)
	{
		if (value <= 0.0) return 1.0;
		return GGUtil.Clamp(value, 0.01, 5.0);
	}

	protected static GGTierDefinition FindTier(array<ref GGTierDefinition> definitions, string tierKey)
	{
		if (!definitions) return null;
		foreach (GGTierDefinition definition : definitions)
		{
			if (definition && GGUtil.Key(definition.TierKey) == GGUtil.Key(tierKey)) return definition;
		}
		return null;
	}

	protected static string CategoryFromTier(string tierKey)
	{
		if (tierKey.IndexOf("Foregrip_") == 0) return "Foregrip";
		if (tierKey.IndexOf("PistolGrip_") == 0) return "Pistol Grip";
		if (tierKey.IndexOf("Stock_") == 0) return "Stock";
		if (tierKey.IndexOf("Handguard_") == 0) return "Handguard";
		if (tierKey.Contains("Optic_")) return "Optic";
		if (tierKey.IndexOf("Suppressor_") == 0) return "Suppressor";
		if (tierKey.IndexOf("Muzzle_") == 0) return "Muzzle";
		if (tierKey.IndexOf("Laser_") == 0) return "Laser";
		if (tierKey.IndexOf("Flashlight_") == 0) return "Flashlight";
		if (tierKey.IndexOf("Bayonet_") == 0) return "Bayonet";
		if (tierKey.IndexOf("Bipod_") == 0) return "Bipod";
		if (tierKey.IndexOf("WeaponWrap_") == 0) return "Wrap";
		return "Neutral";
	}
}
