class GGOptionsMenuGunplay : ScriptedWidgetEventHandler
{
	protected Widget m_Root;
	protected OptionsMenu m_Menu;
	protected bool m_IsChanged;
	protected bool m_CanEditTooltip;
	protected bool m_CanEditInspect;
	protected bool m_CanEditMarket;
	protected bool m_CanChooseCrosshair;
	protected bool m_CanChooseCrosshairColor;
	protected bool m_CanChooseCrosshairStyle;
	protected bool m_CanChooseCrosshairOpacity;
	protected bool m_UpdatingCrosshairAppearance;
	protected ref OptionSelectorMultistate m_ShowStatsSelector;
	protected ref OptionSelectorMultistate m_TooltipSelector;
	protected ref OptionSelectorMultistate m_InspectSelector;
	protected ref OptionSelectorMultistate m_MarketSelector;
	protected ref OptionSelectorMultistate m_CrosshairSelector;
	protected ref OptionSelectorMultistate m_CrosshairColorSelector;
	protected ref OptionSelectorMultistate m_CrosshairStyleSelector;
	protected ref OptionSelectorMultistate m_CrosshairOpacitySelector;
	protected ref OptionSelectorMultistate m_RecoilSelector;
	protected ref OptionSelectorMultistate m_SwaySelector;
	protected ref OptionSelectorMultistate m_ADSSelector;
	protected ref OptionSelectorMultistate m_PrecisionSelector;
	protected ref OptionSelectorMultistate m_DispersionSelector;
	protected ref OptionSelectorMultistate m_HipFireSelector;
	protected ref OptionSelectorMultistate m_RPMSelector;
	protected ref OptionSelectorMultistate m_MuzzleVelocitySelector;
	protected ref OptionSelectorMultistate m_MagazineCapacitySelector;
	protected ref OptionSelectorMultistate m_AmmoBallisticsSelector;
	protected ref OptionSelectorMultistate m_AmmoDamageSelector;
	protected ref OptionSelectorMultistate m_ArmorSelector;
	protected GGClientSettings m_ClientSettings;
	protected GGSettings m_ServerSettings;

	void GGOptionsMenuGunplay(Widget parent, Widget detailsRoot, GameOptions options, OptionsMenu menu)
	{
		m_Menu = menu;
		GGConfigManager manager = GetGGConfigManager();
		m_ClientSettings = manager.GetClientSettings();
		m_ServerSettings = manager.GetSettings();

		m_Root = g_Game.GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGOptionsTab.layout", parent);
		if (!m_Root || !m_ClientSettings || !m_ServerSettings) return;
		m_Root.SetHandler(this);

		Widget rootContent = m_Root.FindAnyWidget("gg_options_tab_content");
		GridSpacerWidget displayGrid = CreateCategory(rootContent, "#STR_GG_CATEGORY_STATS");
		m_ShowStatsSelector = AddSelector(displayGrid, "#STR_GG_SHOW_ALL_STATS", BoolIndex(m_ClientSettings.ShowStats), MakeBoolStates());
		if (m_ShowStatsSelector)
			m_ShowStatsSelector.m_OptionChanged.Insert(OnOptionChanged);

		m_CanEditTooltip = m_ServerSettings.EnableTooltipStats;
		m_TooltipSelector = AddServerBoundSelector(displayGrid, "#STR_GG_TOOLTIP_STATS", m_ClientSettings.ShowTooltipStats, m_CanEditTooltip);

		m_CanEditInspect = m_ServerSettings.EnableInspectStats;
		m_InspectSelector = AddServerBoundSelector(displayGrid, "#STR_GG_INSPECT_STATS", m_ClientSettings.ShowInspectStats, m_CanEditInspect);

		m_CanEditMarket = m_ServerSettings.EnableExpansionMarketStats;
		m_MarketSelector = AddServerBoundSelector(displayGrid, "#STR_GG_MARKET_STATS", m_ClientSettings.ShowExpansionMarketStats, m_CanEditMarket);

		GGStatVisibility clientVisible = EnsureClientVisibleStats();
		GGStatVisibility serverVisible = m_ServerSettings.VisibleStats;
		if (!serverVisible) serverVisible = new GGStatVisibility();
		GridSpacerWidget visibleGrid = CreateCategory(rootContent, "#STR_GG_CATEGORY_VISIBLE_STATS");
		m_RecoilSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_RECOIL", clientVisible.Recoil, serverVisible.Recoil);
		if (serverVisible.Recoil && m_RecoilSelector) m_RecoilSelector.m_OptionChanged.Insert(OnRecoilChanged);
		m_SwaySelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_SWAY", clientVisible.Sway, serverVisible.Sway);
		if (serverVisible.Sway && m_SwaySelector) m_SwaySelector.m_OptionChanged.Insert(OnSwayChanged);
		m_ADSSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_ADS", clientVisible.ADS, serverVisible.ADS);
		if (serverVisible.ADS && m_ADSSelector) m_ADSSelector.m_OptionChanged.Insert(OnADSChanged);
		m_PrecisionSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_PRECISION", clientVisible.Precision, serverVisible.Precision);
		if (serverVisible.Precision && m_PrecisionSelector) m_PrecisionSelector.m_OptionChanged.Insert(OnPrecisionChanged);
		m_DispersionSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_DISPERSION", clientVisible.Dispersion, serverVisible.Dispersion);
		if (serverVisible.Dispersion && m_DispersionSelector) m_DispersionSelector.m_OptionChanged.Insert(OnDispersionChanged);
		m_HipFireSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_HIPFIRE", clientVisible.HipFire, serverVisible.HipFire);
		if (serverVisible.HipFire && m_HipFireSelector) m_HipFireSelector.m_OptionChanged.Insert(OnHipFireChanged);
		m_RPMSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_RPM", clientVisible.RPM, serverVisible.RPM);
		if (serverVisible.RPM && m_RPMSelector) m_RPMSelector.m_OptionChanged.Insert(OnRPMChanged);
		m_MuzzleVelocitySelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_MUZZLE_VELOCITY", clientVisible.MuzzleVelocity, serverVisible.MuzzleVelocity);
		if (serverVisible.MuzzleVelocity && m_MuzzleVelocitySelector) m_MuzzleVelocitySelector.m_OptionChanged.Insert(OnMuzzleVelocityChanged);
		m_MagazineCapacitySelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_MAGAZINE_CAPACITY", clientVisible.MagazineCapacity, serverVisible.MagazineCapacity);
		if (serverVisible.MagazineCapacity && m_MagazineCapacitySelector) m_MagazineCapacitySelector.m_OptionChanged.Insert(OnMagazineCapacityChanged);
		m_AmmoBallisticsSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_AMMO_BALLISTICS", clientVisible.AmmoBallistics, serverVisible.AmmoBallistics);
		if (serverVisible.AmmoBallistics && m_AmmoBallisticsSelector) m_AmmoBallisticsSelector.m_OptionChanged.Insert(OnAmmoBallisticsChanged);
		m_AmmoDamageSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_AMMO_DAMAGE", clientVisible.AmmoDamage, serverVisible.AmmoDamage);
		if (serverVisible.AmmoDamage && m_AmmoDamageSelector) m_AmmoDamageSelector.m_OptionChanged.Insert(OnAmmoDamageChanged);
		m_ArmorSelector = AddStatSelector(visibleGrid, "#STR_GG_STAT_ARMOR", clientVisible.Armor, serverVisible.Armor);
		if (serverVisible.Armor && m_ArmorSelector) m_ArmorSelector.m_OptionChanged.Insert(OnArmorChanged);

		GridSpacerWidget crosshairGrid = CreateCategory(rootContent, "#STR_GG_CATEGORY_CROSSHAIR");
		m_CanChooseCrosshair = GGNetworkSync.IsClientReady() && m_ServerSettings.AllowClientCrosshairChoice;
		if (m_CanChooseCrosshair)
		{
			m_CrosshairSelector = AddSelector(crosshairGrid, "#STR_GG_CROSSHAIR_MODE", Math.Clamp(m_ClientSettings.CrosshairMode, 0, 2), MakeCrosshairStates());
			if (m_CrosshairSelector)
				m_CrosshairSelector.m_OptionChanged.Insert(OnCrosshairModeChanged);
		}
		else
		{
			int serverMode = Math.Clamp(m_ServerSettings.CrosshairMode, 0, 2);
			TStringArray serverState = new TStringArray();
			if (!GGNetworkSync.IsClientReady())
				serverState.Insert("#STR_GG_WAITING_SERVER");
			else
				serverState.Insert(ServerCrosshairModeState(serverMode));
			m_CrosshairSelector = AddSelector(crosshairGrid, "#STR_GG_CROSSHAIR_MODE", 0, serverState);
		}

		TStringArray initialAppearanceState = MakeCrosshairAppearanceUnavailableStates();
		m_CrosshairColorSelector = AddSelector(crosshairGrid, "#STR_GG_CROSSHAIR_COLOR", 0, initialAppearanceState);
		m_CrosshairStyleSelector = AddSelector(crosshairGrid, "#STR_GG_CROSSHAIR_STYLE", 0, initialAppearanceState);
		m_CrosshairOpacitySelector = AddSelector(crosshairGrid, "#STR_GG_CROSSHAIR_OPACITY", 0, initialAppearanceState);
		if (m_CrosshairColorSelector)
			m_CrosshairColorSelector.m_OptionChanged.Insert(OnOptionChanged);
		if (m_CrosshairStyleSelector)
			m_CrosshairStyleSelector.m_OptionChanged.Insert(OnOptionChanged);
		if (m_CrosshairOpacitySelector)
			m_CrosshairOpacitySelector.m_OptionChanged.Insert(OnOptionChanged);
		RefreshCrosshairAppearanceSelectors();
	}

	bool IsChanged()
	{
		return m_IsChanged;
	}

	void Apply()
	{
		if (!m_ClientSettings) return;
		CommitClientSelections(true);
		m_IsChanged = false;
		GGNetworkSync.SendClientCrosshairPreference();
	}

	protected void CommitClientSelections(bool save)
	{
		if (!m_ClientSettings) return;
		if (m_ShowStatsSelector)
			m_ClientSettings.ShowStats = m_ShowStatsSelector.GetValue() == 1;
		if (m_CanEditTooltip && m_TooltipSelector)
			m_ClientSettings.ShowTooltipStats = m_TooltipSelector.GetValue() == 1;
		if (m_CanEditInspect && m_InspectSelector)
			m_ClientSettings.ShowInspectStats = m_InspectSelector.GetValue() == 1;
		if (m_CanEditMarket && m_MarketSelector)
			m_ClientSettings.ShowExpansionMarketStats = m_MarketSelector.GetValue() == 1;
		if (m_CanChooseCrosshair && m_CrosshairSelector)
			m_ClientSettings.CrosshairMode = Math.Clamp(m_CrosshairSelector.GetValue(), 0, 2);
		if (m_CanChooseCrosshairColor && m_CrosshairColorSelector)
			m_ClientSettings.CrosshairColor = Math.Clamp(m_CrosshairColorSelector.GetValue(), 0, 5);
		if (m_CanChooseCrosshairStyle && m_CrosshairStyleSelector)
			m_ClientSettings.CrosshairStyle = Math.Clamp(m_CrosshairStyleSelector.GetValue(), 0, 5);
		if (m_CanChooseCrosshairOpacity && m_CrosshairOpacitySelector)
			m_ClientSettings.CrosshairOpacity = Math.Clamp(m_CrosshairOpacitySelector.GetValue(), 0, 3);

		GGStatVisibility visible = EnsureClientVisibleStats();
		GGStatVisibility serverVisible = m_ServerSettings.VisibleStats;
		if (visible && serverVisible)
		{
			if (serverVisible.Recoil && m_RecoilSelector) visible.Recoil = m_RecoilSelector.GetValue() == 1;
			if (serverVisible.Sway && m_SwaySelector) visible.Sway = m_SwaySelector.GetValue() == 1;
			if (serverVisible.ADS && m_ADSSelector) visible.ADS = m_ADSSelector.GetValue() == 1;
			if (serverVisible.Precision && m_PrecisionSelector) visible.Precision = m_PrecisionSelector.GetValue() == 1;
			if (serverVisible.Dispersion && m_DispersionSelector) visible.Dispersion = m_DispersionSelector.GetValue() == 1;
			if (serverVisible.HipFire && m_HipFireSelector) visible.HipFire = m_HipFireSelector.GetValue() == 1;
			if (serverVisible.RPM && m_RPMSelector) visible.RPM = m_RPMSelector.GetValue() == 1;
			if (serverVisible.MuzzleVelocity && m_MuzzleVelocitySelector) visible.MuzzleVelocity = m_MuzzleVelocitySelector.GetValue() == 1;
			if (serverVisible.MagazineCapacity && m_MagazineCapacitySelector) visible.MagazineCapacity = m_MagazineCapacitySelector.GetValue() == 1;
			if (serverVisible.AmmoBallistics && m_AmmoBallisticsSelector) visible.AmmoBallistics = m_AmmoBallisticsSelector.GetValue() == 1;
			if (serverVisible.AmmoDamage && m_AmmoDamageSelector) visible.AmmoDamage = m_AmmoDamageSelector.GetValue() == 1;
			if (serverVisible.Armor && m_ArmorSelector) visible.Armor = m_ArmorSelector.GetValue() == 1;
		}

		if (save)
			GetGGConfigManager().SaveClientSettings();
	}

	protected GridSpacerWidget CreateCategory(Widget parent, string title)
	{
		Widget category = g_Game.GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGOptionsCategory.layout", parent);
		if (!category) return null;
		TextWidget header = TextWidget.Cast(category.FindAnyWidget("gg_options_category_header_text"));
		if (header) header.SetText(title);
		return GridSpacerWidget.Cast(category.FindAnyWidget("gg_options_category_content"));
	}

	protected OptionSelectorMultistate AddServerBoundSelector(GridSpacerWidget parent, string label, bool currentValue, bool enabledByServer)
	{
		OptionSelectorMultistate selector;
		if (enabledByServer)
		{
			selector = AddSelector(parent, label, BoolIndex(currentValue), MakeBoolStates());
			if (selector)
				selector.m_OptionChanged.Insert(OnOptionChanged);
			return selector;
		}
		TStringArray disabledState = new TStringArray();
		disabledState.Insert("#STR_GG_DISABLED_BY_SERVER");
		return AddSelector(parent, label, 0, disabledState);
	}

	protected OptionSelectorMultistate AddStatSelector(GridSpacerWidget parent, string label, bool currentValue, bool enabledByServer)
	{
		if (enabledByServer)
			return AddSelector(parent, label, BoolIndex(currentValue), MakeBoolStates());
		TStringArray disabledState = new TStringArray();
		disabledState.Insert("#STR_GG_DISABLED_BY_SERVER");
		return AddSelector(parent, label, 0, disabledState);
	}

	protected GGStatVisibility EnsureClientVisibleStats()
	{
		if (!m_ClientSettings) return null;
		if (!m_ClientSettings.VisibleStats)
			m_ClientSettings.VisibleStats = new GGStatVisibility();
		return m_ClientSettings.VisibleStats;
	}

	protected OptionSelectorMultistate AddSelector(GridSpacerWidget parent, string label, int selection, TStringArray states)
	{
		if (!parent) return null;
		Widget root = g_Game.GetWorkspace().CreateWidgets("Generic_Gunplay/GUI/layouts/GGOptionsSetting.layout", parent);
		if (!root) return null;
		TextWidget labelWidget = TextWidget.Cast(root.FindAnyWidget("gg_options_setting_label"));
		if (labelWidget) labelWidget.SetText(label);
		Widget optionWidget = root.FindAnyWidget("gg_options_setting_option");
		return new OptionSelectorMultistate(optionWidget, selection, this, false, states);
	}

	protected void OnRecoilChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.Recoil = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnSwayChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.Sway = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnADSChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.ADS = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnPrecisionChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.Precision = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnDispersionChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.Dispersion = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnHipFireChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.HipFire = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnRPMChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.RPM = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnMuzzleVelocityChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.MuzzleVelocity = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnMagazineCapacityChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.MagazineCapacity = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnAmmoBallisticsChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.AmmoBallistics = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnAmmoDamageChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.AmmoDamage = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnArmorChanged(int value)
	{
		GGStatVisibility visible = EnsureClientVisibleStats();
		if (!visible) return;
		visible.Armor = value == 1;
		SaveAndMarkChanged();
	}

	protected void OnOptionChanged(int value)
	{
		if (m_UpdatingCrosshairAppearance) return;
		CommitClientSelections(true);
		MarkChanged();
	}

	protected void OnCrosshairModeChanged(int value)
	{
		if (!m_CanChooseCrosshair || !m_ClientSettings) return;
		CommitClientSelections(true);
		RefreshCrosshairAppearanceSelectors();
		GGNetworkSync.SendClientCrosshairPreference();
		MarkChanged();
	}

	protected void RefreshCrosshairAppearanceSelectors()
	{
		int mode = GetDisplayedCrosshairMode();
		bool canCustomize = GGNetworkSync.IsClientReady() && m_ServerSettings && m_ServerSettings.EnableHipFireAlignment && mode != 0;
		m_CanChooseCrosshairColor = canCustomize;
		m_CanChooseCrosshairStyle = canCustomize;
		m_CanChooseCrosshairOpacity = canCustomize;

		m_UpdatingCrosshairAppearance = true;
		if (canCustomize)
		{
			if (m_CrosshairColorSelector)
			{
				m_CrosshairColorSelector.SetCanSwitch(true);
				m_CrosshairColorSelector.LoadNewValues(MakeCrosshairColorStates(), Math.Clamp(m_ClientSettings.CrosshairColor, 0, 5));
			}
			if (m_CrosshairStyleSelector)
			{
				m_CrosshairStyleSelector.SetCanSwitch(true);
				m_CrosshairStyleSelector.LoadNewValues(MakeCrosshairStyleStates(), Math.Clamp(m_ClientSettings.CrosshairStyle, 0, 5));
			}
			if (m_CrosshairOpacitySelector)
			{
				m_CrosshairOpacitySelector.SetCanSwitch(true);
				m_CrosshairOpacitySelector.LoadNewValues(MakeCrosshairOpacityStates(), Math.Clamp(m_ClientSettings.CrosshairOpacity, 0, 3));
			}
		}
		else
		{
			TStringArray unavailableStates = MakeCrosshairAppearanceUnavailableStates();
			if (m_CrosshairColorSelector)
			{
				m_CrosshairColorSelector.SetCanSwitch(false);
				m_CrosshairColorSelector.LoadNewValues(unavailableStates, 0);
			}
			if (m_CrosshairStyleSelector)
			{
				m_CrosshairStyleSelector.SetCanSwitch(false);
				m_CrosshairStyleSelector.LoadNewValues(unavailableStates, 0);
			}
			if (m_CrosshairOpacitySelector)
			{
				m_CrosshairOpacitySelector.SetCanSwitch(false);
				m_CrosshairOpacitySelector.LoadNewValues(unavailableStates, 0);
			}
		}
		m_UpdatingCrosshairAppearance = false;
	}

	protected int GetDisplayedCrosshairMode()
	{
		if (!m_ServerSettings) return 0;
		if (m_ServerSettings.AllowClientCrosshairChoice && m_ClientSettings)
			return Math.Clamp(m_ClientSettings.CrosshairMode, 0, 2);
		return Math.Clamp(m_ServerSettings.CrosshairMode, 0, 2);
	}

	protected TStringArray MakeCrosshairAppearanceUnavailableStates()
	{
		TStringArray states = new TStringArray();
		if (!GGNetworkSync.IsClientReady())
			states.Insert("#STR_GG_WAITING_SERVER");
		else if (GetDisplayedCrosshairMode() == 0)
			states.Insert("#STR_GG_NOT_ADJUSTABLE_WITH_VANILLA");
		else
			states.Insert("#STR_GG_DISABLED_BY_SERVER");
		return states;
	}

	protected void SaveAndMarkChanged()
	{
		GetGGConfigManager().SaveClientSettings();
		MarkChanged();
	}

	protected void MarkChanged()
	{
		m_IsChanged = true;
		if (m_Menu)
			m_Menu.OnChanged();
	}

	protected int BoolIndex(bool value)
	{
		if (value) return 1;
		return 0;
	}

	protected TStringArray MakeBoolStates()
	{
		TStringArray states = new TStringArray();
		states.Insert("#STR_GG_OFF");
		states.Insert("#STR_GG_ON");
		return states;
	}

	protected TStringArray MakeCrosshairStates()
	{
		TStringArray states = new TStringArray();
		states.Insert("#STR_GG_CROSSHAIR_VANILLA");
		states.Insert("#STR_GG_CROSSHAIR_WEAPON");
		states.Insert("#STR_GG_CROSSHAIR_LASER");
		return states;
	}

	protected TStringArray MakeCrosshairColorStates()
	{
		TStringArray states = new TStringArray();
		states.Insert("#STR_GG_COLOR_WHITE");
		states.Insert("#STR_GG_COLOR_RED");
		states.Insert("#STR_GG_COLOR_GREEN");
		states.Insert("#STR_GG_COLOR_CYAN");
		states.Insert("#STR_GG_COLOR_YELLOW");
		states.Insert("#STR_GG_COLOR_ORANGE");
		return states;
	}

	protected TStringArray MakeCrosshairStyleStates()
	{
		TStringArray states = new TStringArray();
		states.Insert("#STR_GG_STYLE_CLASSIC");
		states.Insert("#STR_GG_STYLE_DOT");
		states.Insert("#STR_GG_STYLE_OPEN_CROSS");
		states.Insert("#STR_GG_STYLE_TACTICAL_T");
		states.Insert("#STR_GG_STYLE_BRACKETS");
		states.Insert("#STR_GG_STYLE_CROSS_DOT");
		return states;
	}

	protected TStringArray MakeCrosshairOpacityStates()
	{
		TStringArray states = new TStringArray();
		states.Insert("#STR_GG_OPACITY_25");
		states.Insert("#STR_GG_OPACITY_50");
		states.Insert("#STR_GG_OPACITY_75");
		states.Insert("#STR_GG_OPACITY_100");
		return states;
	}

	protected string ServerCrosshairModeState(int mode)
	{
		if (mode == 1) return "#STR_GG_SERVER_MODE_WEAPON";
		if (mode == 2) return "#STR_GG_SERVER_MODE_LASER";
		return "#STR_GG_SERVER_MODE_VANILLA";
	}
}
