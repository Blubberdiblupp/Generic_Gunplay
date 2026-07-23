class GGServerSyncTransfer : Managed
{
	int PayloadHash;
	int NextChunk;

	void GGServerSyncTransfer(int payloadHash)
	{
		PayloadHash = payloadHash;
		NextChunk = 0;
	}
}

class GGNetworkSync
{
	protected static int s_ExpectedHash;
	protected static int s_ExpectedChars;
	protected static int s_ExpectedChunks;
	protected static int s_ReceivedChunks;
	protected static int s_AppliedHash;
	protected static bool s_ClientReady;
	protected static bool s_ClientFailed;
	protected static ref array<string> s_Chunks;
	protected static ref array<bool> s_Received;

	protected static bool s_ServerCacheBuilt;
	protected static int s_ServerHash;
	protected static int s_ServerChars;
	protected static string s_ServerCacheError;
	protected static ref array<string> s_ServerChunks;
	protected static ref map<string, ref GGServerSyncTransfer> s_ServerTransfers = new map<string, ref GGServerSyncTransfer>;
	protected static ref map<string, int> s_ServerAcknowledgedHashes = new map<string, int>;

	static void BeginClientSync()
	{
		if (!g_Game || g_Game.IsServer()) return;
		GGDebug.SetClientReportingActive(true);
		s_ClientReady = false;
		s_ClientFailed = false;
		s_AppliedHash = 0;
		ResetTransfer();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(RequestUntilReady);
		RequestUntilReady();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RequestUntilReady, GGConstants.CLIENT_SYNC_RETRY_MS, true);
	}

	static void StopClientSync()
	{
		GGDebug.SetClientReportingActive(false);
		if (g_Game)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(RequestUntilReady);
		s_ClientReady = false;
		s_ClientFailed = false;
		s_AppliedHash = 0;
		ResetTransfer();
		GGDebug.Configure(0);
	}

	static bool IsClientReady()
	{
		return s_ClientReady;
	}

	protected static void RequestUntilReady()
	{
		if (s_ClientReady || s_ClientFailed)
		{
			if (g_Game)
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(RequestUntilReady);
			return;
		}
		RequestFromServer();
	}

	static void RequestFromServer()
	{
		if (!g_Game || g_Game.IsServer()) return;
		Param1<int> request = new Param1<int>(GGConstants.SYNC_PROTOCOL_VERSION);
		g_Game.RPCSingleParam(null, GGConstants.RPC_REQUEST_CONFIG, request, true, null);
	}

	static void SendToClient(PlayerIdentity identity, bool clientRequestedTransfer = false)
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

		string identityId = identity.GetId();
		if (identityId == "") return;

		int acknowledgedHash;
		if (!clientRequestedTransfer && s_ServerAcknowledgedHashes.Find(identityId, acknowledgedHash) && acknowledgedHash == s_ServerHash)
			return;

		int total = s_ServerChunks.Count();
		GGServerSyncTransfer existing;
		if (s_ServerTransfers.Find(identityId, existing) && existing && existing.PayloadHash == s_ServerHash)
			return;

		Param4<int, int, int, int> hello = new Param4<int, int, int, int>(s_ServerHash, s_ServerChars, total, GGConstants.SYNC_PROTOCOL_VERSION);
		g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_HELLO, hello, true, identity);
		string transferDebug = "Starting config transfer. Client=" + identity.GetName();
		transferDebug += " hash=" + s_ServerHash.ToString();
		transferDebug += " chars=" + s_ServerChars.ToString();
		transferDebug += " chunks=" + total.ToString();
		GGDebug.Log(3, "SYNC", transferDebug);
		s_ServerAcknowledgedHashes.Remove(identityId);
		s_ServerTransfers.Set(identityId, new GGServerSyncTransfer(s_ServerHash));
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SendNextBurst, 1, false, identity, s_ServerHash);
	}

	protected static void SendNextBurst(PlayerIdentity identity, int expectedHash)
	{
		if (!g_Game || !g_Game.IsServer() || !identity) return;
		string identityId = identity.GetId();
		GGServerSyncTransfer transfer;
		if (identityId == "" || !s_ServerTransfers.Find(identityId, transfer) || !transfer) return;
		if (transfer.PayloadHash != expectedHash || expectedHash != s_ServerHash) return;

		int total = s_ServerChunks.Count();
		int sent = 0;
		while (transfer.NextChunk < total && sent < GGConstants.SERVER_SYNC_BURST_CHUNKS)
		{
			int index = transfer.NextChunk;
			Param4<int, int, int, string> data = new Param4<int, int, int, string>(s_ServerHash, index, total, s_ServerChunks.Get(index));
			g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_CHUNK, data, true, identity);
			transfer.NextChunk++;
			sent++;
		}
		GGDebug.Count(9, "SYNC", "config_chunks_sent", 10000, sent);

		if (transfer.NextChunk < total)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SendNextBurst, GGConstants.SERVER_SYNC_BURST_MS, false, identity, expectedHash);
			return;
		}
		GGUtil.Log("Sent client config to " + identity.GetName() + ". Chunks=" + total.ToString() + ".");
	}

	static void HandleClientAck(PlayerIdentity identity, int payloadHash, int protocolVersion)
	{
		if (!g_Game || !g_Game.IsServer() || !identity) return;
		if (protocolVersion != GGConstants.SYNC_PROTOCOL_VERSION || payloadHash != s_ServerHash)
		{
			GGUtil.Warning("Rejected invalid client config acknowledgement from " + identity.GetName() + ".");
			return;
		}
		string identityId = identity.GetId();
		s_ServerTransfers.Remove(identityId);
		s_ServerAcknowledgedHashes.Set(identityId, payloadHash);
		GGUtil.Log("Client config acknowledged by " + identity.GetName() + ".");
		GGDebug.Log(3, "SYNC", "Config acknowledgement accepted. Client=" + identity.GetName() + " hash=" + payloadHash.ToString());
	}

	static void RemoveClient(PlayerIdentity identity)
	{
		if (!identity) return;
		string identityId = identity.GetId();
		s_ServerTransfers.Remove(identityId);
		s_ServerAcknowledgedHashes.Remove(identityId);
	}

	static void InvalidateServerCache()
	{
		s_ServerCacheBuilt = false;
		s_ServerHash = 0;
		s_ServerChars = 0;
		s_ServerCacheError = "";
		s_ServerChunks = null;
		s_ServerTransfers.Clear();
		s_ServerAcknowledgedHashes.Clear();
		GGDebug.Log(8, "CACHE", "Synchronized payload cache invalidated.");
	}

	protected static bool BuildServerCache(out string errorMessage)
	{
		if (s_ServerCacheBuilt)
		{
			errorMessage = s_ServerCacheError;
			return errorMessage == "";
		}

		int debugStarted = GGDebug.BeginTiming(9);
		s_ServerCacheBuilt = true;
		s_ServerChunks = new array<string>;
		GGSyncPayload payload = GetGGConfigManager().CreateSyncPayload();
		GGWireSyncPayload wirePayload = new GGWireSyncPayload();
		if (!wirePayload.Load(payload))
		{
			s_ServerCacheError = "Could not build compact Generic Gunplay client config.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		string json;
		string serializeError;
		if (!JsonFileLoader<GGWireSyncPayload>.MakeData(wirePayload, json, serializeError, false))
		{
			s_ServerCacheError = "Could not serialize Generic Gunplay config: " + serializeError;
			errorMessage = s_ServerCacheError;
			return false;
		}

		GGWireSyncPayload roundTripWire = new GGWireSyncPayload();
		string roundTripError;
		if (!JsonFileLoader<GGWireSyncPayload>.LoadData(json, roundTripWire, roundTripError))
		{
			s_ServerCacheError = "Could not verify Generic Gunplay client config: " + roundTripError;
			errorMessage = s_ServerCacheError;
			return false;
		}
		GGSyncPayload roundTripPayload;
		if (!roundTripWire.Expand(roundTripPayload))
		{
			s_ServerCacheError = "Could not expand compact Generic Gunplay client config.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (!GetGGConfigManager().ValidateSyncedPayload(roundTripPayload))
		{
			s_ServerCacheError = "Generic Gunplay client config failed semantic round-trip validation.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		string roundTripJson;
		string roundTripSerializeError;
		if (!JsonFileLoader<GGWireSyncPayload>.MakeData(roundTripWire, roundTripJson, roundTripSerializeError, false))
		{
			s_ServerCacheError = "Could not reserialize Generic Gunplay client config: " + roundTripSerializeError;
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripJson.Length() != json.Length() || roundTripJson.Hash() != json.Hash())
		{
			s_ServerCacheError = "Generic Gunplay client config changed during JSON round-trip.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripPayload.Items.Weapons.Count() != payload.Items.Weapons.Count())
		{
			s_ServerCacheError = "Generic Gunplay weapon count changed during client config serialization.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripPayload.Items.Attachments.Count() != payload.Items.Attachments.Count())
		{
			s_ServerCacheError = "Generic Gunplay attachment count changed during client config serialization.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripPayload.Items.Magazines.Count() != payload.Items.Magazines.Count())
		{
			s_ServerCacheError = "Generic Gunplay magazine count changed during client config serialization.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripPayload.Items.Ammunition.Count() != payload.Items.Ammunition.Count())
		{
			s_ServerCacheError = "Generic Gunplay ammunition count changed during client config serialization.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripPayload.Items.Armor.Count() != payload.Items.Armor.Count())
		{
			s_ServerCacheError = "Generic Gunplay armor count changed during client config serialization.";
			errorMessage = s_ServerCacheError;
			return false;
		}
		if (roundTripPayload.BlockedAttachments.Count() != payload.BlockedAttachments.Count())
		{
			s_ServerCacheError = "Generic Gunplay blocked-attachment count changed during client config serialization.";
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
		if (total <= 0 || total > GGConstants.MAX_SYNC_CHUNKS)
		{
			s_ServerCacheError = "Generic Gunplay sync requires an invalid number of chunks.";
			errorMessage = s_ServerCacheError;
			return false;
		}

		s_ServerHash = json.Hash();
		s_ServerChars = jsonLength;
		for (int i = 0; i < total; i++)
		{
			int start = i * GGConstants.SYNC_CHUNK_SIZE;
			int length = Math.Min(GGConstants.SYNC_CHUNK_SIZE, jsonLength - start);
			s_ServerChunks.Insert(json.Substring(start, length));
		}
		GGUtil.Log("Prepared client config payload. Characters=" + s_ServerChars.ToString() + ", chunks=" + total.ToString() + ".");
		GGDebug.Log(3, "SYNC", "Payload cache ready. hash=" + s_ServerHash.ToString() + " characters=" + s_ServerChars.ToString() + " chunks=" + total.ToString());
		GGDebug.EndTiming(9, "PERFORMANCE", "Build synchronized payload", debugStarted, "characters=" + s_ServerChars.ToString() + " chunks=" + total.ToString());
		errorMessage = "";
		return true;
	}

	static void ReceiveHello(int hash, int chars, int total, int protocolVersion)
	{
		if (!g_Game || g_Game.IsServer()) return;
		if (protocolVersion != GGConstants.SYNC_PROTOCOL_VERSION)
		{
			FailClientSync("Client/server Generic Gunplay protocol mismatch.");
			return;
		}
		int calculatedChunks = (chars + GGConstants.SYNC_CHUNK_SIZE - 1) / GGConstants.SYNC_CHUNK_SIZE;
		if (chars <= 0 || chars > GGConstants.MAX_SYNC_CHARS)
		{
			FailClientSync("Server sent invalid Generic Gunplay payload size.");
			return;
		}
		if (total <= 0 || total > GGConstants.MAX_SYNC_CHUNKS)
		{
			FailClientSync("Server sent invalid Generic Gunplay chunk count.");
			return;
		}
		if (total != calculatedChunks)
		{
			FailClientSync("Server sent invalid Generic Gunplay payload metadata.");
			return;
		}
		if (s_ClientReady && s_AppliedHash == hash)
		{
			SendClientAck(hash);
			return;
		}
		if (!s_Chunks || hash != s_ExpectedHash || chars != s_ExpectedChars || total != s_ExpectedChunks)
			BeginTransfer(hash, chars, total);
		s_ClientFailed = false;
	}

	static void ReceiveChunk(int hash, int index, int total, string chunk)
	{
		if (!g_Game || g_Game.IsServer() || s_ClientReady) return;
		if (!s_Chunks || hash != s_ExpectedHash || total != s_ExpectedChunks) return;
		if (index < 0 || index >= total || chunk.Length() > GGConstants.SYNC_CHUNK_SIZE) return;
		int expectedLength = GGConstants.SYNC_CHUNK_SIZE;
		if (index == total - 1)
			expectedLength = s_ExpectedChars - (index * GGConstants.SYNC_CHUNK_SIZE);
		if (chunk.Length() != expectedLength)
		{
			FailClientSync("Server sent a Generic Gunplay chunk with an invalid length.");
			return;
		}

		if (!s_Received.Get(index))
		{
			s_Chunks.Set(index, chunk);
			s_Received.Set(index, true);
			s_ReceivedChunks++;
		}
		if (s_ReceivedChunks != s_ExpectedChunks) return;

		string json = "";
		for (int i = 0; i < s_ExpectedChunks; i++)
			json += s_Chunks.Get(i);
		if (json.Length() != s_ExpectedChars || json.Hash() != s_ExpectedHash)
		{
			FailClientSync("Client config sync failed integrity validation.");
			return;
		}

		GGWireSyncPayload wirePayload = new GGWireSyncPayload();
		string error;
		if (!JsonFileLoader<GGWireSyncPayload>.LoadData(json, wirePayload, error))
		{
			FailClientSync("Client config sync JSON is invalid: " + error);
			return;
		}
		string roundTripJson;
		string roundTripError;
		if (!JsonFileLoader<GGWireSyncPayload>.MakeData(wirePayload, roundTripJson, roundTripError, false))
		{
			FailClientSync("Client config sync could not be verified: " + roundTripError);
			return;
		}
		if (roundTripJson.Length() != json.Length() || roundTripJson.Hash() != json.Hash())
		{
			FailClientSync("Client config sync changed during JSON round-trip.");
			return;
		}

		GGSyncPayload payload;
		if (!wirePayload.Expand(payload))
		{
			FailClientSync("Client config sync could not be expanded.");
			return;
		}
		int appliedHash = s_ExpectedHash;
		if (!GetGGConfigManager().ApplySyncedPayload(payload))
		{
			FailClientSync("Client rejected the synchronized Generic Gunplay config.");
			return;
		}
		s_AppliedHash = appliedHash;
		s_ClientReady = true;
		s_ClientFailed = false;
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(RequestUntilReady);
		SendClientCrosshairPreference();
		SendClientAck(appliedHash);
		ResetTransfer();
		GGUtil.Log("Server config received and activated on client.");
		GGDebug.ClientOnce(3, "SYNC", "payload_" + appliedHash.ToString(), "Payload received, validated and activated. hash=" + appliedHash.ToString());
	}

	static void SendClientCrosshairPreference()
	{
		if (!g_Game || g_Game.IsServer() || !g_Game.GetPlayer()) return;
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.AllowClientCrosshairChoice) return;
		int mode = GetGGConfigManager().GetEffectiveCrosshairMode();
		Param1<int> preference = new Param1<int>(mode);
		g_Game.RPCSingleParam(g_Game.GetPlayer(), GGConstants.RPC_CLIENT_CROSSHAIR, preference, true, null);
		GGDebug.ClientState(7, "CROSSHAIR", "preference", mode.ToString(), "Submitted client crosshair preference");
	}

	protected static void SendClientAck(int hash)
	{
		if (!g_Game || g_Game.IsServer()) return;
		Param2<int, int> ack = new Param2<int, int>(hash, GGConstants.SYNC_PROTOCOL_VERSION);
		g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_ACK, ack, true, null);
		GGDebug.ClientCount(9, "SYNC", "config_ack_sent", 10000);
	}

	static void ReceiveError(string message)
	{
		FailClientSync("Server config sync failed: " + message);
	}

	protected static void SendError(PlayerIdentity identity, string message)
	{
		Param1<string> error = new Param1<string>(message);
		g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_ERROR, error, true, identity);
		GGUtil.Error(message);
	}

	protected static void BeginTransfer(int hash, int chars, int total)
	{
		s_ExpectedHash = hash;
		s_ExpectedChars = chars;
		s_ExpectedChunks = total;
		s_ReceivedChunks = 0;
		s_Chunks = new array<string>;
		s_Received = new array<bool>;
		for (int i = 0; i < total; i++)
		{
			s_Chunks.Insert("");
			s_Received.Insert(false);
		}
	}

	protected static void FailClientSync(string message)
	{
		s_ClientReady = false;
		s_ClientFailed = true;
		if (g_Game)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(RequestUntilReady);
		GGUtil.Error(message);
		ResetTransfer();
	}

	protected static void ResetTransfer()
	{
		s_ExpectedHash = 0;
		s_ExpectedChars = 0;
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
				if (request.param1 == GGConstants.SYNC_PROTOCOL_VERSION)
					GGNetworkSync.SendToClient(sender, true);
				else
					g_Game.RPCSingleParam(null, GGConstants.RPC_CONFIG_ERROR, new Param1<string>("Client/server Generic Gunplay protocol mismatch."), true, sender);
			}
			return;
		}

		if (rpc_type == GGConstants.RPC_CONFIG_ACK && g_Game && g_Game.IsServer())
		{
			Param2<int, int> ack = new Param2<int, int>(0, 0);
			if (ctx.Read(ack) && sender)
				GGNetworkSync.HandleClientAck(sender, ack.param1, ack.param2);
			return;
		}

		if (rpc_type == GGConstants.RPC_DEBUG_EVENT && g_Game && g_Game.IsServer())
		{
			Param3<int, string, string> debugEvent = new Param3<int, string, string>(0, "", "");
			if (ctx.Read(debugEvent) && sender)
				GGDebug.ReceiveClientEvent(sender, debugEvent.param1, debugEvent.param2, debugEvent.param3);
			return;
		}

		if (rpc_type == GGConstants.RPC_CONFIG_HELLO && g_Game && !g_Game.IsServer())
		{
			Param4<int, int, int, int> hello = new Param4<int, int, int, int>(0, 0, 0, 0);
			if (ctx.Read(hello))
				GGNetworkSync.ReceiveHello(hello.param1, hello.param2, hello.param3, hello.param4);
			return;
		}

		if (rpc_type == GGConstants.RPC_CONFIG_CHUNK && g_Game && !g_Game.IsServer())
		{
			Param4<int, int, int, string> data = new Param4<int, int, int, string>(0, 0, 0, "");
			if (ctx.Read(data))
				GGNetworkSync.ReceiveChunk(data.param1, data.param2, data.param3, data.param4);
			return;
		}

		if (rpc_type == GGConstants.RPC_CONFIG_ERROR && g_Game && !g_Game.IsServer())
		{
			Param1<string> error = new Param1<string>("");
			if (ctx.Read(error))
				GGNetworkSync.ReceiveError(error.param1);
			return;
		}

		super.OnRPC(sender, target, rpc_type, ctx);
	}
}
