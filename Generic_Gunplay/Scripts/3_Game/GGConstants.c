class GGConstants
{
	static const int CONFIG_VERSION = 1;
	static const int SYNC_PROTOCOL_VERSION = 3;
	static const int WEAPON_ATTACHMENTS_FORMAT_VERSION = 2;

	static const string CONFIG_DIR = "$profile:Generic_Gunplay";
	static const string SETTINGS_FILE = "$profile:Generic_Gunplay\\Settings.json";
	static const string LEGACY_ITEMS_FILE = "$profile:Generic_Gunplay\\Items.json";
	static const string WEAPONS_FILE = "$profile:Generic_Gunplay\\Weapons.json";
	static const string ATTACHMENTS_FILE = "$profile:Generic_Gunplay\\Attachments.json";
	static const string MAGAZINES_FILE = "$profile:Generic_Gunplay\\Magazines.json";
	static const string AMMUNITION_FILE = "$profile:Generic_Gunplay\\Ammunition.json";
	static const string CLOTHING_ARMOR_FILE = "$profile:Generic_Gunplay\\ClothingArmor.json";
	static const string WEAPON_ATTACHMENTS_FILE = "$profile:Generic_Gunplay\\WeaponAttachments.json";
	static const string CLIENT_FILE = "$profile:Generic_Gunplay\\Client.json";

	static const int RPC_REQUEST_CONFIG = 4786101;
	static const int RPC_CONFIG_CHUNK = 4786102;
	static const int RPC_CONFIG_ERROR = 4786103;
	static const int RPC_CLIENT_CROSSHAIR = 4786104;
	static const int SYNC_CHUNK_SIZE = 7000;
	static const int MAX_SYNC_CHUNKS = 4096;
	static const int MAX_SYNC_CHARS = 24000000;
}

class GGUtil
{
	static string Key(string value)
	{
		string key = value;
		key.ToLower();
		return key;
	}

	static float Clamp(float value, float minimum, float maximum)
	{
		if (value < minimum) return minimum;
		if (value > maximum) return maximum;
		return value;
	}

	static bool NearlyEqual(float left, float right, float tolerance = 0.0005)
	{
		return Math.AbsFloat(left - right) <= tolerance;
	}

	static bool IsSafeIdentifier(string value)
	{
		if (value == "" || value.Length() > 128) return false;
		if ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_".IndexOf(value.Substring(0, 1)) == -1) return false;

		for (int i = 1; i < value.Length(); i++)
		{
			if ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789".IndexOf(value.Substring(i, 1)) == -1)
				return false;
		}

		return true;
	}

	static void Log(string message)
	{
		Print("[Generic Gunplay] " + message);
	}

	static void Warning(string message)
	{
		Print("[Generic Gunplay WARNING] " + message);
	}

	static void Error(string message)
	{
		Print("[Generic Gunplay ERROR] " + message);
	}
}
