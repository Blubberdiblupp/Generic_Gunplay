modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		if (g_Game && g_Game.IsServer())
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GGLoadServerConfig, 250, false);
	}
}

void GGLoadServerConfig()
{
	GetGGConfigManager().LoadServerConfig();
}

modded class MissionGameplay
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetGGConfigManager().InitializeClient();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GGRequestServerConfig, 1000, false);
	}
}

void GGRequestServerConfig()
{
	GGNetworkSync.RequestFromServer();
}
