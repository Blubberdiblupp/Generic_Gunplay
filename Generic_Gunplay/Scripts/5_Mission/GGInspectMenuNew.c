modded class InspectMenuNew
{
	protected Widget m_GGInspectRoot;
	protected ref GGStatsPanel m_GGInspectPanel;

	override void SetItem(EntityAI item)
	{
		super.SetItem(item);
		EnsureGGInspectPanel();
		if (!m_GGInspectPanel) return;
		if (!GetGGConfigManager().ShouldShowInspectStats() || !item)
		{
			m_GGInspectPanel.Hide();
			return;
		}
		bool applied = m_GGInspectPanel.Apply(GGDisplayStats.GetDisplay(item));
		if (GGDebug.Enabled(8))
		{
			string shown = GGDebug.BoolString(applied);
			string message = "Inspect stat panel updated. item=" + item.GetType();
			message += " shown=" + shown;
			GGDebug.ClientState(8, "UI", "inspect", item.GetType() + "|" + shown, message);
		}
	}

	protected void EnsureGGInspectPanel()
	{
		if (m_GGInspectPanel || !layoutRoot) return;
		Widget parent = layoutRoot.FindAnyWidget("ItemFrameWidgetPanel");
		if (!parent) parent = layoutRoot;
		m_GGInspectRoot = GetGame().GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGItemStatsInspect.layout", parent);
		if (m_GGInspectRoot) m_GGInspectPanel = new GGStatsPanel(m_GGInspectRoot);
	}
}
