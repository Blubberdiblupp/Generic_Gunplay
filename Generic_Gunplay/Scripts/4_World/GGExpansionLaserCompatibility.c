#ifdef EXPANSIONMODWEAPONS
modded class PlayerBase
{
	protected ref map<string, bool> m_GGAJLaserClassCache;
	protected ItemBase m_GGAJLaserCachedHandsItem;
	protected int m_GGAJLaserCachedTreeRevision = -1;
	protected bool m_GGAJLaserCachedTreeResult;

	override void EOnFrame(IEntity other, float timeSlice)
	{
		ItemBase item = GetItemInHands();
		if (item && GGHandsItemContainsAJLaser(item))
		{
			if (GGDebug.Enabled(10))
			{
				GGDebug.Count(10, "LASER", "aj_expansion_frame_bypasses", 10000);
				GGDebug.ClientCount(10, "LASER", "aj_expansion_frame_bypasses", 10000);
			}
			if (GGDebug.Enabled(7))
				GGDebug.ClientState(7, "LASER", "aj_expansion_guard", item.GetType(), "Expansion laser frame handler bypassed for AJ laser compatibility");
			return;
		}
		super.EOnFrame(other, timeSlice);
	}

	protected bool GGHandsItemContainsAJLaser(ItemBase item)
	{
		if (!item) return false;
		Weapon_Base weapon = Weapon_Base.Cast(item);
		if (!weapon) return GGContainsAJLaserBox(item);
		int treeRevision = weapon.GetGGAttachmentTreeRevision();

		bool sameItem = m_GGAJLaserCachedHandsItem == item;
		bool sameRevision = m_GGAJLaserCachedTreeRevision == treeRevision;
		if (sameItem && sameRevision) return m_GGAJLaserCachedTreeResult;

		m_GGAJLaserCachedHandsItem = item;
		m_GGAJLaserCachedTreeRevision = treeRevision;
		m_GGAJLaserCachedTreeResult = GGContainsAJLaserBox(item);
		GGDebug.ClientState(8, "LASER", "aj_tree_scan", item.GetType() + "|" + treeRevision.ToString(), "AJ laser attachment tree rescanned");
		return m_GGAJLaserCachedTreeResult;
	}

	protected bool GGContainsAJLaserBox(EntityAI root, int depth = 0)
	{
		if (!root || depth > 8) return false;
		ItemBase item = ItemBase.Cast(root);
		if (item && GGIsAJLaserBox(item)) return true;
		if (!root.GetInventory()) return false;

		for (int i = 0; i < root.GetInventory().AttachmentCount(); i++)
		{
			EntityAI attachment = root.GetInventory().GetAttachmentFromIndex(i);
			if (GGContainsAJLaserBox(attachment, depth + 1)) return true;
		}
		return false;
	}

	protected bool GGIsAJLaserBox(ItemBase item)
	{
		if (!item || !g_Game) return false;
		if (!m_GGAJLaserClassCache) m_GGAJLaserClassCache = new map<string, bool>;
		string itemKey = GGUtil.Key(item.GetType());
		bool cachedResult;
		if (m_GGAJLaserClassCache.Find(itemKey, cachedResult)) return cachedResult;

		string current = item.GetType();
		for (int depth = 0; depth < 32 && current != ""; depth++)
		{
			if (GGUtil.Key(current) == "ajw_laserbox_base")
			{
				m_GGAJLaserClassCache.Set(itemKey, true);
				GGDebug.Once(8, "LASER", "aj_class_" + itemKey, "Detected AJ laser class " + item.GetType());
				GGDebug.ClientOnce(8, "LASER", "aj_class_" + itemKey, "Detected AJ laser class " + item.GetType());
				return true;
			}
			string parent;
			if (!g_Game.ConfigGetBaseName("CfgVehicles " + current, parent) || parent == current) break;
			current = parent;
		}
		m_GGAJLaserClassCache.Set(itemKey, false);
		return false;
	}
}
#endif
