modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		if (g_Game && g_Game.IsServer())
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GGLoadServerConfig, 250, false);
	}

	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		if (g_Game && g_Game.IsServer() && identity)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GGSendServerConfig, 750, false, identity);
	}

	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
		GGNetworkSync.RemoveClient(identity);
		super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
	}
}

void GGLoadServerConfig()
{
	GetGGConfigManager().LoadServerConfig();
}

void GGSendServerConfig(PlayerIdentity identity)
{
	GGNetworkSync.SendToClient(identity);
}

modded class MissionGameplay
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetGGConfigManager().InitializeClient();
		GGNetworkSync.BeginClientSync();
	}

	override void OnMissionFinish()
	{
		GGNetworkSync.StopClientSync();
		super.OnMissionFinish();
	}
}
