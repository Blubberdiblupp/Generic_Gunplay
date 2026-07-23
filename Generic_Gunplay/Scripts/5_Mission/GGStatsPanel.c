class GGStatsPanel : Managed
{
	protected Widget m_Root;

	void GGStatsPanel(Widget root)
	{
		m_Root = root;
		if (m_Root) m_Root.Show(false);
	}

	bool Apply(GGDisplayData data)
	{
		if (!m_Root) return false;
		if (!data || !data.Lines || data.Lines.Count() == 0)
		{
			m_Root.Show(false);
			return false;
		}

		GGStatVisibility clientVisible = GetClientVisibleStats();
		SetText("title_label", "Tier:", 0xFFAF9442);
		SetText("title_value", data.Title, 0xFFFFFFFF);
		int visibleLineCount;
		foreach (GGDisplayLine line : data.Lines)
		{
			if (!line) continue;
			bool primaryVisible = IsLineVisible(line.Stat, clientVisible);
			bool secondaryVisible = line.SecondaryStat != GGDisplayStat.NONE && IsLineVisible(line.SecondaryStat, clientVisible);
			if (!primaryVisible && !secondaryVisible) continue;
			if (visibleLineCount >= 8) break;

			string rowName = "line" + (visibleLineCount + 1).ToString();
			Widget row = m_Root.FindAnyWidget(rowName);
			if (row) row.Show(true);
			if (primaryVisible)
			{
				string displayValue = line.Value;
				if (secondaryVisible)
					displayValue += " | " + line.SecondaryLabel + " " + line.SecondaryValue;
				SetText(rowName + "_label", line.Label, 0xFFAF9442);
				SetText(rowName + "_value", displayValue, line.Color);
			}
			else
			{
				SetText(rowName + "_label", line.SecondaryLabel, 0xFFAF9442);
				SetText(rowName + "_value", line.SecondaryValue, line.SecondaryColor);
			}
			visibleLineCount++;
		}

		for (int i = visibleLineCount; i < 8; i++)
		{
			Widget unusedRow = m_Root.FindAnyWidget("line" + (i + 1).ToString());
			if (unusedRow) unusedRow.Show(false);
		}

		if (visibleLineCount == 0)
		{
			m_Root.Show(false);
			return false;
		}

		m_Root.Show(true);
		return true;
	}

	void Hide()
	{
		if (m_Root) m_Root.Show(false);
	}

	protected void SetText(string widgetName, string value, int color)
	{
		TextWidget widget = TextWidget.Cast(m_Root.FindAnyWidget(widgetName));
		if (!widget) return;
		widget.SetText(value);
		widget.SetColor(color);
	}

	protected GGStatVisibility GetClientVisibleStats()
	{
		GGClientSettings clientSettings = GetGGConfigManager().GetClientSettings();
		if (!clientSettings) return null;
		return clientSettings.VisibleStats;
	}

	protected bool IsLineVisible(int stat, GGStatVisibility clientVisible)
	{
		if (!clientVisible || stat == GGDisplayStat.NONE) return true;
		if (stat == GGDisplayStat.RECOIL) return clientVisible.Recoil;
		if (stat == GGDisplayStat.SWAY) return clientVisible.Sway;
		if (stat == GGDisplayStat.ADS) return clientVisible.ADS;
		if (stat == GGDisplayStat.PRECISION) return clientVisible.Precision;
		if (stat == GGDisplayStat.DISPERSION) return clientVisible.Dispersion;
		if (stat == GGDisplayStat.HIPFIRE) return clientVisible.HipFire;
		if (stat == GGDisplayStat.RPM) return clientVisible.RPM;
		if (stat == GGDisplayStat.MUZZLE_VELOCITY) return clientVisible.MuzzleVelocity;
		if (stat == GGDisplayStat.MAGAZINE_CAPACITY) return clientVisible.MagazineCapacity;
		if (stat == GGDisplayStat.AMMO_BALLISTICS) return clientVisible.AmmoBallistics;
		if (stat == GGDisplayStat.AMMO_DAMAGE) return clientVisible.AmmoDamage;
		if (stat == GGDisplayStat.ARMOR) return clientVisible.Armor;
		return true;
	}
}
