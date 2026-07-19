class CfgPatches
{
	class Generic_Gunplay
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 1.25;
		requiredAddons[] =
		{
			"DZ_Data",
			"DZ_Scripts",
			"DZ_Weapons_Firearms",
			"DZ_Weapons_Magazines",
			"DZ_Weapons_Ammunition",
			"DZ_Weapons_Muzzles",
			"DZ_Weapons_Optics",
			"DZ_Weapons_Supports"
		};
	};
};

class CfgMods
{
	class Generic_Gunplay
	{
		dir = "Generic_Gunplay";
		picture = "";
		action = "";
		hideName = 0;
		hidePicture = 0;
		name = "Generic Gunplay";
		credits = "";
		author = "Blubber";
		authorID = "76561197995145122";
		version = "1.0";
		extra = 0;
		type = "mod";
		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"Generic_Gunplay/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"Generic_Gunplay/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"Generic_Gunplay/Scripts/5_Mission"};
			};
		};
	};
};
