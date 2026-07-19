modded class DayZPlayerCameraOptics
{
	override void OnActivate(DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult)
	{
		super.OnActivate(pPrevCamera, pPrevCameraResult);
		PlayerBase player = PlayerBase.Cast(m_pPlayer);
		if (!player || !m_opticsUsed) return;
		Weapon_Base weapon = Weapon_Base.Cast(player.GetItemInHands());
		if (!weapon || !weapon.GGShouldApplyGunplay()) return;

		int delay = weapon.GetGGOpticEnterDelay();
		int ppDelay = Math.Max(0, delay - 100);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(player.HideClothing);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(SetCameraPP);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(SetCameraPP, ppDelay, false, true, this);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(player.HideClothing, delay, false, m_opticsUsed, true);
	}
}
