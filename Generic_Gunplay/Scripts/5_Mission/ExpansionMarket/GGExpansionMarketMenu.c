#ifdef EXPANSIONMODMARKET
modded class ExpansionMarketMenu
{
	protected Widget m_GGMarketRoot;
	protected ref GGStatsPanel m_GGMarketPanel;

	void ExpansionMarketMenu()
	{
		EnsureGGMarketPanel();
	}

	override void UpdatePreview()
	{
		super.UpdatePreview();
		EnsureGGMarketPanel();
		if (!m_GGMarketPanel) return;
		if (!GetGGConfigManager().ShouldShowExpansionMarketStats())
		{
			m_GGMarketPanel.Hide();
			return;
		}
		string itemType = GetGGMarketItemType();
		bool applied = m_GGMarketPanel.Apply(GGDisplayStats.GetDisplay(EntityAI.Cast(m_CurrentPreviewObject), itemType, GetGGMarketAttachmentTypes()));
		if (GGDebug.Enabled(8))
		{
			string shown = GGDebug.BoolString(applied);
			string message = "Expansion Market stat panel updated. item=" + itemType;
			message += " shown=" + shown;
			GGDebug.ClientState(8, "UI", "expansion_market", itemType + "|" + shown, message);
		}
	}

	protected string GetGGMarketItemType()
	{
		if (m_CurrentPreviewObject) return m_CurrentPreviewObject.GetType();
		ExpansionMarketItem marketItem = GetSelectedMarketItem();
		if (marketItem) return marketItem.ClassName;
		return "";
	}

	protected array<string> GetGGMarketAttachmentTypes()
	{
		array<string> attachmentTypes = new array<string>;
		ExpansionMarketItem marketItem = GetSelectedMarketItem();
		if (!marketItem || !marketItem.SpawnAttachments) return attachmentTypes;
		foreach (string attachmentType : marketItem.SpawnAttachments)
			if (attachmentType != "") attachmentTypes.Insert(attachmentType);
		return attachmentTypes;
	}

	protected void EnsureGGMarketPanel()
	{
		if (m_GGMarketPanel || !m_LayoutRoot) return;
		Widget parent = m_LayoutRoot.FindAnyWidget("market_item_description_container");
		if (!parent) parent = m_LayoutRoot;
		m_GGMarketRoot = GetGame().GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGExpansionMarketStats.layout", parent);
		if (m_GGMarketRoot) m_GGMarketPanel = new GGStatsPanel(m_GGMarketRoot);
	}
}
#endif
