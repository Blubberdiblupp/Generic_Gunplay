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
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.EnableTooltipStats || !item)
		{
			m_GGTooltipPanel.Hide();
			return;
		}
		m_GGTooltipPanel.Apply(GGDisplayStats.GetDisplay(item));
	}
}
