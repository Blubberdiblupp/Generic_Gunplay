modded class PlayerBase
{
	protected int m_GGCrosshairPreference = -1;

	void PlayerBase()
	{
		RegisterNetSyncVariableInt("m_GGCrosshairPreference", -1, 2);
	}

	override void EEInit()
	{
		super.EEInit();
		if (g_Game && !g_Game.IsServer())
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GGNetworkSync.SendClientCrosshairPreference, 1500, false);
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == GGConstants.RPC_CLIENT_CROSSHAIR && g_Game && g_Game.IsServer())
		{
			Param1<int> preference = new Param1<int>(-1);
			GGSettings settings = GetGGConfigManager().GetSettings();
			bool validPreference = ctx.Read(preference);
			PlayerIdentity identity = GetIdentity();
			if (validPreference && sender && identity && settings)
			{
				if (sender.GetId() == identity.GetId() && settings.AllowClientCrosshairChoice)
				{
					m_GGCrosshairPreference = Math.Clamp(preference.param1, 0, 2);
					SetSynchDirty();
					GGDebug.State(7, "CROSSHAIR", "preference_" + identity.GetId(), m_GGCrosshairPreference.ToString(), "Accepted crosshair preference from " + identity.GetName());
				}
			}
			return;
		}
		super.OnRPC(sender, rpc_type, ctx);
	}

	int GetGGResolvedCrosshairMode()
	{
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings) return 0;
		if (!settings.AllowClientCrosshairChoice) return Math.Clamp(settings.CrosshairMode, 0, 2);
		if (m_GGCrosshairPreference >= 0) return Math.Clamp(m_GGCrosshairPreference, 0, 2);
		return Math.Clamp(settings.CrosshairMode, 0, 2);
	}
}
