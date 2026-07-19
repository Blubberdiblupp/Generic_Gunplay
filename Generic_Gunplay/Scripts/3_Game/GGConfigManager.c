class GGConfigManager
{
	protected ref GGSettings m_Settings;
	protected ref GGItemsConfig m_Items;
	protected ref GGWeaponAttachmentsFile m_WeaponAttachments;
	protected ref GGClientSettings m_ClientSettings;
	protected ref array<ref GGBlockedAttachmentRule> m_SyncedBlockedAttachments;
	protected ref map<string, ref GGTierDefinition> m_Tiers;
	protected ref map<string, ref GGWeaponConfig> m_Weapons;
	protected ref map<string, ref GGAttachmentConfig> m_Attachments;
	protected ref map<string, ref GGMagazineConfig> m_Magazines;
	protected ref map<string, ref GGAmmoConfig> m_Ammo;
	protected ref map<string, ref GGArmorConfig> m_Armor;
	protected ref map<string, bool> m_BlockedAttachments;
	protected bool m_Ready;
	protected bool m_Loading;
	protected bool m_LoadError;
	protected int m_RuntimeRevision;

	void GGConfigManager()
	{
		m_Settings = new GGSettings();
		m_Items = new GGItemsConfig();
		m_WeaponAttachments = new GGWeaponAttachmentsFile();
		m_ClientSettings = new GGClientSettings();
		m_SyncedBlockedAttachments = new array<ref GGBlockedAttachmentRule>;
		m_Tiers = new map<string, ref GGTierDefinition>;
		m_Weapons = new map<string, ref GGWeaponConfig>;
		m_Attachments = new map<string, ref GGAttachmentConfig>;
		m_Magazines = new map<string, ref GGMagazineConfig>;
		m_Ammo = new map<string, ref GGAmmoConfig>;
		m_Armor = new map<string, ref GGArmorConfig>;
		m_BlockedAttachments = new map<string, bool>;
	}

	void EnsureReady()
	{
		if (m_Ready || m_Loading || m_LoadError || !g_Game) return;
		if (g_Game.IsServer()) LoadServerConfig();
		else InitializeClient();
	}

	void LoadServerConfig()
	{
		if (m_Ready) return;
		if (m_Loading) return;
		if (m_LoadError) return;
		if (!g_Game) return;
		if (!g_Game.IsServer()) return;
		GGNetworkSync.InvalidateServerCache();
		m_Loading = true;
		m_LoadError = false;
		if (!FileExist(GGConstants.CONFIG_DIR)) MakeDirectory(GGConstants.CONFIG_DIR);

		bool settingsExisted = FileExist(GGConstants.SETTINGS_FILE);
		string error;
		if (settingsExisted && !JsonFileLoader<GGSettings>.LoadFile(GGConstants.SETTINGS_FILE, m_Settings, error))
		{
			GGUtil.Error("Settings.json is invalid; it was not overwritten. " + error);
			m_LoadError = true;
		}
		if (!LoadServerItemFiles()) m_LoadError = true;

		if (m_LoadError)
		{
			m_Loading = false;
			return;
		}
		bool migrationChanged;
		string migrationError;
		if (!GGConfigMigration.Migrate(m_Settings, m_Items, migrationChanged, migrationError))
		{
			GGUtil.Error(migrationError + " Files were not overwritten.");
			m_LoadError = true;
			m_Loading = false;
			return;
		}

		m_Settings.EnsureValid();
		m_Items.EnsureArrays();
		m_WeaponAttachments.EnsureArrays();
		CompactItems();
		CompactWeaponAttachmentPolicies();
		if (m_Settings.AutoDiscoverNewItems)
		{
			GGDiscovery discovery = new GGDiscovery();
			discovery.ScanAndMerge(m_Items, m_Settings, m_WeaponAttachments);
		}

		if (m_Settings.ImportLegacyConfigsOnFirstRun && !m_Settings.LegacyImportCompleted)
		{
			GGLegacyMigration.Import(m_Settings, m_Items);
			m_Settings.LegacyImportCompleted = true;
		}

		ValidateItems();
		CompactWeaponAttachmentPolicies();
		BuildCaches();
		if (!SaveServerFiles())
		{
			m_LoadError = true;
			m_Loading = false;
			return;
		}

		WarnAboutOldGunplayMods();
		m_Ready = true;
		m_Loading = false;
		GGUtil.Log("Config version " + m_Settings.ConfigVersion.ToString() + " loaded. Changes are fixed for this process and apply after a server restart.");
	}

	void InitializeClient()
	{
		if (m_Ready || m_Loading || !g_Game || g_Game.IsServer()) return;
		m_Loading = true;
		if (!FileExist(GGConstants.CONFIG_DIR)) MakeDirectory(GGConstants.CONFIG_DIR);
		string error;
		if (FileExist(GGConstants.CLIENT_FILE))
		{
			if (!JsonFileLoader<GGClientSettings>.LoadFile(GGConstants.CLIENT_FILE, m_ClientSettings, error))
				GGUtil.Warning("Client.json is invalid; defaults are used. " + error);
		}
		else
		{
			JsonFileLoader<GGClientSettings>.SaveFile(GGConstants.CLIENT_FILE, m_ClientSettings, error);
		}
		m_ClientSettings.CrosshairMode = Math.Clamp(m_ClientSettings.CrosshairMode, 0, 2);
		BuildCaches();
		m_Ready = true;
		m_Loading = false;
	}

	void ApplySyncedPayload(GGSyncPayload payload)
	{
		if (!payload || payload.ProtocolVersion != GGConstants.SYNC_PROTOCOL_VERSION || !payload.Settings || !payload.Items)
		{
			GGUtil.Error("Rejected invalid synchronized config payload.");
			return;
		}
		m_Settings = payload.Settings;
		m_Items = payload.Items;
		m_SyncedBlockedAttachments = payload.BlockedAttachments;
		if (!m_SyncedBlockedAttachments)
			m_SyncedBlockedAttachments = new array<ref GGBlockedAttachmentRule>;
		m_Settings.EnsureValid();
		m_Items.EnsureArrays();
		CompactItems();
		ValidateItems();
		BuildCaches();
		m_Ready = true;
		int itemCount = m_Items.Weapons.Count();
		itemCount += m_Items.Attachments.Count();
		itemCount += m_Items.Magazines.Count();
		itemCount += m_Items.Ammunition.Count();
		itemCount += m_Items.Armor.Count();
		GGUtil.Log("Server config synchronized to client. Items=" + itemCount.ToString() + ".");
	}

	GGSyncPayload CreateSyncPayload()
	{
		EnsureReady();
		GGSyncPayload payload = new GGSyncPayload();
		payload.Settings = m_Settings;
		payload.Items = CreateLoadedItemsSnapshot();
		payload.BlockedAttachments = CreateBlockedAttachmentSnapshot();
		return payload;
	}

	int GetRuntimeRevision()
	{
		EnsureReady();
		return m_RuntimeRevision;
	}

	GGSettings GetSettings()
	{
		EnsureReady();
		return m_Settings;
	}

	GGItemsConfig GetItems()
	{
		EnsureReady();
		return m_Items;
	}

	GGWeaponAttachmentsFile GetWeaponAttachments()
	{
		EnsureReady();
		return m_WeaponAttachments;
	}

	bool IsAttachmentAllowed(string weaponClassName, string attachmentClassName)
	{
		EnsureReady();
		bool blocked;
		string key = AttachmentPolicyKey(weaponClassName, attachmentClassName);
		return !m_BlockedAttachments.Find(key, blocked);
	}

	bool HasLoadError()
	{
		return m_LoadError;
	}

	int GetEffectiveCrosshairMode()
	{
		EnsureReady();
		if (m_Settings.AllowClientCrosshairChoice && !g_Game.IsServer())
			return Math.Clamp(m_ClientSettings.CrosshairMode, 0, 2);
		return Math.Clamp(m_Settings.CrosshairMode, 0, 2);
	}

	GGTierDefinition GetTier(string tierKey)
	{
		EnsureReady();
		GGTierDefinition definition;
		if (m_Tiers.Find(GGUtil.Key(tierKey), definition)) return definition;
		if (m_Tiers.Find("neutral", definition)) return definition;
		return new GGTierDefinition();
	}

	GGTierDefinition GetAttachmentEffect(string className)
	{
		EnsureReady();
		GGAttachmentConfig item;
		if (!m_Attachments.Find(GGUtil.Key(className), item)) return null;
		if (!item.UseCustomStats) return GetTier(item.TierKey).Copy();
		string tierLabel = item.CustomTierLabel;
		if (tierLabel == "") tierLabel = "Custom";
		return new GGTierDefinition(item.TierKey, item.Category, tierLabel, item.Recoil, item.Sway, item.ADS, item.Precision, item.HipFire);
	}

	GGTierDefinition GetMagazineEffect(string className)
	{
		EnsureReady();
		GGMagazineConfig item;
		if (!m_Magazines.Find(GGUtil.Key(className), item) || item.IsLooseAmmo) return null;
		if (!item.UseCustomStats) return GetTier(item.TierKey).Copy();
		string tierLabel = item.CustomTierLabel;
		if (tierLabel == "") tierLabel = "Custom";
		return new GGTierDefinition(item.TierKey, "Magazine", tierLabel, item.Recoil, item.Sway, item.ADS, item.Precision, item.HipFire);
	}

	GGWeaponConfig GetWeapon(string className)
	{
		EnsureReady();
		GGWeaponConfig value;
		m_Weapons.Find(GGUtil.Key(className), value);
		return value;
	}

	GGAttachmentConfig GetAttachment(string className)
	{
		EnsureReady();
		GGAttachmentConfig value;
		m_Attachments.Find(GGUtil.Key(className), value);
		return value;
	}

	GGMagazineConfig GetMagazine(string className)
	{
		EnsureReady();
		GGMagazineConfig value;
		m_Magazines.Find(GGUtil.Key(className), value);
		return value;
	}

	GGAmmoConfig GetAmmo(string className)
	{
		EnsureReady();
		GGAmmoConfig value;
		m_Ammo.Find(GGUtil.Key(className), value);
		return value;
	}

	GGArmorConfig GetArmor(string className)
	{
		EnsureReady();
		GGArmorConfig value;
		m_Armor.Find(GGUtil.Key(className), value);
		return value;
	}

	GGFireModeConfig GetFireMode(GGWeaponConfig weapon, string modeName)
	{
		if (!weapon || !weapon.FireModes) return null;
		foreach (GGFireModeConfig mode : weapon.FireModes)
		{
			if (mode && GGUtil.Key(mode.ModeName) == GGUtil.Key(modeName)) return mode;
		}
		return null;
	}

	protected bool LoadServerItemFiles()
	{
		bool splitFilesComplete = HasCompleteSplitFileSet();
		bool usedLegacyItems = false;
		if (!splitFilesComplete && FileExist(GGConstants.LEGACY_ITEMS_FILE))
		{
			string legacyError;
			if (!JsonFileLoader<GGItemsConfig>.LoadFile(GGConstants.LEGACY_ITEMS_FILE, m_Items, legacyError))
			{
				GGUtil.Error("Items.json migration source is invalid; it was not overwritten. " + legacyError);
				return false;
			}
			if (!ValidateFileVersion("Items.json", m_Items.ConfigVersion)) return false;
			m_Items.EnsureArrays();
			usedLegacyItems = true;
		}

		if (!LoadWeaponsFile()) return false;
		if (!LoadAttachmentsFile()) return false;
		if (!LoadMagazinesFile()) return false;
		if (!LoadAmmunitionFile()) return false;
		if (!LoadClothingArmorFile()) return false;
		if (!LoadWeaponAttachmentsFile()) return false;

		if (usedLegacyItems)
			GGUtil.Log("Items.json was imported as the migration source. It remains untouched as a backup.");
		return true;
	}

	protected bool HasCompleteSplitFileSet()
	{
		if (!FileExist(GGConstants.WEAPONS_FILE)) return false;
		if (!FileExist(GGConstants.ATTACHMENTS_FILE)) return false;
		if (!FileExist(GGConstants.MAGAZINES_FILE)) return false;
		if (!FileExist(GGConstants.AMMUNITION_FILE)) return false;
		if (!FileExist(GGConstants.CLOTHING_ARMOR_FILE)) return false;
		return true;
	}

	protected bool LoadWeaponsFile()
	{
		if (!FileExist(GGConstants.WEAPONS_FILE)) return true;
		GGWeaponsFile data = new GGWeaponsFile();
		string error;
		if (!JsonFileLoader<GGWeaponsFile>.LoadFile(GGConstants.WEAPONS_FILE, data, error))
		{
			GGUtil.Error("Weapons.json is invalid; it was not overwritten. " + error);
			return false;
		}
		if (!ValidateFileVersion("Weapons.json", data.ConfigVersion)) return false;
		if (!data.Weapons) data.Weapons = new array<ref GGWeaponConfig>;
		m_Items.Weapons = data.Weapons;
		return true;
	}

	protected bool LoadAttachmentsFile()
	{
		if (!FileExist(GGConstants.ATTACHMENTS_FILE)) return true;
		GGAttachmentsFile data = new GGAttachmentsFile();
		string error;
		if (!JsonFileLoader<GGAttachmentsFile>.LoadFile(GGConstants.ATTACHMENTS_FILE, data, error))
		{
			GGUtil.Error("Attachments.json is invalid; it was not overwritten. " + error);
			return false;
		}
		if (!ValidateFileVersion("Attachments.json", data.ConfigVersion)) return false;
		if (!data.Attachments) data.Attachments = new array<ref GGAttachmentConfig>;
		m_Items.Attachments = data.Attachments;
		return true;
	}

	protected bool LoadMagazinesFile()
	{
		if (!FileExist(GGConstants.MAGAZINES_FILE)) return true;
		GGMagazinesFile data = new GGMagazinesFile();
		string error;
		if (!JsonFileLoader<GGMagazinesFile>.LoadFile(GGConstants.MAGAZINES_FILE, data, error))
		{
			GGUtil.Error("Magazines.json is invalid; it was not overwritten. " + error);
			return false;
		}
		if (!ValidateFileVersion("Magazines.json", data.ConfigVersion)) return false;
		if (!data.Magazines) data.Magazines = new array<ref GGMagazineConfig>;
		m_Items.Magazines = data.Magazines;
		return true;
	}

	protected bool LoadAmmunitionFile()
	{
		if (!FileExist(GGConstants.AMMUNITION_FILE)) return true;
		GGAmmunitionFile data = new GGAmmunitionFile();
		string error;
		if (!JsonFileLoader<GGAmmunitionFile>.LoadFile(GGConstants.AMMUNITION_FILE, data, error))
		{
			GGUtil.Error("Ammunition.json is invalid; it was not overwritten. " + error);
			return false;
		}
		if (!ValidateFileVersion("Ammunition.json", data.ConfigVersion)) return false;
		if (!data.Ammunition) data.Ammunition = new array<ref GGAmmoConfig>;
		m_Items.Ammunition = data.Ammunition;
		return true;
	}

	protected bool LoadClothingArmorFile()
	{
		if (!FileExist(GGConstants.CLOTHING_ARMOR_FILE)) return true;
		GGClothingArmorFile data = new GGClothingArmorFile();
		string error;
		if (!JsonFileLoader<GGClothingArmorFile>.LoadFile(GGConstants.CLOTHING_ARMOR_FILE, data, error))
		{
			GGUtil.Error("ClothingArmor.json is invalid; it was not overwritten. " + error);
			return false;
		}
		if (!ValidateFileVersion("ClothingArmor.json", data.ConfigVersion)) return false;
		if (!data.ClothingArmor) data.ClothingArmor = new array<ref GGArmorConfig>;
		m_Items.Armor = data.ClothingArmor;
		return true;
	}

	protected bool LoadWeaponAttachmentsFile()
	{
		if (!FileExist(GGConstants.WEAPON_ATTACHMENTS_FILE)) return true;
		GGWeaponAttachmentsFile data = new GGWeaponAttachmentsFile();
		string error;
		if (!JsonFileLoader<GGWeaponAttachmentsFile>.LoadFile(GGConstants.WEAPON_ATTACHMENTS_FILE, data, error))
		{
			GGUtil.Error("WeaponAttachments.json is invalid; it was not overwritten. " + error);
			return false;
		}
		if (!ValidateFileVersion("WeaponAttachments.json", data.ConfigVersion)) return false;
		if (data.FormatVersion != GGConstants.WEAPON_ATTACHMENTS_FORMAT_VERSION)
		{
			GGUtil.Error("WeaponAttachments.json uses an unsupported test format. Delete it once so Generic_Gunplay can generate the compact format.");
			return false;
		}
		data.EnsureArrays();
		m_WeaponAttachments = data;
		return true;
	}

	protected bool ValidateFileVersion(string fileName, int version)
	{
		if (version <= GGConstants.CONFIG_VERSION) return true;
		GGUtil.Error(fileName + " was written by a newer Generic_Gunplay version.");
		return false;
	}

	protected bool SaveServerFiles()
	{
		string error;
		GGWeaponsFile weapons = new GGWeaponsFile();
		weapons.ConfigVersion = GGConstants.CONFIG_VERSION;
		weapons.Weapons = m_Items.Weapons;
		if (!JsonFileLoader<GGWeaponsFile>.SaveFile(GGConstants.WEAPONS_FILE, weapons, error))
		{
			GGUtil.Error("Could not save Weapons.json: " + error);
			return false;
		}

		GGAttachmentsFile attachments = new GGAttachmentsFile();
		attachments.ConfigVersion = GGConstants.CONFIG_VERSION;
		attachments.Attachments = m_Items.Attachments;
		if (!JsonFileLoader<GGAttachmentsFile>.SaveFile(GGConstants.ATTACHMENTS_FILE, attachments, error))
		{
			GGUtil.Error("Could not save Attachments.json: " + error);
			return false;
		}

		GGMagazinesFile magazines = new GGMagazinesFile();
		magazines.ConfigVersion = GGConstants.CONFIG_VERSION;
		magazines.Magazines = m_Items.Magazines;
		if (!JsonFileLoader<GGMagazinesFile>.SaveFile(GGConstants.MAGAZINES_FILE, magazines, error))
		{
			GGUtil.Error("Could not save Magazines.json: " + error);
			return false;
		}

		GGAmmunitionFile ammunition = new GGAmmunitionFile();
		ammunition.ConfigVersion = GGConstants.CONFIG_VERSION;
		ammunition.Ammunition = m_Items.Ammunition;
		if (!JsonFileLoader<GGAmmunitionFile>.SaveFile(GGConstants.AMMUNITION_FILE, ammunition, error))
		{
			GGUtil.Error("Could not save Ammunition.json: " + error);
			return false;
		}

		GGClothingArmorFile clothingArmor = new GGClothingArmorFile();
		clothingArmor.ConfigVersion = GGConstants.CONFIG_VERSION;
		clothingArmor.ClothingArmor = m_Items.Armor;
		if (!JsonFileLoader<GGClothingArmorFile>.SaveFile(GGConstants.CLOTHING_ARMOR_FILE, clothingArmor, error))
		{
			GGUtil.Error("Could not save ClothingArmor.json: " + error);
			return false;
		}

		m_WeaponAttachments.ConfigVersion = GGConstants.CONFIG_VERSION;
		m_WeaponAttachments.FormatVersion = GGConstants.WEAPON_ATTACHMENTS_FORMAT_VERSION;
		if (!JsonFileLoader<GGWeaponAttachmentsFile>.SaveFile(GGConstants.WEAPON_ATTACHMENTS_FILE, m_WeaponAttachments, error))
		{
			GGUtil.Error("Could not save WeaponAttachments.json: " + error);
			return false;
		}
		if (!JsonFileLoader<GGSettings>.SaveFile(GGConstants.SETTINGS_FILE, m_Settings, error))
		{
			GGUtil.Error("Could not save Settings.json: " + error);
			return false;
		}
		return true;
	}

	protected void BuildCaches()
	{
		m_Tiers.Clear();
		m_Weapons.Clear();
		m_Attachments.Clear();
		m_Magazines.Clear();
		m_Ammo.Clear();
		m_Armor.Clear();
		m_BlockedAttachments.Clear();
		foreach (GGTierDefinition tier : m_Settings.TierDefinitions) if (tier) m_Tiers.Set(GGUtil.Key(tier.TierKey), tier);
		foreach (GGWeaponConfig weapon : m_Items.Weapons) if (weapon) m_Weapons.Set(GGUtil.Key(weapon.ClassName), weapon);
		foreach (GGAttachmentConfig attachment : m_Items.Attachments) if (attachment) m_Attachments.Set(GGUtil.Key(attachment.ClassName), attachment);
		foreach (GGMagazineConfig magazine : m_Items.Magazines) if (magazine) m_Magazines.Set(GGUtil.Key(magazine.ClassName), magazine);
		foreach (GGAmmoConfig ammo : m_Items.Ammunition) if (ammo) m_Ammo.Set(GGUtil.Key(ammo.ClassName), ammo);
		foreach (GGArmorConfig armor : m_Items.Armor) if (armor) m_Armor.Set(GGUtil.Key(armor.ClassName), armor);
		BuildBlockedAttachmentCache();
		m_RuntimeRevision++;
	}

	protected void BuildBlockedAttachmentCache()
	{
		if (g_Game && g_Game.IsServer())
		{
			if (!m_WeaponAttachments || !m_WeaponAttachments.Weapons) return;
			foreach (GGWeaponAttachmentPolicy policy : m_WeaponAttachments.Weapons)
			{
				if (!policy || !policy.IsCurrentlyLoaded || !policy.AttachmentOverrides) continue;
				foreach (string attachmentClassName, int allowed : policy.AttachmentOverrides)
				{
					if (attachmentClassName == "" || allowed != 0) continue;
					string serverKey = AttachmentPolicyKey(policy.WeaponClassName, attachmentClassName);
					m_BlockedAttachments.Set(serverKey, true);
				}
			}
			return;
		}

		if (!m_SyncedBlockedAttachments) return;
		foreach (GGBlockedAttachmentRule syncedRule : m_SyncedBlockedAttachments)
		{
			if (!syncedRule) continue;
			string clientKey = AttachmentPolicyKey(syncedRule.WeaponClassName, syncedRule.AttachmentClassName);
			m_BlockedAttachments.Set(clientKey, true);
		}
	}

	protected string AttachmentPolicyKey(string weaponClassName, string attachmentClassName)
	{
		string key = GGUtil.Key(weaponClassName);
		key += "|";
		key += GGUtil.Key(attachmentClassName);
		return key;
	}

	protected GGItemsConfig CreateLoadedItemsSnapshot()
	{
		GGItemsConfig loaded = new GGItemsConfig();
		loaded.ConfigVersion = m_Items.ConfigVersion;
		foreach (GGWeaponConfig weapon : m_Items.Weapons) if (weapon && weapon.IsCurrentlyLoaded) loaded.Weapons.Insert(weapon);
		foreach (GGAttachmentConfig attachment : m_Items.Attachments) if (attachment && attachment.IsCurrentlyLoaded) loaded.Attachments.Insert(attachment);
		foreach (GGMagazineConfig magazine : m_Items.Magazines) if (magazine && magazine.IsCurrentlyLoaded) loaded.Magazines.Insert(magazine);
		foreach (GGAmmoConfig ammo : m_Items.Ammunition) if (ammo && ammo.IsCurrentlyLoaded) loaded.Ammunition.Insert(ammo);
		foreach (GGArmorConfig armor : m_Items.Armor) if (armor && armor.IsCurrentlyLoaded) loaded.Armor.Insert(armor);
		return loaded;
	}

	protected array<ref GGBlockedAttachmentRule> CreateBlockedAttachmentSnapshot()
	{
		ref array<ref GGBlockedAttachmentRule> blocked = new array<ref GGBlockedAttachmentRule>;
		if (!m_WeaponAttachments || !m_WeaponAttachments.Weapons) return blocked;
		foreach (GGWeaponAttachmentPolicy policy : m_WeaponAttachments.Weapons)
		{
			if (!policy || !policy.IsCurrentlyLoaded || !policy.AttachmentOverrides) continue;
			foreach (string attachmentClassName, int allowed : policy.AttachmentOverrides)
			{
				if (attachmentClassName == "" || allowed != 0) continue;
				blocked.Insert(new GGBlockedAttachmentRule(policy.WeaponClassName, attachmentClassName));
			}
		}
		return blocked;
	}

	protected void ValidateItems()
	{
		foreach (GGWeaponConfig weapon : m_Items.Weapons)
		{
			if (!weapon) continue;
			if (!weapon.FireModes) weapon.FireModes = new array<ref GGFireModeConfig>;
			CompactFireModes(weapon);
			weapon.RecoilMultiplier = GGUtil.Clamp(weapon.RecoilMultiplier, 0.01, 5.0);
			weapon.SwayMultiplier = GGUtil.Clamp(weapon.SwayMultiplier, 0.01, 5.0);
			weapon.ADSMultiplier = GGUtil.Clamp(weapon.ADSMultiplier, 0.01, 5.0);
			weapon.PrecisionMultiplier = GGUtil.Clamp(weapon.PrecisionMultiplier, 0.01, 5.0);
			weapon.HipFireMultiplier = GGUtil.Clamp(weapon.HipFireMultiplier, 0.01, 5.0);
			foreach (GGFireModeConfig mode : weapon.FireModes)
			{
				if (!mode) continue;
				mode.RecoilMultiplier = GGUtil.Clamp(mode.RecoilMultiplier, 0.01, 5.0);
				mode.SwayMultiplier = GGUtil.Clamp(mode.SwayMultiplier, 0.01, 5.0);
				mode.ADSMultiplier = GGUtil.Clamp(mode.ADSMultiplier, 0.01, 5.0);
				mode.PrecisionMultiplier = GGUtil.Clamp(mode.PrecisionMultiplier, 0.01, 5.0);
				mode.HipFireMultiplier = GGUtil.Clamp(mode.HipFireMultiplier, 0.01, 5.0);
			}
		}
		foreach (GGAttachmentConfig attachment : m_Items.Attachments)
		{
			if (!attachment) continue;
			if (!attachment.DetectedSlots) attachment.DetectedSlots = new array<string>;
			if (attachment.CustomTierLabel == "") attachment.CustomTierLabel = "Custom";
			if (!attachment.UseCustomStats)
			{
				GGTierDefinition attachmentTier = FindTierDefinition(attachment.TierKey);
				if (attachmentTier)
				{
					attachment.Category = attachmentTier.Category;
					attachment.Recoil = attachmentTier.Recoil;
					attachment.Sway = attachmentTier.Sway;
					attachment.ADS = attachmentTier.ADS;
					attachment.Precision = attachmentTier.Precision;
					attachment.HipFire = attachmentTier.HipFire;
				}
			}
			attachment.Recoil = GGUtil.Clamp(attachment.Recoil, 0.01, 5.0);
			attachment.Sway = GGUtil.Clamp(attachment.Sway, 0.01, 5.0);
			attachment.ADS = GGUtil.Clamp(attachment.ADS, 0.01, 5.0);
			attachment.Precision = GGUtil.Clamp(attachment.Precision, 0.01, 5.0);
			attachment.HipFire = GGUtil.Clamp(attachment.HipFire, 0.01, 5.0);
		}
		foreach (GGMagazineConfig magazine : m_Items.Magazines)
		{
			if (!magazine) continue;
			if (magazine.CustomTierLabel == "") magazine.CustomTierLabel = "Custom";
			if (!magazine.UseCustomStats)
			{
				GGTierDefinition magazineTier = FindTierDefinition(magazine.TierKey);
				if (magazineTier)
				{
					magazine.Recoil = magazineTier.Recoil;
					magazine.Sway = magazineTier.Sway;
					magazine.ADS = magazineTier.ADS;
					magazine.Precision = magazineTier.Precision;
					magazine.HipFire = magazineTier.HipFire;
				}
			}
			magazine.Recoil = GGUtil.Clamp(magazine.Recoil, 0.01, 5.0);
			magazine.Sway = GGUtil.Clamp(magazine.Sway, 0.01, 5.0);
			magazine.ADS = GGUtil.Clamp(magazine.ADS, 0.01, 5.0);
			magazine.Precision = GGUtil.Clamp(magazine.Precision, 0.01, 5.0);
			magazine.HipFire = GGUtil.Clamp(magazine.HipFire, 0.01, 5.0);
		}
	}

	protected void CompactItems()
	{
		ref map<string, bool> known = new map<string, bool>;
		ref array<ref GGWeaponConfig> weapons = new array<ref GGWeaponConfig>;
		foreach (GGWeaponConfig weapon : m_Items.Weapons)
		{
			if (!weapon || weapon.ClassName == "") continue;
			string weaponKey = GGUtil.Key(weapon.ClassName);
			bool weaponIgnored;
			if (known.Find(weaponKey, weaponIgnored)) continue;
			known.Set(weaponKey, true);
			weapons.Insert(weapon);
		}
		m_Items.Weapons = weapons;

		known.Clear();
		ref array<ref GGAttachmentConfig> attachments = new array<ref GGAttachmentConfig>;
		foreach (GGAttachmentConfig attachment : m_Items.Attachments)
		{
			if (!attachment || attachment.ClassName == "") continue;
			string attachmentKey = GGUtil.Key(attachment.ClassName);
			bool attachmentIgnored;
			if (known.Find(attachmentKey, attachmentIgnored)) continue;
			known.Set(attachmentKey, true);
			attachments.Insert(attachment);
		}
		m_Items.Attachments = attachments;

		known.Clear();
		ref array<ref GGMagazineConfig> magazines = new array<ref GGMagazineConfig>;
		foreach (GGMagazineConfig magazine : m_Items.Magazines)
		{
			if (!magazine || magazine.ClassName == "") continue;
			string magazineKey = GGUtil.Key(magazine.ClassName);
			bool magazineIgnored;
			if (known.Find(magazineKey, magazineIgnored)) continue;
			known.Set(magazineKey, true);
			magazines.Insert(magazine);
		}
		m_Items.Magazines = magazines;

		known.Clear();
		ref array<ref GGAmmoConfig> ammunition = new array<ref GGAmmoConfig>;
		foreach (GGAmmoConfig ammo : m_Items.Ammunition)
		{
			if (!ammo || ammo.ClassName == "") continue;
			string ammoKey = GGUtil.Key(ammo.ClassName);
			bool ammoIgnored;
			if (known.Find(ammoKey, ammoIgnored)) continue;
			known.Set(ammoKey, true);
			ammunition.Insert(ammo);
		}
		m_Items.Ammunition = ammunition;

		known.Clear();
		ref array<ref GGArmorConfig> armorItems = new array<ref GGArmorConfig>;
		foreach (GGArmorConfig armor : m_Items.Armor)
		{
			if (!armor || armor.ClassName == "") continue;
			string armorKey = GGUtil.Key(armor.ClassName);
			bool armorIgnored;
			if (known.Find(armorKey, armorIgnored)) continue;
			known.Set(armorKey, true);
			armorItems.Insert(armor);
		}
		m_Items.Armor = armorItems;
	}

	protected void CompactWeaponAttachmentPolicies()
	{
		m_WeaponAttachments.EnsureArrays();
		ref array<ref GGAttachmentSlotCatalog> catalog = new array<ref GGAttachmentSlotCatalog>;
		ref map<string, ref GGAttachmentSlotCatalog> knownCatalog = new map<string, ref GGAttachmentSlotCatalog>;
		foreach (GGAttachmentSlotCatalog slot : m_WeaponAttachments.SlotCatalog)
		{
			if (!slot || slot.SlotName == "") continue;
			string slotKey = GGUtil.Key(slot.SlotName);
			GGAttachmentSlotCatalog existingSlot;
			if (!knownCatalog.Find(slotKey, existingSlot))
			{
				existingSlot = new GGAttachmentSlotCatalog();
				existingSlot.SlotName = slot.SlotName;
				knownCatalog.Set(slotKey, existingSlot);
				catalog.Insert(existingSlot);
			}
			if (!slot.Attachments) continue;
			foreach (string catalogAttachment : slot.Attachments)
			{
				if (catalogAttachment == "") continue;
				if (existingSlot.Attachments.Find(catalogAttachment) == -1)
					existingSlot.Attachments.Insert(catalogAttachment);
			}
		}
		foreach (GGAttachmentSlotCatalog compactSlot : catalog)
		{
			compactSlot.Attachments.Sort();
		}
		m_WeaponAttachments.SlotCatalog = catalog;

		ref array<ref GGWeaponAttachmentPolicy> policies = new array<ref GGWeaponAttachmentPolicy>;
		ref map<string, bool> knownPolicies = new map<string, bool>;
		foreach (GGWeaponAttachmentPolicy policy : m_WeaponAttachments.Weapons)
		{
			if (!policy || policy.WeaponClassName == "") continue;
			string policyKey = GGUtil.Key(policy.WeaponClassName);
			bool policyIgnored;
			if (knownPolicies.Find(policyKey, policyIgnored)) continue;
			knownPolicies.Set(policyKey, true);
			if (!policy.Slots) policy.Slots = new array<string>;
			if (!policy.AttachmentOverrides) policy.AttachmentOverrides = new map<string, int>;

			ref array<string> slots = new array<string>;
			ref map<string, bool> knownSlots = new map<string, bool>;
			foreach (string policySlot : policy.Slots)
			{
				if (policySlot == "") continue;
				string policySlotKey = GGUtil.Key(policySlot);
				bool slotIgnored;
				if (knownSlots.Find(policySlotKey, slotIgnored)) continue;
				knownSlots.Set(policySlotKey, true);
				slots.Insert(policySlot);
			}
			slots.Sort();
			policy.Slots = slots;

			ref map<string, int> overrides = new map<string, int>;
			ref map<string, bool> knownOverrides = new map<string, bool>;
			foreach (string overrideClassName, int overrideAllowed : policy.AttachmentOverrides)
			{
				if (overrideClassName == "" || overrideAllowed != 0) continue;
				string overrideKey = GGUtil.Key(overrideClassName);
				bool overrideIgnored;
				if (knownOverrides.Find(overrideKey, overrideIgnored)) continue;
				knownOverrides.Set(overrideKey, true);
				overrides.Set(overrideClassName, 0);
			}
			policy.AttachmentOverrides = overrides;
			policies.Insert(policy);
		}
		m_WeaponAttachments.Weapons = policies;
	}

	protected void CompactFireModes(GGWeaponConfig weapon)
	{
		ref array<ref GGFireModeConfig> modes = new array<ref GGFireModeConfig>;
		ref map<string, bool> known = new map<string, bool>;
		foreach (GGFireModeConfig mode : weapon.FireModes)
		{
			if (!mode || mode.ModeName == "") continue;
			string key = GGUtil.Key(mode.ModeName);
			bool ignored;
			if (known.Find(key, ignored)) continue;
			known.Set(key, true);
			modes.Insert(mode);
		}
		weapon.FireModes = modes;
	}

	protected GGTierDefinition FindTierDefinition(string tierKey)
	{
		if (!m_Settings || !m_Settings.TierDefinitions) return null;
		foreach (GGTierDefinition tier : m_Settings.TierDefinitions)
		{
			if (tier && GGUtil.Key(tier.TierKey) == GGUtil.Key(tierKey)) return tier;
		}
		return null;
	}

	protected void WarnAboutOldGunplayMods()
	{
		ref array<string> oldPatches = new array<string>;
		oldPatches.Insert("SNAFU_Gunplay");
		oldPatches.Insert("AJ_Gunplay");
		oldPatches.Insert("Mortys_Gunplay");
		oldPatches.Insert("DeltaForce_Gunplay");
		oldPatches.Insert("Buca_Gunplay");
		foreach (string patchName : oldPatches)
		{
			if (g_Game.ConfigIsExisting("CfgPatches " + patchName))
				GGUtil.Error("Legacy mod " + patchName + " is loaded together with Generic_Gunplay. Remove all five legacy Gunplay mods; their global hooks cannot be safely stacked.");
		}
	}
}

ref GGConfigManager g_GGConfigManager;

GGConfigManager GetGGConfigManager()
{
	if (!g_GGConfigManager) g_GGConfigManager = new GGConfigManager();
	return g_GGConfigManager;
}
