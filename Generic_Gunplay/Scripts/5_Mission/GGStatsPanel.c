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

		SetText("title_value", data.Title, 0xFFFFFFFF);
		for (int i = 0; i < 8; i++)
		{
			string rowName = "line" + (i + 1).ToString();
			Widget row = m_Root.FindAnyWidget(rowName);
			if (i < data.Lines.Count() && data.Lines[i])
			{
				if (row) row.Show(true);
				SetText(rowName + "_label", data.Lines[i].Label, 0xFFAF9442);
				SetText(rowName + "_value", data.Lines[i].Value, data.Lines[i].Color);
			}
			else if (row)
			{
				row.Show(false);
			}
		}
		float width;
		float height;
		m_Root.GetSize(width, height);
		if (width >= 540.0) height = 42.0 + (data.Lines.Count() * 23.0);
		else height = 38.0 + (data.Lines.Count() * 20.0);
		m_Root.SetSize(width, height);

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
}
