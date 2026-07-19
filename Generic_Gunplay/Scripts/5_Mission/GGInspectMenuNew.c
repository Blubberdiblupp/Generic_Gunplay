modded class InspectMenuNew
{
	protected Widget m_GGInspectRoot;
	protected ref GGStatsPanel m_GGInspectPanel;

	override void SetItem(EntityAI item)
	{
		super.SetItem(item);
		EnsureGGInspectPanel();
		if (!m_GGInspectPanel) return;
		GGSettings settings = GetGGConfigManager().GetSettings();
		if (!settings || !settings.EnableInspectStats || !item)
		{
			m_GGInspectPanel.Hide();
			return;
		}
		m_GGInspectPanel.Apply(GGDisplayStats.GetDisplay(item));
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
