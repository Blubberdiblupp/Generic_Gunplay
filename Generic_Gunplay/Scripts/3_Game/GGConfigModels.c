class GGTierDefinition : Managed
{
	string TierKey;
	string Category;
	string Tier;
	float Recoil;
	float Sway;
	float ADS;
	float Precision;
	float HipFire;

	void GGTierDefinition(string tierKey = "Neutral", string category = "Neutral", string tier = "Neutral", float recoil = 1.0, float sway = 1.0, float ads = 1.0, float precision = 1.0, float hipFire = 1.0)
	{
		TierKey = tierKey;
		Category = category;
		Tier = tier;
		Recoil = recoil;
		Sway = sway;
		ADS = ads;
		Precision = precision;
		HipFire = hipFire;
	}

	GGTierDefinition Copy()
	{
		return new GGTierDefinition(TierKey, Category, Tier, Recoil, Sway, ADS, Precision, HipFire);
	}
}

class GGStatVisibility : Managed
{
	bool Recoil;
	bool Sway;
	bool ADS;
	bool Precision;
	bool Dispersion;
	bool HipFire;
	bool RPM;
	bool MuzzleVelocity;
	bool MagazineCapacity;
	bool AmmoBallistics;
	bool AmmoDamage;
	bool Armor;

	void GGStatVisibility()
	{
		Recoil = true;
		Sway = true;
		ADS = true;
		Precision = true;
		Dispersion = true;
		HipFire = true;
		RPM = true;
		MuzzleVelocity = true;
		MagazineCapacity = true;
		AmmoBallistics = true;
		AmmoDamage = true;
		Armor = true;
	}
}

class GGSettings : Managed
{
	int ConfigVersion;
	int CrosshairMode;
	bool AllowClientCrosshairChoice;
	bool EnableTooltipStats;
	bool EnableInspectStats;
	bool EnableExpansionMarketStats;
	bool EnableWeaponStats;
	bool EnableAttachmentStats;
	bool EnableMagazineStats;
	bool EnableAmmoStats;
	bool EnableArmorStats;
	bool EnableHipFireAlignment;
	bool EnableWeaponGeometryDamage;
	bool AutoDiscoverNewItems;
	bool PreserveMissingItems;
	bool ImportLegacyConfigsOnFirstRun;
	bool LegacyImportCompleted;
	bool DebugMode;

	float GlobalRecoilMultiplier;
	float GlobalSwayMultiplier;
	float GlobalAimSpeedMultiplier;
	float GlobalHipFireMultiplier;
	float GlobalPrecisionMultiplier;
	int HighCapMagazineThreshold;
	float ArmorTier1Minimum;
	float ArmorTier2Minimum;
	float ArmorTier3Minimum;

	ref GGStatVisibility VisibleStats;
	ref array<ref GGTierDefinition> TierDefinitions;

	void GGSettings()
	{
		SetDefaults();
	}

	void SetDefaults()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		CrosshairMode = 1;
		AllowClientCrosshairChoice = false;
		EnableTooltipStats = true;
		EnableInspectStats = true;
		EnableExpansionMarketStats = true;
		EnableWeaponStats = true;
		EnableAttachmentStats = true;
		EnableMagazineStats = true;
		EnableAmmoStats = true;
		EnableArmorStats = true;
		EnableHipFireAlignment = true;
		EnableWeaponGeometryDamage = true;
		AutoDiscoverNewItems = true;
		PreserveMissingItems = true;
		ImportLegacyConfigsOnFirstRun = true;
		LegacyImportCompleted = false;
		DebugMode = false;

		GlobalRecoilMultiplier = 1.08;
		GlobalSwayMultiplier = 1.08;
		GlobalAimSpeedMultiplier = 0.95;
		GlobalHipFireMultiplier = 1.0;
		GlobalPrecisionMultiplier = 1.0;
		HighCapMagazineThreshold = 30;
		ArmorTier1Minimum = 50.0;
		ArmorTier2Minimum = 76.0;
		ArmorTier3Minimum = 90.0;

		VisibleStats = new GGStatVisibility();
		TierDefinitions = new array<ref GGTierDefinition>;
		GGTierCatalog.FillMissing(TierDefinitions);
	}

	bool EnsureValid()
	{
		bool changed = false;
		if (ConfigVersion < 1)
		{
			ConfigVersion = GGConstants.CONFIG_VERSION;
			changed = true;
		}
		if (!VisibleStats)
		{
			VisibleStats = new GGStatVisibility();
			changed = true;
		}
		if (!TierDefinitions)
		{
			TierDefinitions = new array<ref GGTierDefinition>;
			changed = true;
		}
		if (CompactTierDefinitions()) changed = true;
		if (GGTierCatalog.FillMissing(TierDefinitions)) changed = true;

		int oldCrosshair = CrosshairMode;
		CrosshairMode = Math.Clamp(CrosshairMode, 0, 2);
		if (oldCrosshair != CrosshairMode) changed = true;
		GlobalRecoilMultiplier = GGUtil.Clamp(GlobalRecoilMultiplier, 0.01, 5.0);
		GlobalSwayMultiplier = GGUtil.Clamp(GlobalSwayMultiplier, 0.01, 5.0);
		GlobalAimSpeedMultiplier = GGUtil.Clamp(GlobalAimSpeedMultiplier, 0.01, 5.0);
		GlobalHipFireMultiplier = GGUtil.Clamp(GlobalHipFireMultiplier, 0.01, 5.0);
		GlobalPrecisionMultiplier = GGUtil.Clamp(GlobalPrecisionMultiplier, 0.01, 5.0);
		HighCapMagazineThreshold = Math.Clamp(HighCapMagazineThreshold, 1, 500);
		ArmorTier1Minimum = GGUtil.Clamp(ArmorTier1Minimum, 0.0, 100.0);
		ArmorTier2Minimum = GGUtil.Clamp(ArmorTier2Minimum, ArmorTier1Minimum, 100.0);
		ArmorTier3Minimum = GGUtil.Clamp(ArmorTier3Minimum, ArmorTier2Minimum, 100.0);

		foreach (GGTierDefinition definition : TierDefinitions)
		{
			if (!definition) continue;
			definition.Recoil = GGUtil.Clamp(definition.Recoil, 0.01, 5.0);
			definition.Sway = GGUtil.Clamp(definition.Sway, 0.01, 5.0);
			definition.ADS = GGUtil.Clamp(definition.ADS, 0.01, 5.0);
			definition.Precision = GGUtil.Clamp(definition.Precision, 0.01, 5.0);
			definition.HipFire = GGUtil.Clamp(definition.HipFire, 0.01, 5.0);
		}

		return changed;
	}

	protected bool CompactTierDefinitions()
	{
		if (!TierDefinitions) return false;
		bool changed = false;
		ref array<ref GGTierDefinition> compacted = new array<ref GGTierDefinition>;
		ref map<string, bool> known = new map<string, bool>;
		foreach (GGTierDefinition definition : TierDefinitions)
		{
			if (!definition || definition.TierKey == "")
			{
				changed = true;
				continue;
			}
			string key = GGUtil.Key(definition.TierKey);
			bool ignored;
			if (known.Find(key, ignored))
			{
				changed = true;
				continue;
			}
			known.Set(key, true);
			compacted.Insert(definition);
		}
		if (changed) TierDefinitions = compacted;
		return changed;
	}
}

class GGAttachmentConfig : Managed
{
	string ClassName;
	string ParentClass;
	string Category;
	string TierKey;
	string CustomTierLabel;
	bool UseCustomStats;
	float Recoil;
	float Sway;
	float ADS;
	float Precision;
	float HipFire;
	ref array<string> DetectedSlots;
	bool NeedsReview;
	bool IsCurrentlyLoaded;

	void GGAttachmentConfig()
	{
		ClassName = "";
		ParentClass = "";
		Category = "Neutral";
		TierKey = "Neutral";
		CustomTierLabel = "Custom";
		UseCustomStats = false;
		Recoil = 1.0;
		Sway = 1.0;
		ADS = 1.0;
		Precision = 1.0;
		HipFire = 1.0;
		DetectedSlots = new array<string>;
		NeedsReview = true;
		IsCurrentlyLoaded = true;
	}
}

class GGFireModeConfig : Managed
{
	string ModeName;
	float DetectedReloadTime;
	float DetectedDispersion;
	float RecoilMultiplier;
	float SwayMultiplier;
	float ADSMultiplier;
	float PrecisionMultiplier;
	float HipFireMultiplier;

	void GGFireModeConfig()
	{
		ModeName = "";
		DetectedReloadTime = 0.0;
		DetectedDispersion = 0.0;
		RecoilMultiplier = 1.0;
		SwayMultiplier = 1.0;
		ADSMultiplier = 1.0;
		PrecisionMultiplier = 1.0;
		HipFireMultiplier = 1.0;
	}
}

class GGWeaponConfig : Managed
{
	string ClassName;
	string ParentClass;
	float DetectedWeightKg;
	float DetectedLengthM;
	float DetectedRecoil;
	float DetectedSway;
	float DetectedADSSpeed;
	float DetectedPrecision;
	float DetectedInitSpeedMultiplier;
	float RecoilMultiplier;
	float SwayMultiplier;
	float ADSMultiplier;
	float PrecisionMultiplier;
	float HipFireMultiplier;
	ref array<ref GGFireModeConfig> FireModes;
	bool IsCurrentlyLoaded;

	void GGWeaponConfig()
	{
		ClassName = "";
		ParentClass = "";
		DetectedWeightKg = 3.0;
		DetectedLengthM = 0.7;
		DetectedRecoil = 1.0;
		DetectedSway = 1.0;
		DetectedADSSpeed = 1.0;
		DetectedPrecision = 1.0;
		DetectedInitSpeedMultiplier = 1.0;
		RecoilMultiplier = 1.0;
		SwayMultiplier = 1.0;
		ADSMultiplier = 1.0;
		PrecisionMultiplier = 1.0;
		HipFireMultiplier = 1.0;
		FireModes = new array<ref GGFireModeConfig>;
		IsCurrentlyLoaded = true;
	}
}

class GGMagazineConfig : Managed
{
	string ClassName;
	string ParentClass;
	string AmmoClass;
	int DetectedCapacity;
	bool IsLooseAmmo;
	string TierKey;
	string CustomTierLabel;
	bool UseCustomStats;
	float Recoil;
	float Sway;
	float ADS;
	float Precision;
	float HipFire;
	bool IsCurrentlyLoaded;

	void GGMagazineConfig()
	{
		ClassName = "";
		ParentClass = "";
		AmmoClass = "";
		DetectedCapacity = 0;
		IsLooseAmmo = false;
		TierKey = "StandardMag_Neutral";
		CustomTierLabel = "Custom";
		UseCustomStats = false;
		Recoil = 1.0;
		Sway = 1.0;
		ADS = 1.0;
		Precision = 1.0;
		HipFire = 1.0;
		IsCurrentlyLoaded = true;
	}
}

class GGAmmoConfig : Managed
{
	string ClassName;
	string ParentClass;
	float DetectedInitSpeed;
	float DetectedTypicalSpeed;
	float DetectedAirFriction;
	float DetectedHit;
	float DetectedIndirectHit;
	float DetectedHealthDamage;
	float DetectedBloodDamage;
	float DetectedShockDamage;
	bool IsCurrentlyLoaded;

	void GGAmmoConfig()
	{
		ClassName = "";
		ParentClass = "";
		DetectedInitSpeed = 0.0;
		DetectedTypicalSpeed = 0.0;
		DetectedAirFriction = 0.0;
		DetectedHit = 0.0;
		DetectedIndirectHit = 0.0;
		DetectedHealthDamage = 0.0;
		DetectedBloodDamage = 0.0;
		DetectedShockDamage = 0.0;
		IsCurrentlyLoaded = true;
	}
}

class GGArmorConfig : Managed
{
	string ClassName;
	string ParentClass;
	float DetectedProjectileReduction;
	float DetectedMeleeReduction;
	float DetectedInfectedReduction;
	float DetectedFragReduction;
	bool IsCurrentlyLoaded;

	void GGArmorConfig()
	{
		ClassName = "";
		ParentClass = "";
		DetectedProjectileReduction = 0.0;
		DetectedMeleeReduction = 0.0;
		DetectedInfectedReduction = 0.0;
		DetectedFragReduction = 0.0;
		IsCurrentlyLoaded = true;
	}
}

class GGItemsConfig : Managed
{
	int ConfigVersion;
	ref array<ref GGWeaponConfig> Weapons;
	ref array<ref GGAttachmentConfig> Attachments;
	ref array<ref GGMagazineConfig> Magazines;
	ref array<ref GGAmmoConfig> Ammunition;
	ref array<ref GGArmorConfig> Armor;

	void GGItemsConfig()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		Weapons = new array<ref GGWeaponConfig>;
		Attachments = new array<ref GGAttachmentConfig>;
		Magazines = new array<ref GGMagazineConfig>;
		Ammunition = new array<ref GGAmmoConfig>;
		Armor = new array<ref GGArmorConfig>;
	}

	bool EnsureArrays()
	{
		bool changed = false;
		if (!Weapons) { Weapons = new array<ref GGWeaponConfig>; changed = true; }
		if (!Attachments) { Attachments = new array<ref GGAttachmentConfig>; changed = true; }
		if (!Magazines) { Magazines = new array<ref GGMagazineConfig>; changed = true; }
		if (!Ammunition) { Ammunition = new array<ref GGAmmoConfig>; changed = true; }
		if (!Armor) { Armor = new array<ref GGArmorConfig>; changed = true; }
		if (ConfigVersion < 1) { ConfigVersion = GGConstants.CONFIG_VERSION; changed = true; }
		return changed;
	}
}

class GGWeaponsFile : Managed
{
	int ConfigVersion;
	ref array<ref GGWeaponConfig> Weapons;

	void GGWeaponsFile()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		Weapons = new array<ref GGWeaponConfig>;
	}
}

class GGAttachmentsFile : Managed
{
	int ConfigVersion;
	ref array<ref GGAttachmentConfig> Attachments;

	void GGAttachmentsFile()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		Attachments = new array<ref GGAttachmentConfig>;
	}
}

class GGMagazinesFile : Managed
{
	int ConfigVersion;
	ref array<ref GGMagazineConfig> Magazines;

	void GGMagazinesFile()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		Magazines = new array<ref GGMagazineConfig>;
	}
}

class GGAmmunitionFile : Managed
{
	int ConfigVersion;
	ref array<ref GGAmmoConfig> Ammunition;

	void GGAmmunitionFile()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		Ammunition = new array<ref GGAmmoConfig>;
	}
}

class GGClothingArmorFile : Managed
{
	int ConfigVersion;
	ref array<ref GGArmorConfig> ClothingArmor;

	void GGClothingArmorFile()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		ClothingArmor = new array<ref GGArmorConfig>;
	}
}

class GGAttachmentSlotCatalog : Managed
{
	string SlotName;
	ref array<string> Attachments;

	void GGAttachmentSlotCatalog()
	{
		SlotName = "";
		Attachments = new array<string>;
	}
}

class GGWeaponAttachmentPolicy : Managed
{
	string WeaponClassName;
	ref array<string> Slots;
	ref map<string, int> AttachmentOverrides;
	[NonSerialized()]
	bool IsCurrentlyLoaded;

	void GGWeaponAttachmentPolicy()
	{
		WeaponClassName = "";
		Slots = new array<string>;
		AttachmentOverrides = new map<string, int>;
		IsCurrentlyLoaded = true;
	}
}

class GGWeaponAttachmentsFile : Managed
{
	int ConfigVersion;
	int FormatVersion;
	ref array<ref GGAttachmentSlotCatalog> SlotCatalog;
	ref array<ref GGWeaponAttachmentPolicy> Weapons;

	void GGWeaponAttachmentsFile()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		FormatVersion = 0;
		SlotCatalog = new array<ref GGAttachmentSlotCatalog>;
		Weapons = new array<ref GGWeaponAttachmentPolicy>;
	}

	void EnsureArrays()
	{
		if (!SlotCatalog) SlotCatalog = new array<ref GGAttachmentSlotCatalog>;
		foreach (GGAttachmentSlotCatalog slot : SlotCatalog)
		{
			if (slot && !slot.Attachments) slot.Attachments = new array<string>;
		}
		if (!Weapons) Weapons = new array<ref GGWeaponAttachmentPolicy>;
		foreach (GGWeaponAttachmentPolicy policy : Weapons)
		{
			if (!policy) continue;
			if (!policy.Slots) policy.Slots = new array<string>;
			if (!policy.AttachmentOverrides) policy.AttachmentOverrides = new map<string, int>;
		}
	}
}

class GGBlockedAttachmentRule : Managed
{
	string WeaponClassName;
	string AttachmentClassName;

	void GGBlockedAttachmentRule(string weaponClassName = "", string attachmentClassName = "")
	{
		WeaponClassName = weaponClassName;
		AttachmentClassName = attachmentClassName;
	}
}

class GGClientSettings : Managed
{
	int ConfigVersion;
	int CrosshairMode;

	void GGClientSettings()
	{
		ConfigVersion = GGConstants.CONFIG_VERSION;
		CrosshairMode = 1;
	}
}

class GGSyncPayload : Managed
{
	int ProtocolVersion;
	ref GGSettings Settings;
	ref GGItemsConfig Items;
	ref array<ref GGBlockedAttachmentRule> BlockedAttachments;

	void GGSyncPayload()
	{
		ProtocolVersion = GGConstants.SYNC_PROTOCOL_VERSION;
		Settings = new GGSettings();
		Items = new GGItemsConfig();
		BlockedAttachments = new array<ref GGBlockedAttachmentRule>;
	}
}
