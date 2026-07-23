modded class OptionsMenu
{
	protected ref GGOptionsMenuGunplay m_GGGunplayTab;

	void ~OptionsMenu()
	{
		if (m_GGGunplayTab)
			delete m_GGGunplayTab;
	}

	override Widget Init()
	{
		layoutRoot = super.Init();
		int tabIndex = m_Tabber.AddTab("#STR_GG_TAB");
		Widget tabRoot = layoutRoot.FindAnyWidget("Tab_" + tabIndex);
		if (tabRoot)
			m_GGGunplayTab = new GGOptionsMenuGunplay(tabRoot, m_Details, m_Options, this);
		return layoutRoot;
	}

	override void OnChanged()
	{
		super.OnChanged();
		if (m_GGGunplayTab && m_GGGunplayTab.IsChanged())
		{
			m_Apply.ClearFlags(WidgetFlags.IGNOREPOINTER);
			ColorNormal(m_Apply);
		}
	}

	override void Apply()
	{
		super.Apply();
		if (m_GGGunplayTab && m_GGGunplayTab.IsChanged())
			m_GGGunplayTab.Apply();
	}
}
