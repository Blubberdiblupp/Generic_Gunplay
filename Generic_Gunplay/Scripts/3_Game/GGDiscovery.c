class GGDiscovery
{
	protected ref map<string, ref GGWeaponConfig> m_Weapons;
	protected ref map<string, ref GGAttachmentConfig> m_Attachments;
	protected ref map<string, ref GGMagazineConfig> m_Magazines;
	protected ref map<string, ref GGAmmoConfig> m_Ammo;
	protected ref map<string, ref GGArmorConfig> m_Armor;
	protected ref map<string, ref GGWeaponAttachmentPolicy> m_WeaponPolicies;
	protected ref map<string, ref TStringArray> m_AttachmentsBySlot;
	protected ref map<string, string> m_AttachmentSlotNames;
	protected ref map<string, bool> m_WeaponSlots;
	protected ref map<string, bool> m_ReferencedAmmo;
	protected GGItemsConfig m_Items;
	protected GGWeaponAttachmentsFile m_WeaponAttachments;
	protected GGSettings m_Settings;
	protected int m_AddedWeapons;
	protected int m_AddedAttachments;
	protected int m_AddedMagazines;
	protected int m_AddedAmmo;
	protected int m_AddedArmor;
	protected int m_AddedWeaponSlots;
	protected int m_RemovedInvalidArmor;

	void GGDiscovery()
	{
		m_Weapons = new map<string, ref GGWeaponConfig>;
		m_Attachments = new map<string, ref GGAttachmentConfig>;
		m_Magazines = new map<string, ref GGMagazineConfig>;
		m_Ammo = new map<string, ref GGAmmoConfig>;
		m_Armor = new map<string, ref GGArmorConfig>;
		m_WeaponPolicies = new map<string, ref GGWeaponAttachmentPolicy>;
		m_AttachmentsBySlot = new map<string, ref TStringArray>;
		m_AttachmentSlotNames = new map<string, string>;
		m_WeaponSlots = new map<string, bool>;
		m_ReferencedAmmo = new map<string, bool>;
	}

	bool ScanAndMerge(GGItemsConfig items, GGSettings settings, GGWeaponAttachmentsFile weaponAttachments)
	{
		if (!g_Game || !items || !settings || !weaponAttachments) return false;
		int debugStarted = GGDebug.BeginTiming(9);
		m_Items = items;
		m_Settings = settings;
		m_WeaponAttachments = weaponAttachments;
		items.EnsureArrays();
		weaponAttachments.EnsureArrays();
		BuildExistingMaps();
		MarkAllNotLoaded();

		ScanWeapons();
		ScanMagazines();
		ScanVehicleItems();
		RemoveInvalidArmorEntries();
		ScanWeaponAttachmentPolicies();
		ScanAmmo();
		if (!settings.PreserveMissingItems)
		{
			RemoveMissingItems();
			RemoveMissingWeaponAttachmentPolicies();
		}

		int newEntries = m_AddedWeapons;
		newEntries += m_AddedAttachments;
		newEntries += m_AddedMagazines;
		newEntries += m_AddedAmmo;
		newEntries += m_AddedArmor;
		newEntries += m_AddedWeaponSlots;
		string summary = "Discovery complete. Loaded weapons=" + CountLoadedWeapons().ToString();
		summary += ", attachments=" + CountLoadedAttachments().ToString();
		summary += ", weapon/attachment pairs=" + CountLoadedAttachmentCombinations().ToString();
		summary += ", magazines/ammo piles=" + CountLoadedMagazines().ToString();
		summary += ", projectiles=" + CountLoadedAmmo().ToString();
		summary += ", armor/clothing=" + CountLoadedArmor().ToString();
		summary += ". New entries=" + newEntries.ToString() + ".";
		if (m_RemovedInvalidArmor > 0)
			summary += " Removed invalid armor/clothing entries=" + m_RemovedInvalidArmor.ToString() + ".";
		GGUtil.Log(summary);
		string addedDetails = "weapons=" + m_AddedWeapons.ToString();
		addedDetails += " attachments=" + m_AddedAttachments.ToString();
		addedDetails += " magazines=" + m_AddedMagazines.ToString();
		addedDetails += " projectiles=" + m_AddedAmmo.ToString();
		addedDetails += " armor=" + m_AddedArmor.ToString();
		addedDetails += " weaponSlots=" + m_AddedWeaponSlots.ToString();
		addedDetails += " removedInvalidArmor=" + m_RemovedInvalidArmor.ToString();
		GGDebug.Log(4, "DISCOVERY", "Discovery changes: " + addedDetails);
		GGDebug.EndTiming(9, "PERFORMANCE", "Config discovery", debugStarted, addedDetails);
		return newEntries > 0 || m_RemovedInvalidArmor > 0;
	}

	protected void BuildExistingMaps()
	{
		m_Weapons.Clear();
		m_Attachments.Clear();
		m_Magazines.Clear();
		m_Ammo.Clear();
		m_Armor.Clear();
		m_WeaponPolicies.Clear();

		foreach (GGWeaponConfig weapon : m_Items.Weapons)
		{
			if (weapon && weapon.ClassName != "") m_Weapons.Set(GGUtil.Key(weapon.ClassName), weapon);
		}
		foreach (GGAttachmentConfig attachment : m_Items.Attachments)
		{
			if (attachment && attachment.ClassName != "") m_Attachments.Set(GGUtil.Key(attachment.ClassName), attachment);
		}
		foreach (GGMagazineConfig magazine : m_Items.Magazines)
		{
			if (magazine && magazine.ClassName != "") m_Magazines.Set(GGUtil.Key(magazine.ClassName), magazine);
		}
		foreach (GGAmmoConfig ammo : m_Items.Ammunition)
		{
			if (ammo && ammo.ClassName != "") m_Ammo.Set(GGUtil.Key(ammo.ClassName), ammo);
		}
		foreach (GGArmorConfig armor : m_Items.Armor)
		{
			if (armor && armor.ClassName != "") m_Armor.Set(GGUtil.Key(armor.ClassName), armor);
		}
		foreach (GGWeaponAttachmentPolicy policy : m_WeaponAttachments.Weapons)
		{
			if (policy && policy.WeaponClassName != "")
				m_WeaponPolicies.Set(GGUtil.Key(policy.WeaponClassName), policy);
		}
	}

	protected void MarkAllNotLoaded()
	{
		foreach (GGWeaponConfig weapon : m_Items.Weapons) if (weapon) weapon.IsCurrentlyLoaded = false;
		foreach (GGAttachmentConfig attachment : m_Items.Attachments) if (attachment) attachment.IsCurrentlyLoaded = false;
		foreach (GGMagazineConfig magazine : m_Items.Magazines) if (magazine) magazine.IsCurrentlyLoaded = false;
		foreach (GGAmmoConfig ammo : m_Items.Ammunition) if (ammo) ammo.IsCurrentlyLoaded = false;
		foreach (GGArmorConfig armor : m_Items.Armor) if (armor) armor.IsCurrentlyLoaded = false;
		foreach (GGWeaponAttachmentPolicy policy : m_WeaponAttachments.Weapons)
		{
			if (!policy) continue;
			policy.IsCurrentlyLoaded = false;
			if (!policy.Slots) policy.Slots = new array<string>;
			if (!policy.AttachmentOverrides) policy.AttachmentOverrides = new map<string, int>;
		}
	}

	protected void ScanWeapons()
	{
		int count = g_Game.ConfigGetChildrenCount("CfgWeapons");
		for (int i = 0; i < count; i++)
		{
			string className;
			if (!g_Game.ConfigGetChildName("CfgWeapons", i, className)) continue;
			if (!IsConcreteWeapon(className)) continue;

			CollectWeaponSlots(className);
			CollectWeaponAmmo(className);
			GGWeaponConfig weapon;
			if (!m_Weapons.Find(GGUtil.Key(className), weapon))
			{
				weapon = CreateWeapon(className);
				m_Items.Weapons.Insert(weapon);
				m_Weapons.Set(GGUtil.Key(className), weapon);
				m_AddedWeapons++;
				if (GGDebug.Enabled(10))
					GGDebug.Once(10, "DISCOVERY", "weapon_" + GGUtil.Key(className), "Added weapon " + className + " parent=" + weapon.ParentClass);
			}
			else
			{
				RefreshWeaponDetectedValues(weapon);
			}
			weapon.IsCurrentlyLoaded = true;
		}
	}

	protected bool IsConcreteWeapon(string className)
	{
		string path = "CfgWeapons " + className;
		if (!g_Game.ConfigIsExisting(path)) return false;
		if (g_Game.ConfigGetInt(path + " scope") != 2) return false;
		return InheritsFrom("CfgWeapons", className, "Weapon_Base");
	}

	protected GGWeaponConfig CreateWeapon(string className)
	{
		GGWeaponConfig weapon = new GGWeaponConfig();
		weapon.ClassName = className;
		g_Game.ConfigGetBaseName("CfgWeapons " + className, weapon.ParentClass);
		RefreshWeaponDetectedValues(weapon);
		return weapon;
	}

	protected void RefreshWeaponDetectedValues(GGWeaponConfig weapon)
	{
		if (!weapon) return;
		string className = weapon.ClassName;
		string path = "CfgWeapons " + className;
		g_Game.ConfigGetBaseName(path, weapon.ParentClass);

		float weight = g_Game.ConfigGetFloat(path + " weight");
		if (weight <= 0.0) weight = 3000.0;
		weapon.DetectedWeightKg = weight / 1000.0;
		weapon.DetectedLengthM = g_Game.ConfigGetFloat(path + " WeaponLength");
		if (weapon.DetectedLengthM <= 0.0) weapon.DetectedLengthM = 0.7;

		float configRecoil = GetAverageFloatArray(path + " recoilModifier", 1.0);
		float configSway = GetAverageFloatArray(path + " swayModifier", 1.0);
		float configADS = GetAverageFloatArray(path + " aimSpeedModifier", 0.35);
		float weightReduction = GGUtil.Clamp((weapon.DetectedWeightKg - 3.0) * 0.025, -0.06, 0.12);
		float weightSway = GGUtil.Clamp((weapon.DetectedWeightKg - 3.0) * 0.025, -0.05, 0.16);
		float lengthSway = GGUtil.Clamp((weapon.DetectedLengthM - 0.7) * 0.18, -0.06, 0.14);
		float weightADS = GGUtil.Clamp((weapon.DetectedWeightKg - 3.0) * 0.018, -0.05, 0.14);
		float lengthADS = GGUtil.Clamp((weapon.DetectedLengthM - 0.7) * 0.14, -0.05, 0.12);
		float lengthPrecision = GGUtil.Clamp((weapon.DetectedLengthM - 0.55) * 0.20, -0.08, 0.18);
		float weightPrecision = GGUtil.Clamp((weapon.DetectedWeightKg - 3.0) * 0.010, -0.04, 0.08);

		weapon.DetectedRecoil = GGUtil.Clamp(configRecoil * (1.0 - weightReduction), 0.65, 1.85);
		weapon.DetectedSway = GGUtil.Clamp(configSway * (1.0 + weightSway + lengthSway), 0.70, 1.95);
		weapon.DetectedADSSpeed = GGUtil.Clamp((0.35 / GGUtil.Clamp(configADS, 0.12, 1.20)) * (1.0 - weightADS - lengthADS), 0.65, 1.35);
		weapon.DetectedPrecision = GGUtil.Clamp(1.0 + lengthPrecision + weightPrecision, 0.85, 1.25);
		weapon.DetectedInitSpeedMultiplier = g_Game.ConfigGetFloat(path + " initSpeedMultiplier");
		if (weapon.DetectedInitSpeedMultiplier <= 0.0) weapon.DetectedInitSpeedMultiplier = 1.0;
		RefreshFireModes(weapon);
	}

	protected void RefreshFireModes(GGWeaponConfig weapon)
	{
		if (!weapon.FireModes) weapon.FireModes = new array<ref GGFireModeConfig>;
		ref map<string, ref GGFireModeConfig> existing = new map<string, ref GGFireModeConfig>;
		foreach (GGFireModeConfig oldMode : weapon.FireModes)
		{
			if (oldMode && oldMode.ModeName != "") existing.Set(GGUtil.Key(oldMode.ModeName), oldMode);
		}

		TStringArray modes = new TStringArray;
		g_Game.ConfigGetTextArray("CfgWeapons " + weapon.ClassName + " modes", modes);
		AddKnownModeIfPresent(weapon.ClassName, modes, "SemiAuto");
		AddKnownModeIfPresent(weapon.ClassName, modes, "FullAuto");
		AddKnownModeIfPresent(weapon.ClassName, modes, "Burst");
		AddKnownModeIfPresent(weapon.ClassName, modes, "Single");
		AddKnownModeIfPresent(weapon.ClassName, modes, "Double");

		ref array<ref GGFireModeConfig> refreshed = new array<ref GGFireModeConfig>;
		ref map<string, bool> used = new map<string, bool>;
		foreach (string modeName : modes)
		{
			string key = GGUtil.Key(modeName);
			bool ignored;
			if (used.Find(key, ignored)) continue;
			used.Set(key, true);

			GGFireModeConfig mode;
			if (!existing.Find(key, mode)) mode = new GGFireModeConfig();
			mode.ModeName = modeName;
			string modePath = "CfgWeapons " + weapon.ClassName + " " + modeName;
			mode.DetectedReloadTime = g_Game.ConfigGetFloat(modePath + " reloadTime");
			mode.DetectedDispersion = g_Game.ConfigGetFloat(modePath + " dispersion");
			refreshed.Insert(mode);
		}
		weapon.FireModes = refreshed;
	}

	protected void AddKnownModeIfPresent(string className, TStringArray modes, string candidate)
	{
		string path = "CfgWeapons " + className + " " + candidate;
		if (!g_Game.ConfigIsExisting(path)) return;
		foreach (string existing : modes) if (existing == candidate) return;
		modes.Insert(candidate);
	}

	protected void CollectWeaponSlots(string className)
	{
		TStringArray slots = new TStringArray;
		g_Game.ConfigGetTextArray("CfgWeapons " + className + " attachments", slots);
		foreach (string slot : slots)
		{
			if (slot != "") m_WeaponSlots.Set(GGUtil.Key(slot), true);
		}
	}

	protected void CollectWeaponAmmo(string className)
	{
		TStringArray chamberable = new TStringArray;
		g_Game.ConfigGetTextArray("CfgWeapons " + className + " chamberableFrom", chamberable);
		foreach (string chamberType : chamberable)
		{
			if (chamberType == "") continue;
			string ammoClass;
			if (g_Game.ConfigIsExisting("CfgAmmo " + chamberType)) ammoClass = chamberType;
			if (ammoClass == "" && g_Game.ConfigIsExisting("CfgMagazines " + chamberType))
				g_Game.ConfigGetText("CfgMagazines " + chamberType + " ammo", ammoClass);
			if (ammoClass == "") AmmoTypesAPI.GetAmmoType(chamberType, ammoClass);
			if (ammoClass != "") m_ReferencedAmmo.Set(GGUtil.Key(ammoClass), true);
		}
	}

	protected void ScanMagazines()
	{
		int count = g_Game.ConfigGetChildrenCount("CfgMagazines");
		for (int i = 0; i < count; i++)
		{
			string className;
			if (!g_Game.ConfigGetChildName("CfgMagazines", i, className)) continue;
			string path = "CfgMagazines " + className;
			if (g_Game.ConfigGetInt(path + " scope") != 2) continue;
			int capacity = g_Game.ConfigGetInt(path + " count");
			if (capacity <= 0) continue;

			string ammoClass;
			g_Game.ConfigGetText(path + " ammo", ammoClass);
			if (ammoClass == "") AmmoTypesAPI.GetAmmoType(className, ammoClass);
			if (ammoClass != "") m_ReferencedAmmo.Set(GGUtil.Key(ammoClass), true);

			GGMagazineConfig magazine;
			if (!m_Magazines.Find(GGUtil.Key(className), magazine))
			{
				magazine = new GGMagazineConfig();
				magazine.ClassName = className;
				g_Game.ConfigGetBaseName(path, magazine.ParentClass);
				magazine.IsLooseAmmo = InheritsFrom("CfgMagazines", className, "Ammunition_Base");
				string presetTier = FindMagazinePresetInParentChain(className);
				if (presetTier != "") magazine.TierKey = presetTier;
				else if (!magazine.IsLooseAmmo && capacity > m_Settings.HighCapMagazineThreshold) magazine.TierKey = "HighCap_Heavy";
				ApplyTierSnapshotToMagazine(magazine);
				m_Items.Magazines.Insert(magazine);
				m_Magazines.Set(GGUtil.Key(className), magazine);
				m_AddedMagazines++;
				if (GGDebug.Enabled(10))
				{
					string magazineDebug = "Added magazine/ammo pile " + className;
					magazineDebug += " capacity=" + capacity.ToString();
					magazineDebug += " tier=" + magazine.TierKey;
					GGDebug.Once(10, "DISCOVERY", "magazine_" + GGUtil.Key(className), magazineDebug);
				}
			}
			g_Game.ConfigGetBaseName(path, magazine.ParentClass);
			magazine.IsLooseAmmo = InheritsFrom("CfgMagazines", className, "Ammunition_Base");
			magazine.DetectedCapacity = capacity;
			magazine.AmmoClass = ammoClass;
			magazine.IsCurrentlyLoaded = true;
		}
	}

	protected string FindMagazinePresetInParentChain(string className)
	{
		string current = className;
		for (int i = 0; i < 64 && current != ""; i++)
		{
			string tierKey = GGLegacyPresets.GetTier(current);
			if (tierKey != "") return tierKey;
			string parent;
			if (!g_Game.ConfigGetBaseName("CfgMagazines " + current, parent) || parent == current) break;
			current = parent;
		}
		return "";
	}

	protected void ScanVehicleItems()
	{
		ExpandWeaponAttachmentSlots();
		int count = g_Game.ConfigGetChildrenCount("CfgVehicles");
		for (int i = 0; i < count; i++)
		{
			string className;
			if (!g_Game.ConfigGetChildName("CfgVehicles", i, className)) continue;
			string path = "CfgVehicles " + className;
			if (g_Game.ConfigGetInt(path + " scope") != 2) continue;

			TStringArray slots = GetInventorySlots(path);
			if (IsWeaponAttachment(slots)) MergeAttachment(className, slots);
			if (IsWearableArmorItem(className, slots)) MergeArmor(className);
		}
	}

	protected void ExpandWeaponAttachmentSlots()
	{
		int count = g_Game.ConfigGetChildrenCount("CfgVehicles");
		for (int passIndex = 0; passIndex < 32; passIndex++)
		{
			bool changed = false;
			for (int i = 0; i < count; i++)
			{
				string className;
				if (!g_Game.ConfigGetChildName("CfgVehicles", i, className)) continue;
				string path = "CfgVehicles " + className;
				if (g_Game.ConfigGetInt(path + " scope") != 2) continue;

				TStringArray inventorySlots = GetInventorySlots(path);
				if (!IsWeaponAttachment(inventorySlots)) continue;

				TStringArray childSlots = new TStringArray;
				g_Game.ConfigGetTextArray(path + " attachments", childSlots);
				foreach (string childSlot : childSlots)
				{
					if (childSlot == "") continue;
					string key = GGUtil.Key(childSlot);
					bool ignored;
					if (m_WeaponSlots.Find(key, ignored)) continue;
					m_WeaponSlots.Set(key, true);
					changed = true;
				}
			}
			if (!changed) return;
		}
		GGUtil.Warning("Weapon attachment slot discovery reached its safety limit; check for an unusually deep attachment chain.");
	}

	protected TStringArray GetInventorySlots(string path)
	{
		TStringArray slots = new TStringArray;
		g_Game.ConfigGetTextArray(path + " inventorySlot", slots);
		string singleSlot;
		g_Game.ConfigGetText(path + " inventorySlot", singleSlot);
		if (singleSlot != "")
		{
			bool found = false;
			foreach (string slot : slots) if (slot == singleSlot) found = true;
			if (!found) slots.Insert(singleSlot);
		}
		return slots;
	}

	protected bool IsWeaponAttachment(TStringArray slots)
	{
		foreach (string slot : slots)
		{
			bool ignored;
			if (m_WeaponSlots.Find(GGUtil.Key(slot), ignored)) return true;
		}
		return false;
	}

	protected void MergeAttachment(string className, TStringArray slots)
	{
		GGAttachmentConfig attachment;
		if (!m_Attachments.Find(GGUtil.Key(className), attachment))
		{
			attachment = new GGAttachmentConfig();
			attachment.ClassName = className;
			g_Game.ConfigGetBaseName("CfgVehicles " + className, attachment.ParentClass);
			foreach (string slot : slots) attachment.DetectedSlots.Insert(slot);
			ClassifyAttachment(attachment);
			ApplyTierSnapshotToAttachment(attachment);
			m_Items.Attachments.Insert(attachment);
			m_Attachments.Set(GGUtil.Key(className), attachment);
			m_AddedAttachments++;
			if (GGDebug.Enabled(10))
			{
				string attachmentDebug = "Added attachment " + className;
				attachmentDebug += " category=" + attachment.Category;
				attachmentDebug += " tier=" + attachment.TierKey;
				GGDebug.Once(10, "DISCOVERY", "attachment_" + GGUtil.Key(className), attachmentDebug);
			}
		}
		else
		{
			if (!attachment.DetectedSlots) attachment.DetectedSlots = new array<string>;
			attachment.DetectedSlots.Clear();
			foreach (string refreshedSlot : slots) attachment.DetectedSlots.Insert(refreshedSlot);
			g_Game.ConfigGetBaseName("CfgVehicles " + className, attachment.ParentClass);
		}
		attachment.IsCurrentlyLoaded = true;
	}

	protected void ScanWeaponAttachmentPolicies()
	{
		BuildAttachmentSlotIndex();
		RefreshAttachmentSlotCatalog();
		foreach (GGWeaponConfig weapon : m_Items.Weapons)
		{
			if (!weapon || !weapon.IsCurrentlyLoaded) continue;
			RefreshWeaponAttachmentPolicy(weapon);
		}
	}

	protected void BuildAttachmentSlotIndex()
	{
		m_AttachmentsBySlot.Clear();
		m_AttachmentSlotNames.Clear();
		foreach (GGAttachmentConfig attachment : m_Items.Attachments)
		{
			if (!attachment || !attachment.IsCurrentlyLoaded || !attachment.DetectedSlots) continue;
			foreach (string slotName : attachment.DetectedSlots)
			{
				if (slotName == "") continue;
				string slotKey = GGUtil.Key(slotName);
				TStringArray classNames;
				if (!m_AttachmentsBySlot.Find(slotKey, classNames))
				{
					classNames = new TStringArray;
					m_AttachmentsBySlot.Set(slotKey, classNames);
					m_AttachmentSlotNames.Set(slotKey, slotName);
				}
				if (classNames.Find(attachment.ClassName) == -1)
					classNames.Insert(attachment.ClassName);
			}
		}
	}

	protected void RefreshAttachmentSlotCatalog()
	{
		m_WeaponAttachments.SlotCatalog.Clear();
		TStringArray slotKeys = new TStringArray;
		for (int slotIndex = 0; slotIndex < m_AttachmentsBySlot.Count(); slotIndex++)
		{
			slotKeys.Insert(m_AttachmentsBySlot.GetKey(slotIndex));
		}
		slotKeys.Sort();

		foreach (string sortedSlotKey : slotKeys)
		{
			TStringArray classNames;
			if (!m_AttachmentsBySlot.Find(sortedSlotKey, classNames)) continue;
			classNames.Sort();
			GGAttachmentSlotCatalog slot = new GGAttachmentSlotCatalog();
			string displaySlotName;
			if (!m_AttachmentSlotNames.Find(sortedSlotKey, displaySlotName))
				displaySlotName = sortedSlotKey;
			slot.SlotName = displaySlotName;
			foreach (string className : classNames)
			{
				slot.Attachments.Insert(className);
			}
			m_WeaponAttachments.SlotCatalog.Insert(slot);
		}
		m_WeaponAttachments.FormatVersion = GGConstants.WEAPON_ATTACHMENTS_FORMAT_VERSION;
	}

	protected void RefreshWeaponAttachmentPolicy(GGWeaponConfig weapon)
	{
		GGWeaponAttachmentPolicy policy;
		string weaponKey = GGUtil.Key(weapon.ClassName);
		if (!m_WeaponPolicies.Find(weaponKey, policy))
		{
			policy = new GGWeaponAttachmentPolicy();
			policy.WeaponClassName = weapon.ClassName;
			m_WeaponAttachments.Weapons.Insert(policy);
			m_WeaponPolicies.Set(weaponKey, policy);
		}
		if (!policy.Slots) policy.Slots = new array<string>;
		if (!policy.AttachmentOverrides) policy.AttachmentOverrides = new map<string, int>;
		policy.IsCurrentlyLoaded = true;

		ref map<string, bool> previousSlots = new map<string, bool>;
		foreach (string previousSlot : policy.Slots)
		{
			if (previousSlot != "") previousSlots.Set(GGUtil.Key(previousSlot), true);
		}

		TStringArray pendingSlots = new TStringArray;
		ref map<string, bool> knownSlots = new map<string, bool>;
		TStringArray activeSlots = new TStringArray;
		ref map<string, bool> knownActiveSlots = new map<string, bool>;
		TStringArray directSlots = new TStringArray;
		string weaponPath = "CfgWeapons " + weapon.ClassName;
		g_Game.ConfigGetTextArray(weaponPath + " attachments", directSlots);
		foreach (string directSlot : directSlots)
		{
			AddPendingAttachmentSlot(directSlot, pendingSlots, knownSlots);
		}

		ref map<string, bool> expandedAttachments = new map<string, bool>;
		int pendingIndex = 0;
		while (pendingIndex < pendingSlots.Count() && pendingIndex < 8192)
		{
			string slotName = pendingSlots.Get(pendingIndex);
			pendingIndex++;
			TStringArray candidateClasses;
			if (!m_AttachmentsBySlot.Find(GGUtil.Key(slotName), candidateClasses)) continue;
			bool slotHasCompatibleAttachment = false;
			foreach (string candidateClass : candidateClasses)
			{
				string candidateKey = GGUtil.Key(candidateClass);
				GGAttachmentConfig attachment;
				if (!m_Attachments.Find(candidateKey, attachment)) continue;
				if (!attachment || !attachment.IsCurrentlyLoaded) continue;
				slotHasCompatibleAttachment = true;

				bool alreadyExpanded;
				if (expandedAttachments.Find(candidateKey, alreadyExpanded)) continue;
				expandedAttachments.Set(candidateKey, true);
				TStringArray childSlots = new TStringArray;
				string attachmentPath = "CfgVehicles " + candidateClass;
				g_Game.ConfigGetTextArray(attachmentPath + " attachments", childSlots);
				foreach (string childSlot : childSlots)
				{
					AddPendingAttachmentSlot(childSlot, pendingSlots, knownSlots);
				}
			}
			if (slotHasCompatibleAttachment)
			{
				string activeSlotKey = GGUtil.Key(slotName);
				bool activeSlotIgnored;
				if (!knownActiveSlots.Find(activeSlotKey, activeSlotIgnored))
				{
					knownActiveSlots.Set(activeSlotKey, true);
					activeSlots.Insert(slotName);
				}
			}
		}
		if (pendingIndex >= 8192)
			GGUtil.Warning("Attachment compatibility scan reached its slot safety limit for " + weapon.ClassName + ".");

		activeSlots.Sort();
		foreach (string activeSlot : activeSlots)
		{
			bool previousSlotIgnored;
			if (!previousSlots.Find(GGUtil.Key(activeSlot), previousSlotIgnored))
			{
				m_AddedWeaponSlots++;
				if (GGDebug.Enabled(10))
				{
					string slotDebugKey = "slot_" + GGUtil.Key(weapon.ClassName);
					slotDebugKey += "_" + GGUtil.Key(activeSlot);
					string slotDebug = "Added compatibility slot " + activeSlot;
					slotDebug += " for " + weapon.ClassName;
					GGDebug.Once(10, "DISCOVERY", slotDebugKey, slotDebug);
				}
			}
		}
		policy.Slots = activeSlots;
	}

	protected void AddPendingAttachmentSlot(string slotName, TStringArray pendingSlots, map<string, bool> knownSlots)
	{
		if (slotName == "") return;
		string slotKey = GGUtil.Key(slotName);
		bool ignored;
		if (knownSlots.Find(slotKey, ignored)) return;
		knownSlots.Set(slotKey, true);
		pendingSlots.Insert(slotName);
	}

	protected void ClassifyAttachment(GGAttachmentConfig attachment)
	{
		string tierKey = FindPresetInParentChain(attachment.ClassName);
		if (tierKey != "")
		{
			attachment.TierKey = tierKey;
			attachment.Category = CategoryFromTier(tierKey);
			attachment.NeedsReview = false;
			return;
		}

		string search = GGUtil.Key(attachment.ClassName);
		foreach (string slot : attachment.DetectedSlots) search = search + " " + GGUtil.Key(slot);
		attachment.NeedsReview = false;

		if (ContainsAny(search, "bipod", "weaponbipod", "slot_bipod")) { attachment.Category = "Bipod"; attachment.TierKey = "Bipod_Special"; return; }
		if (ContainsAny(search, "pistolgrip", "pistol grip", "weaponakpistolgrip")) { attachment.Category = "Pistol Grip"; attachment.TierKey = "PistolGrip_T1"; return; }
		bool isForegrip = ContainsAny(search, "foregrip", "fgrip", "verticalgrip");
		if (!isForegrip && search.Contains("grip") && !search.Contains("pistol")) isForegrip = true;
		if (isForegrip)
		{
			attachment.Category = "Foregrip";
			attachment.TierKey = "Foregrip_T1";
			return;
		}
		if (ContainsAny(search, "buttstock", "weaponstock", " stock", "stock_"))
		{
			attachment.Category = "Stock";
			attachment.TierKey = "Stock_T1";
			return;
		}
		if (ContainsAny(search, "handguard", "hndgrd", "forearm", "dustcover", "scope rail", "scoperail"))
		{
			attachment.Category = "Handguard";
			attachment.TierKey = "Handguard_T1";
			return;
		}
		if (ContainsAny(search, "suppressor", "silencer", "silencerimpro", "suppressorimpro"))
		{
			attachment.Category = "Suppressor";
			attachment.TierKey = "Suppressor_T1";
			return;
		}
		if (ContainsAny(search, "muzzle", "muzzel", "choke", "compensator", "flashhider", "flash_hider", "brake"))
		{
			attachment.Category = "Muzzle";
			attachment.TierKey = "Muzzle_T1";
			return;
		}
		if (ContainsAny(search, "laser", "anpeq", "peq", "ngal", "dbal", "mawl"))
		{
			attachment.Category = "Laser";
			attachment.TierKey = "Laser_Tactical";
			return;
		}
		if (ContainsAny(search, "flashlight", "weaponlight", "taclight", "light rail", "lightrail"))
		{
			attachment.Category = "Flashlight";
			attachment.TierKey = "Flashlight_Utility";
			return;
		}
		if (search.Contains("bayonet")) { attachment.Category = "Bayonet"; attachment.TierKey = "Bayonet_Heavy"; return; }
		if (search.Contains("wrap")) { attachment.Category = "Wrap"; attachment.TierKey = "WeaponWrap_Utility"; return; }
		if (ContainsAny(search, "optic", "scope", "sight", "weaponoptics", "pistoloptics", "reddot", "red dot", "holo"))
		{
			attachment.Category = "Optic";
			attachment.TierKey = DetectOpticTier(attachment.ClassName);
			return;
		}

		attachment.Category = "Neutral";
		attachment.TierKey = "Neutral";
		attachment.NeedsReview = true;
	}

	protected string FindPresetInParentChain(string className)
	{
		string current = className;
		for (int i = 0; i < 64 && current != ""; i++)
		{
			string tierKey = GGLegacyPresets.GetTier(current);
			if (tierKey != "") return tierKey;
			string parent;
			if (!g_Game.ConfigGetBaseName("CfgVehicles " + current, parent) || parent == current) break;
			current = parent;
		}
		return "";
	}

	protected string DetectOpticTier(string className)
	{
		string zoomPath = "CfgVehicles " + className + " OpticsInfo opticsZoomMin";
		float zoomMin = g_Game.ConfigGetFloat(zoomPath);
		if (zoomMin <= 0.0) return "LightOptic_T1";
		float magnification = 0.3926 / zoomMin;
		if (magnification >= 12.0) return "SniperOptic_T3";
		if (magnification > 3.0) return "HeavyOptic_T2";
		return "LightOptic_T1";
	}

	protected bool ContainsAny(string value, string a, string b = "", string c = "", string d = "", string e = "", string f = "", string g = "", string h = "")
	{
		if (a != "" && value.Contains(a)) return true;
		if (b != "" && value.Contains(b)) return true;
		if (c != "" && value.Contains(c)) return true;
		if (d != "" && value.Contains(d)) return true;
		if (e != "" && value.Contains(e)) return true;
		if (f != "" && value.Contains(f)) return true;
		if (g != "" && value.Contains(g)) return true;
		if (h != "" && value.Contains(h)) return true;
		return false;
	}

	protected string CategoryFromTier(string tierKey)
	{
		if (tierKey.IndexOf("Foregrip_") == 0) return "Foregrip";
		if (tierKey.IndexOf("PistolGrip_") == 0) return "Pistol Grip";
		if (tierKey.IndexOf("Stock_") == 0) return "Stock";
		if (tierKey.IndexOf("Handguard_") == 0) return "Handguard";
		if (tierKey.IndexOf("Bipod_") == 0) return "Bipod";
		if (tierKey.Contains("Optic_")) return "Optic";
		if (tierKey.IndexOf("Laser_") == 0) return "Laser";
		if (tierKey.IndexOf("Flashlight_") == 0) return "Flashlight";
		if (tierKey.IndexOf("Bayonet_") == 0) return "Bayonet";
		if (tierKey.IndexOf("Suppressor_") == 0) return "Suppressor";
		if (tierKey.IndexOf("Muzzle_") == 0) return "Muzzle";
		if (tierKey.IndexOf("WeaponWrap_") == 0) return "Wrap";
		if (tierKey.Contains("Mag_") || tierKey.IndexOf("HighCap_") == 0) return "Magazine";
		return "Neutral";
	}

	protected void ApplyTierSnapshotToAttachment(GGAttachmentConfig attachment)
	{
		GGTierDefinition tier = FindTierDefinition(attachment.TierKey);
		if (!tier) return;
		attachment.Recoil = tier.Recoil;
		attachment.Sway = tier.Sway;
		attachment.ADS = tier.ADS;
		attachment.Precision = tier.Precision;
		attachment.HipFire = tier.HipFire;
	}

	protected void ApplyTierSnapshotToMagazine(GGMagazineConfig magazine)
	{
		GGTierDefinition tier = FindTierDefinition(magazine.TierKey);
		if (!tier) return;
		magazine.Recoil = tier.Recoil;
		magazine.Sway = tier.Sway;
		magazine.ADS = tier.ADS;
		magazine.Precision = tier.Precision;
		magazine.HipFire = tier.HipFire;
	}

	protected GGTierDefinition FindTierDefinition(string tierKey)
	{
		foreach (GGTierDefinition tier : m_Settings.TierDefinitions)
		{
			if (tier && GGUtil.Key(tier.TierKey) == GGUtil.Key(tierKey)) return tier;
		}
		return null;
	}

	protected bool IsWearableArmorItem(string className, TStringArray slots)
	{
		string path = "CfgVehicles " + className;
		if (!g_Game.ConfigIsExisting(path)) return false;
		if (g_Game.ConfigGetInt(path + " scope") != 2) return false;
		if (InheritsFrom("CfgVehicles", className, "Clothing")) return true;
		if (!InheritsFrom("CfgVehicles", className, "Inventory_Base")) return false;
		return HasWearableSlot(slots);
	}

	protected bool HasWearableSlot(TStringArray slots)
	{
		foreach (string slot : slots)
		{
			string key = GGUtil.Key(slot);
			if (key == "headgear") return true;
			if (key == "mask") return true;
			if (key == "eyewear") return true;
			if (key == "gloves") return true;
			if (key == "body") return true;
			if (key == "vest") return true;
			if (key == "back") return true;
			if (key == "hips") return true;
			if (key == "legs") return true;
			if (key == "feet") return true;
			if (key == "armband") return true;
		}
		return false;
	}

	protected void RemoveInvalidArmorEntries()
	{
		for (int index = m_Items.Armor.Count() - 1; index >= 0; index--)
		{
			GGArmorConfig armor = m_Items.Armor[index];
			if (!armor || armor.ClassName == "")
			{
				m_Items.Armor.Remove(index);
				m_RemovedInvalidArmor++;
				continue;
			}

			string path = "CfgVehicles " + armor.ClassName;
			if (!g_Game.ConfigIsExisting(path)) continue;
			TStringArray slots = GetInventorySlots(path);
			if (IsWearableArmorItem(armor.ClassName, slots)) continue;

			m_Armor.Remove(GGUtil.Key(armor.ClassName));
			m_Items.Armor.Remove(index);
			m_RemovedInvalidArmor++;
		}
	}

	protected void MergeArmor(string className)
	{
		GGArmorConfig armor;
		if (!m_Armor.Find(GGUtil.Key(className), armor))
		{
			armor = new GGArmorConfig();
			armor.ClassName = className;
			g_Game.ConfigGetBaseName("CfgVehicles " + className, armor.ParentClass);
			m_Items.Armor.Insert(armor);
			m_Armor.Set(GGUtil.Key(className), armor);
			m_AddedArmor++;
			if (GGDebug.Enabled(10))
				GGDebug.Once(10, "DISCOVERY", "armor_" + GGUtil.Key(className), "Added armor/clothing " + className);
		}

		armor.DetectedProjectileReduction = GetArmorReduction(className, "Projectile");
		armor.DetectedMeleeReduction = GetArmorReduction(className, "Melee");
		armor.DetectedInfectedReduction = GetArmorReduction(className, "Infected");
		armor.DetectedFragReduction = GetArmorReduction(className, "FragGrenade");
		armor.IsCurrentlyLoaded = true;
	}

	protected float GetArmorReduction(string className, string damageClass)
	{
		string path = "CfgVehicles " + className + " DamageSystem GlobalArmor " + damageClass + " Health damage";
		if (!g_Game.ConfigIsExisting(path)) return 0.0;
		return GGUtil.Clamp((1.0 - g_Game.ConfigGetFloat(path)) * 100.0, 0.0, 100.0);
	}

	protected void ScanAmmo()
	{
		int count = g_Game.ConfigGetChildrenCount("CfgAmmo");
		for (int i = 0; i < count; i++)
		{
			string className;
			if (!g_Game.ConfigGetChildName("CfgAmmo", i, className)) continue;
			bool ignored;
			if (!m_ReferencedAmmo.Find(GGUtil.Key(className), ignored)) continue;

			GGAmmoConfig ammo;
			if (!m_Ammo.Find(GGUtil.Key(className), ammo))
			{
				ammo = new GGAmmoConfig();
				ammo.ClassName = className;
				g_Game.ConfigGetBaseName("CfgAmmo " + className, ammo.ParentClass);
				m_Items.Ammunition.Insert(ammo);
				m_Ammo.Set(GGUtil.Key(className), ammo);
				m_AddedAmmo++;
				if (GGDebug.Enabled(10))
					GGDebug.Once(10, "DISCOVERY", "ammo_" + GGUtil.Key(className), "Added projectile " + className);
			}
			RefreshAmmo(ammo);
			ammo.IsCurrentlyLoaded = true;
		}
	}

	protected void RefreshAmmo(GGAmmoConfig ammo)
	{
		string path = "CfgAmmo " + ammo.ClassName;
		g_Game.ConfigGetBaseName(path, ammo.ParentClass);
		ammo.DetectedInitSpeed = g_Game.ConfigGetFloat(path + " initSpeed");
		ammo.DetectedTypicalSpeed = g_Game.ConfigGetFloat(path + " typicalSpeed");
		ammo.DetectedAirFriction = g_Game.ConfigGetFloat(path + " airFriction");
		ammo.DetectedHit = g_Game.ConfigGetFloat(path + " hit");
		ammo.DetectedIndirectHit = g_Game.ConfigGetFloat(path + " indirectHit");
		ammo.DetectedHealthDamage = g_Game.ConfigGetFloat(path + " DamageApplied Health damage");
		ammo.DetectedBloodDamage = g_Game.ConfigGetFloat(path + " DamageApplied Blood damage");
		ammo.DetectedShockDamage = g_Game.ConfigGetFloat(path + " DamageApplied Shock damage");
	}

	protected float GetAverageFloatArray(string path, float fallback)
	{
		if (!g_Game.ConfigIsExisting(path)) return fallback;
		TFloatArray values = new TFloatArray;
		g_Game.ConfigGetFloatArray(path, values);
		if (values.Count() == 0) return fallback;
		float total = 0.0;
		foreach (float value : values) total += value;
		return total / values.Count();
	}

	protected bool InheritsFrom(string root, string className, string expectedBase)
	{
		string current = className;
		for (int i = 0; i < 64 && current != ""; i++)
		{
			if (current == expectedBase) return true;
			string parent;
			if (!g_Game.ConfigGetBaseName(root + " " + current, parent) || parent == current) break;
			current = parent;
		}
		return false;
	}

	protected int CountLoadedWeapons()
	{
		int total = 0;
		foreach (GGWeaponConfig value : m_Items.Weapons)
		{
			if (value && value.IsCurrentlyLoaded) total++;
		}
		return total;
	}

	protected int CountLoadedAttachments()
	{
		int total = 0;
		foreach (GGAttachmentConfig value : m_Items.Attachments)
		{
			if (value && value.IsCurrentlyLoaded) total++;
		}
		return total;
	}

	protected int CountLoadedMagazines()
	{
		int total = 0;
		foreach (GGMagazineConfig value : m_Items.Magazines)
		{
			if (value && value.IsCurrentlyLoaded) total++;
		}
		return total;
	}

	protected int CountLoadedAmmo()
	{
		int total = 0;
		foreach (GGAmmoConfig value : m_Items.Ammunition)
		{
			if (value && value.IsCurrentlyLoaded) total++;
		}
		return total;
	}

	protected int CountLoadedArmor()
	{
		int total = 0;
		foreach (GGArmorConfig value : m_Items.Armor)
		{
			if (value && value.IsCurrentlyLoaded) total++;
		}
		return total;
	}

	protected int CountLoadedAttachmentCombinations()
	{
		int total = 0;
		foreach (GGWeaponAttachmentPolicy policy : m_WeaponAttachments.Weapons)
		{
			if (!policy || !policy.IsCurrentlyLoaded || !policy.Slots) continue;
			ref map<string, bool> countedClasses = new map<string, bool>;
			foreach (string slotName : policy.Slots)
			{
				TStringArray classNames;
				if (!m_AttachmentsBySlot.Find(GGUtil.Key(slotName), classNames)) continue;
				foreach (string className : classNames)
				{
					string classKey = GGUtil.Key(className);
					bool ignored;
					if (countedClasses.Find(classKey, ignored)) continue;
					countedClasses.Set(classKey, true);
					total++;
				}
			}
		}
		return total;
	}

	protected void RemoveMissingItems()
	{
		for (int weaponIndex = m_Items.Weapons.Count() - 1; weaponIndex >= 0; weaponIndex--)
			if (!m_Items.Weapons[weaponIndex] || !m_Items.Weapons[weaponIndex].IsCurrentlyLoaded) m_Items.Weapons.Remove(weaponIndex);
		for (int attachmentIndex = m_Items.Attachments.Count() - 1; attachmentIndex >= 0; attachmentIndex--)
			if (!m_Items.Attachments[attachmentIndex] || !m_Items.Attachments[attachmentIndex].IsCurrentlyLoaded) m_Items.Attachments.Remove(attachmentIndex);
		for (int magazineIndex = m_Items.Magazines.Count() - 1; magazineIndex >= 0; magazineIndex--)
			if (!m_Items.Magazines[magazineIndex] || !m_Items.Magazines[magazineIndex].IsCurrentlyLoaded) m_Items.Magazines.Remove(magazineIndex);
		for (int ammoIndex = m_Items.Ammunition.Count() - 1; ammoIndex >= 0; ammoIndex--)
			if (!m_Items.Ammunition[ammoIndex] || !m_Items.Ammunition[ammoIndex].IsCurrentlyLoaded) m_Items.Ammunition.Remove(ammoIndex);
		for (int armorIndex = m_Items.Armor.Count() - 1; armorIndex >= 0; armorIndex--)
			if (!m_Items.Armor[armorIndex] || !m_Items.Armor[armorIndex].IsCurrentlyLoaded) m_Items.Armor.Remove(armorIndex);
	}

	protected void RemoveMissingWeaponAttachmentPolicies()
	{
		for (int policyIndex = m_WeaponAttachments.Weapons.Count() - 1; policyIndex >= 0; policyIndex--)
		{
			GGWeaponAttachmentPolicy policy = m_WeaponAttachments.Weapons[policyIndex];
			if (!policy || !policy.IsCurrentlyLoaded)
			{
				m_WeaponAttachments.Weapons.Remove(policyIndex);
			}
		}
	}
}
