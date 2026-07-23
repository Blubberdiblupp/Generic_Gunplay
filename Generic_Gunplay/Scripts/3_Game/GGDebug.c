class GGDebug
{
	protected static int s_Level;
	protected static ref map<string, bool> s_Once;
	protected static ref map<string, string> s_States;
	protected static ref map<string, int> s_RateTimes;
	protected static ref map<string, int> s_Counters;
	protected static ref map<string, int> s_CounterTimes;
	protected static ref map<string, bool> s_ClientOnce;
	protected static ref map<string, string> s_ClientStates;
	protected static ref map<string, int> s_ClientRateTimes;
	protected static ref map<string, int> s_ClientCounters;
	protected static ref map<string, int> s_ClientCounterTimes;
	protected static ref map<string, int> s_ServerClientRateTimes;
	protected static bool s_ClientReportingActive;

	static void Configure(int level)
	{
		int normalized = Math.Clamp(level, 0, 10);
		bool changed = normalized != s_Level;
		s_Level = normalized;
		EnsureStorage();
		if (!changed) return;

		ClearTransientState();
		if (s_Level > 0 && IsServerContext())
			Print("[Generic Gunplay DEBUG] Server debug level " + s_Level.ToString() + " enabled.");
	}

	static int GetLevel()
	{
		return s_Level;
	}

	static void SetClientReportingActive(bool active)
	{
		s_ClientReportingActive = active;
		if (active) return;
		EnsureStorage();
		s_ClientOnce.Clear();
		s_ClientStates.Clear();
		s_ClientRateTimes.Clear();
		s_ClientCounters.Clear();
		s_ClientCounterTimes.Clear();
	}

	static bool Enabled(int requiredLevel)
	{
		return requiredLevel > 0 && s_Level >= requiredLevel;
	}

	static string BoolString(bool value)
	{
		if (value) return "true";
		return "false";
	}

	static void Log(int level, string category, string message)
	{
		if (!Enabled(level) || !IsServerContext()) return;
		WriteServer(level, category, message);
	}

	static void Once(int level, string category, string key, string message)
	{
		if (!Enabled(level) || !IsServerContext()) return;
		EnsureStorage();
		string storageKey = MakeKey(category, key);
		bool ignored;
		if (s_Once.Find(storageKey, ignored)) return;
		s_Once.Set(storageKey, true);
		WriteServer(level, category, message);
	}

	static void State(int level, string category, string key, string value, string message)
	{
		if (!Enabled(level) || !IsServerContext()) return;
		EnsureStorage();
		string storageKey = MakeKey(category, key);
		string previous;
		if (s_States.Find(storageKey, previous) && previous == value) return;
		s_States.Set(storageKey, value);
		WriteServer(level, category, message + " state=" + value);
	}

	static void RateLimited(int level, string category, string key, int intervalMs, string message)
	{
		if (!Enabled(level) || !IsServerContext()) return;
		EnsureStorage();
		int now = Now();
		string storageKey = MakeKey(category, key);
		int previous;
		if (s_RateTimes.Find(storageKey, previous) && now - previous < Math.Max(intervalMs, 1)) return;
		s_RateTimes.Set(storageKey, now);
		WriteServer(level, category, message);
	}

	static void Count(int level, string category, string key, int flushIntervalMs = 10000, int amount = 1)
	{
		if (!Enabled(level) || !IsServerContext()) return;
		EnsureStorage();
		AccumulateServerCount(level, category, key, flushIntervalMs, amount);
	}

	static int BeginTiming(int level)
	{
		if (!Enabled(level) || !IsServerContext()) return -1;
		return Now();
	}

	static void EndTiming(int level, string category, string operation, int startedAt, string details = "")
	{
		if (startedAt < 0 || !Enabled(level) || !IsServerContext()) return;
		int elapsed = Math.Max(0, Now() - startedAt);
		string message = operation + " completed in " + elapsed.ToString() + " ms";
		if (details != "") message += ". " + details;
		WriteServer(level, category, message);
	}

	static void ClientOnce(int level, string category, string key, string message)
	{
		if (!CanSendClientEvent(level)) return;
		EnsureStorage();
		string storageKey = MakeKey(category, key);
		bool ignored;
		if (s_ClientOnce.Find(storageKey, ignored)) return;
		s_ClientOnce.Set(storageKey, true);
		SendClientEvent(level, category, message);
	}

	static void ClientState(int level, string category, string key, string value, string message)
	{
		if (!CanSendClientEvent(level)) return;
		EnsureStorage();
		string storageKey = MakeKey(category, key);
		string previous;
		if (s_ClientStates.Find(storageKey, previous) && previous == value) return;
		s_ClientStates.Set(storageKey, value);
		SendClientEvent(level, category, message + " state=" + value);
	}

	static void ClientRateLimited(int level, string category, string key, int intervalMs, string message)
	{
		if (!CanSendClientEvent(level)) return;
		EnsureStorage();
		int now = Now();
		string storageKey = MakeKey(category, key);
		int previous;
		if (s_ClientRateTimes.Find(storageKey, previous) && now - previous < Math.Max(intervalMs, 1)) return;
		s_ClientRateTimes.Set(storageKey, now);
		SendClientEvent(level, category, message);
	}

	static void ClientCount(int level, string category, string key, int flushIntervalMs = 10000, int amount = 1)
	{
		if (!CanSendClientEvent(level)) return;
		EnsureStorage();
		int now = Now();
		string storageKey = MakeKey(category, key);
		int count;
		s_ClientCounters.Find(storageKey, count);
		count += amount;
		s_ClientCounters.Set(storageKey, count);

		int started;
		if (!s_ClientCounterTimes.Find(storageKey, started))
		{
			s_ClientCounterTimes.Set(storageKey, now);
			return;
		}
		if (now - started < Math.Max(flushIntervalMs, 1)) return;

		s_ClientCounters.Set(storageKey, 0);
		s_ClientCounterTimes.Set(storageKey, now);
		SendClientEvent(level, category, key + " count=" + count.ToString() + " intervalMs=" + (now - started).ToString());
	}

	static void ReceiveClientEvent(PlayerIdentity sender, int level, string category, string message)
	{
		if (!sender || !Enabled(level) || !IsServerContext()) return;
		if (level < 1 || level > 10) return;
		if (!GGUtil.IsSafeIdentifier(category) || category.Length() > 32) return;
		if (!IsClientCategoryAllowed(category)) return;
		if (message == "" || message.Length() > 512) return;
		if (message.IndexOf("\n") != -1 || message.IndexOf("\r") != -1) return;

		EnsureStorage();
		string senderId = sender.GetId();
		if (senderId == "") return;
		string rateKey = senderId + "|" + category;
		int now = Now();
		int previous;
		if (s_ServerClientRateTimes.Find(rateKey, previous) && now - previous < 200) return;
		s_ServerClientRateTimes.Set(rateKey, now);
		string output = "[Generic Gunplay DEBUG L" + level.ToString();
		output += "][CLIENT " + sender.GetName();
		output += "][" + category + "] ";
		output += message;
		Print(output);
	}

	protected static void AccumulateServerCount(int level, string category, string key, int flushIntervalMs, int amount)
	{
		int now = Now();
		string storageKey = MakeKey(category, key);
		int count;
		s_Counters.Find(storageKey, count);
		count += amount;
		s_Counters.Set(storageKey, count);

		int started;
		if (!s_CounterTimes.Find(storageKey, started))
		{
			s_CounterTimes.Set(storageKey, now);
			return;
		}
		if (now - started < Math.Max(flushIntervalMs, 1)) return;

		s_Counters.Set(storageKey, 0);
		s_CounterTimes.Set(storageKey, now);
		WriteServer(level, category, key + " count=" + count.ToString() + " intervalMs=" + (now - started).ToString());
	}

	protected static void SendClientEvent(int level, string category, string message)
	{
		if (!CanSendClientEvent(level)) return;
		if (!GGUtil.IsSafeIdentifier(category) || category.Length() > 32) return;
		if (message == "") return;
		if (message.Length() > 512) message = message.Substring(0, 512);
		Param3<int, string, string> eventData = new Param3<int, string, string>(level, category, message);
		g_Game.RPCSingleParam(null, GGConstants.RPC_DEBUG_EVENT, eventData, true, null);
	}

	protected static bool CanSendClientEvent(int level)
	{
		if (!s_ClientReportingActive || !Enabled(level) || !g_Game) return false;
		if (g_Game.IsDedicatedServer() || g_Game.IsServer()) return false;
		return GGNetworkSync.IsClientReady();
	}

	protected static bool IsClientCategoryAllowed(string category)
	{
		if (category == "STATS" || category == "SYNC" || category == "CROSSHAIR") return true;
		if (category == "CAMERA" || category == "LASER" || category == "UI") return true;
		if (category == "ADS" || category == "HIPFIRE" || category == "HOLD_BREATH") return true;
		if (category == "MODIFIERS" || category == "ATTACHMENT" || category == "WEAPON") return true;
		if (category == "CACHE" || category == "POLICY" || category == "OPTICS") return true;
		return false;
	}

	protected static bool IsServerContext()
	{
		return g_Game && g_Game.IsDedicatedServer();
	}

	protected static int Now()
	{
		if (!g_Game) return 0;
		return g_Game.GetTime();
	}

	protected static string MakeKey(string category, string key)
	{
		return category + "|" + key;
	}

	protected static void WriteServer(int level, string category, string message)
	{
		Print("[Generic Gunplay DEBUG L" + level.ToString() + "][SERVER][" + category + "] " + message);
	}

	protected static void EnsureStorage()
	{
		if (!s_Once) s_Once = new map<string, bool>;
		if (!s_States) s_States = new map<string, string>;
		if (!s_RateTimes) s_RateTimes = new map<string, int>;
		if (!s_Counters) s_Counters = new map<string, int>;
		if (!s_CounterTimes) s_CounterTimes = new map<string, int>;
		if (!s_ClientOnce) s_ClientOnce = new map<string, bool>;
		if (!s_ClientStates) s_ClientStates = new map<string, string>;
		if (!s_ClientRateTimes) s_ClientRateTimes = new map<string, int>;
		if (!s_ClientCounters) s_ClientCounters = new map<string, int>;
		if (!s_ClientCounterTimes) s_ClientCounterTimes = new map<string, int>;
		if (!s_ServerClientRateTimes) s_ServerClientRateTimes = new map<string, int>;
	}

	protected static void ClearTransientState()
	{
		s_Once.Clear();
		s_States.Clear();
		s_RateTimes.Clear();
		s_Counters.Clear();
		s_CounterTimes.Clear();
		s_ClientOnce.Clear();
		s_ClientStates.Clear();
		s_ClientRateTimes.Clear();
		s_ClientCounters.Clear();
		s_ClientCounterTimes.Clear();
		s_ServerClientRateTimes.Clear();
	}
}
