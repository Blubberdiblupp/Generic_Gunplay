modded class DayZPlayerCameraBase
{
	override void StdFovUpdate(float pDt, out DayZPlayerCameraResult pOutResult)
	{
		super.StdFovUpdate(pDt, pOutResult);

		DayZPlayerImplement player = DayZPlayerImplement.Cast(m_pPlayer);
		if (!player) return;

		int shootingOriginState = player.GGGetCameraShootingOriginState();
		if (shootingOriginState == 1)
			pOutResult.m_fShootFromCamera = 1.0;
		else if (shootingOriginState == 0)
			pOutResult.m_fShootFromCamera = 0.0;

		if (GGDebug.Enabled(7))
		{
			string state = shootingOriginState.ToString() + "|" + pOutResult.m_fShootFromCamera.ToString();
			string message = "Final camera shooting-origin result. requested=" + shootingOriginState.ToString();
			message += " output=" + pOutResult.m_fShootFromCamera.ToString();
			GGDebug.ClientState(7, "CAMERA", "shoot_origin_result", state, message);
		}
		if (GGDebug.Enabled(9))
			GGDebug.ClientCount(9, "CAMERA", "std_fov_updates", 10000);
	}
}
