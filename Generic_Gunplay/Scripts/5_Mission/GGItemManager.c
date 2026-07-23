modded class ItemManager
{
	protected Widget m_GGTooltipRoot;
	protected ref GGStatsPanel m_GGTooltipPanel;

	void ItemManager(Widget root)
	{
		Widget parent;
		if (root) parent = root.FindAnyWidget("GridSpacerWidget1");
		if (!parent) return;
		m_GGTooltipRoot = GetGame().GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGItemStatsTooltip.layout", parent);
		if (m_GGTooltipRoot) m_GGTooltipPanel = new GGStatsPanel(m_GGTooltipRoot);
	}

	override void PrepareTooltip(EntityAI item, int x = 0, int y = 0)
	{
		super.PrepareTooltip(item, x, y);
		if (!m_GGTooltipPanel) return;
		if (!GetGGConfigManager().ShouldShowTooltipStats() || !item)
		{
			m_GGTooltipPanel.Hide();
			return;
		}
		bool applied = m_GGTooltipPanel.Apply(GGDisplayStats.GetDisplay(item));
		if (GGDebug.Enabled(8))
		{
			string shown = GGDebug.BoolString(applied);
			string message = "Tooltip stat panel updated. item=" + item.GetType();
			message += " shown=" + shown;
			GGDebug.ClientState(8, "UI", "tooltip", item.GetType() + "|" + shown, message);
		}
	}
}
