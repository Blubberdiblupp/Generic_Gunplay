class GGTierCatalog
{
	static bool FillMissing(array<ref GGTierDefinition> definitions)
	{
		if (!definitions) return false;
		bool changed = false;
		changed = Add(definitions, new GGTierDefinition("Foregrip_T1", "Foregrip", "T1", 0.70, 0.925, 1.03, 1.05, 0.90)) || changed;
		changed = Add(definitions, new GGTierDefinition("Foregrip_T2", "Foregrip", "T2", 0.40, 0.85, 1.06, 1.10, 0.80)) || changed;
		changed = Add(definitions, new GGTierDefinition("Foregrip_T3", "Foregrip", "T3", 0.10, 0.775, 1.08, 1.15, 0.70)) || changed;
		changed = Add(definitions, new GGTierDefinition("PistolGrip_T1", "Pistol Grip", "T1", 0.70, 1.00, 1.05, 1.00, 0.90)) || changed;
		changed = Add(definitions, new GGTierDefinition("PistolGrip_T2", "Pistol Grip", "T2", 0.40, 0.97, 1.10, 1.02, 0.80)) || changed;
		changed = Add(definitions, new GGTierDefinition("PistolGrip_T3", "Pistol Grip", "T3", 0.10, 0.94, 1.15, 1.04, 0.70)) || changed;
		changed = Add(definitions, new GGTierDefinition("Stock_T1", "Stock", "T1", 0.70, 0.85, 1.00, 1.05, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Stock_T2", "Stock", "T2", 0.40, 0.70, 0.99, 1.10, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Stock_T3", "Stock", "T3", 0.10, 0.55, 0.98, 1.15, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Handguard_T1", "Handguard", "T1", 1.00, 0.85, 1.02, 1.10, 0.90)) || changed;
		changed = Add(definitions, new GGTierDefinition("Handguard_T2", "Handguard", "T2", 0.94, 0.70, 1.04, 1.20, 0.80)) || changed;
		changed = Add(definitions, new GGTierDefinition("Handguard_T3", "Handguard", "T3", 0.88, 0.55, 1.06, 1.30, 0.70)) || changed;
		changed = Add(definitions, new GGTierDefinition("Bipod_Special", "Bipod", "Special", 0.70, 0.43, 0.86, 1.16, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("LightOptic_T1", "Optic", "T1", 1.00, 0.985, 1.05, 1.10, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("HeavyOptic_T2", "Optic", "T2", 1.00, 0.97, 0.95, 1.20, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("SniperOptic_T3", "Optic", "T3", 1.00, 0.955, 0.85, 1.30, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Laser_Tactical", "Laser", "Tactical", 1.00, 1.00, 1.00, 1.00, 0.55)) || changed;
		changed = Add(definitions, new GGTierDefinition("Flashlight_Utility", "Flashlight", "Utility", 1.00, 1.04, 0.96, 1.00, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Bayonet_Heavy", "Bayonet", "Heavy", 1.00, 1.08, 0.92, 1.00, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Muzzle_T1", "Muzzle", "T1", 0.85, 0.925, 1.00, 1.05, 0.95)) || changed;
		changed = Add(definitions, new GGTierDefinition("Muzzle_T2", "Muzzle", "T2", 0.70, 0.85, 0.99, 1.10, 0.90)) || changed;
		changed = Add(definitions, new GGTierDefinition("Muzzle_T3", "Muzzle", "T3", 0.55, 0.775, 0.98, 1.15, 0.85)) || changed;
		changed = Add(definitions, new GGTierDefinition("Suppressor_T1", "Suppressor", "T1", 0.85, 1.00, 0.98, 1.05, 0.95)) || changed;
		changed = Add(definitions, new GGTierDefinition("Suppressor_T2", "Suppressor", "T2", 0.70, 1.01, 0.96, 1.10, 0.90)) || changed;
		changed = Add(definitions, new GGTierDefinition("Suppressor_T3", "Suppressor", "T3", 0.55, 1.02, 0.94, 1.15, 0.85)) || changed;
		changed = Add(definitions, new GGTierDefinition("WeaponWrap_Utility", "Wrap", "Utility", 1.00, 0.925, 0.97, 1.03, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("StandardMag_Neutral", "Magazine", "Standard", 1.00, 1.00, 1.00, 1.00, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("HighCap_Heavy", "Magazine", "HighCap", 1.02, 1.03, 0.98, 1.00, 1.00)) || changed;
		changed = Add(definitions, new GGTierDefinition("Neutral", "Neutral", "Neutral", 1.00, 1.00, 1.00, 1.00, 1.00)) || changed;
		return changed;
	}

	protected static bool Add(array<ref GGTierDefinition> definitions, GGTierDefinition candidate)
	{
		foreach (GGTierDefinition existing : definitions)
		{
			if (existing && GGUtil.Key(existing.TierKey) == GGUtil.Key(candidate.TierKey)) return false;
		}
		definitions.Insert(candidate);
		return true;
	}
}
