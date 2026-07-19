class GGConfigMigration
{
	static bool Migrate(GGSettings settings, GGItemsConfig items, out bool changed, out string error)
	{
		changed = false;
		error = "";
		if (!settings || !items)
		{
			error = "The config root is missing.";
			return false;
		}
		if (settings.ConfigVersion > GGConstants.CONFIG_VERSION || items.ConfigVersion > GGConstants.CONFIG_VERSION)
		{
			error = "A config file was written by a newer Generic_Gunplay version.";
			return false;
		}

		if (settings.ConfigVersion < 1)
		{
			settings.ConfigVersion = 1;
			changed = true;
		}
		if (items.ConfigVersion < 1)
		{
			items.ConfigVersion = 1;
			changed = true;
		}
		return true;
	}
}
