class GGNetworkSync
{
	protected static int s_ExpectedHash;
	protected static int s_ExpectedChunks;
	protected static int s_ReceivedChunks;
	protected static ref array<string> s_Chunks;
	protected static ref array<bool> s_Received;
	protected static bool s_ServerCacheBuilt;
	protected static int s_ServerHash;
	protected static string s_ServerCacheError;
	protected static ref array<string> s_ServerChunks;

	static void RequestFromServer()
	{
		if (!g_Game || g_Game.IsServer()) return;
		g_Game.RPCSingleParam(null, GGConstants.RPC_REQUEST_CONFIG, new Param1<int>(GGConstants.SYNC_PROTOCOL_VERSION), true, null);
	}

	static void SendToClient(PlayerIdentity identity)
	{
		if (!g_Game || !g_Game.IsServer() || !identity) return;
		GGConfigManager config = GetGGConfigManager();
		config.EnsureReady();
		if (config.HasLoadError())
		{
			SendError(identity, "Server configuration did not load; Generic Gunplay is disabled.");
			return;
		}
		string cacheError;
		if (!BuildServerCache(cacheError))
		{
			SendError(identity, cacheError);
			return;
		}

		int total = s_ServerChunks.Count();
		for (int i = 0; i < total; i++)
		{
			string chunk = s_ServerChunks.Get(i);
			Param4<int, int, int, string> data = new Param4<int, int, int, string>(s_ServerHash, i, total, chunk);
			g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_CHUNK, data, true, identity);
		}
	}

	static void InvalidateServerCache()
	{
		s_ServerCacheBuilt = false;
		s_ServerHash = 0;
		s_ServerCacheError = "";
		s_ServerChunks = null;
	}

	protected static bool BuildServerCache(out string errorMessage)
	{
		if (s_ServerCacheBuilt)
		{
			errorMessage = s_ServerCacheError;
			return errorMessage == "";
		}

		s_ServerCacheBuilt = true;
		s_ServerChunks = new array<string>;
		GGSyncPayload payload = GetGGConfigManager().CreateSyncPayload();
		string json;
		string serializeError;
		if (!JsonFileLoader<GGSyncPayload>.MakeData(payload, json, serializeError, false))
		{
			s_ServerCacheError = "Could not serialize Generic Gunplay config: " + serializeError;
			errorMessage = s_ServerCacheError;
			return false;
		}

		int jsonLength = json.Length();
		if (jsonLength <= 0 || jsonLength > GGConstants.MAX_SYNC_CHARS)
		{
			s_ServerCacheError = "Generic Gunplay sync payload exceeds the supported size limit.";
			errorMessage = s_ServerCacheError;
			return false;
		}

		int total = (jsonLength + GGConstants.SYNC_CHUNK_SIZE - 1) / GGConstants.SYNC_CHUNK_SIZE;
		if (total <= 0) total = 1;
		if (total > GGConstants.MAX_SYNC_CHUNKS)
		{
			s_ServerCacheError = "Generic Gunplay sync requires too many chunks.";
			errorMessage = s_ServerCacheError;
			return false;
		}

		s_ServerHash = json.Hash();
		for (int i = 0; i < total; i++)
		{
			int start = i * GGConstants.SYNC_CHUNK_SIZE;
			int length = Math.Min(GGConstants.SYNC_CHUNK_SIZE, jsonLength - start);
			s_ServerChunks.Insert(json.Substring(start, length));
		}
		errorMessage = "";
		return true;
	}

	static void ReceiveChunk(int hash, int index, int total, string chunk)
	{
		if (!g_Game || g_Game.IsServer()) return;
		if (total <= 0 || total > GGConstants.MAX_SYNC_CHUNKS || index < 0 || index >= total) return;

		if (!s_Chunks || hash != s_ExpectedHash || total != s_ExpectedChunks)
		{
			s_ExpectedHash = hash;
			s_ExpectedChunks = total;
			s_ReceivedChunks = 0;
			s_Chunks = new array<string>;
			s_Received = new array<bool>;
			s_Chunks.Resize(total);
			s_Received.Resize(total);
		}

		if (!s_Received.Get(index))
		{
			s_Chunks.Set(index, chunk);
			s_Received.Set(index, true);
			s_ReceivedChunks++;
		}
		if (s_ReceivedChunks != s_ExpectedChunks) return;

		string json = "";
		for (int i = 0; i < s_ExpectedChunks; i++) json += s_Chunks.Get(i);
		if (json.Length() > GGConstants.MAX_SYNC_CHARS || json.Hash() != s_ExpectedHash)
		{
			GGUtil.Error("Client config sync failed integrity validation.");
			Reset();
			return;
		}

		GGSyncPayload payload = new GGSyncPayload();
		string error;
		if (!JsonFileLoader<GGSyncPayload>.LoadData(json, payload, error))
		{
			GGUtil.Error("Client config sync JSON is invalid: " + error);
			Reset();
			return;
		}
		GetGGConfigManager().ApplySyncedPayload(payload);
		SendClientCrosshairPreference();
		Reset();
	}

	static void SendClientCrosshairPreference()
	{
		if (!g_Game || g_Game.IsServer() || !g_Game.GetPlayer()) return;
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.AllowClientCrosshairChoice) return;
		int mode = GetGGConfigManager().GetEffectiveCrosshairMode();
		g_Game.RPCSingleParam(g_Game.GetPlayer(), GGConstants.RPC_CLIENT_CROSSHAIR, new Param1<int>(mode), true, null);
	}

	static void ReceiveError(string message)
	{
		GGUtil.Error("Server config sync failed: " + message);
	}

	protected static void SendError(PlayerIdentity identity, string message)
	{
		g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_ERROR, new Param1<string>(message), true, identity);
		GGUtil.Error(message);
	}

	protected static void Reset()
	{
		s_ExpectedHash = 0;
		s_ExpectedChunks = 0;
		s_ReceivedChunks = 0;
		s_Chunks = null;
		s_Received = null;
	}
}

modded class DayZGame
{
	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == GGConstants.RPC_REQUEST_CONFIG && g_Game && g_Game.IsServer())
		{
			Param1<int> request = new Param1<int>(0);
			if (ctx.Read(request) && sender)
			{
				if (request.param1 == GGConstants.SYNC_PROTOCOL_VERSION) GGNetworkSync.SendToClient(sender);
				else g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_ERROR, new Param1<string>("Client/server Generic Gunplay protocol mismatch."), true, sender);
			}
			return;
		}

		if (rpc_type == GGConstants.RPC_CONFIG_CHUNK && g_Game && !g_Game.IsServer())
		{
			Param4<int, int, int, string> data = new Param4<int, int, int, string>(0, 0, 0, "");
			if (ctx.Read(data)) GGNetworkSync.ReceiveChunk(data.param1, data.param2, data.param3, data.param4);
			return;
		}

		if (rpc_type == GGConstants.RPC_CONFIG_ERROR && g_Game && !g_Game.IsServer())
		{
			Param1<string> error = new Param1<string>("");
			if (ctx.Read(error)) GGNetworkSync.ReceiveError(error.param1);
			return;
		}

		super.OnRPC(sender, target, rpc_type, ctx);
	}
}
