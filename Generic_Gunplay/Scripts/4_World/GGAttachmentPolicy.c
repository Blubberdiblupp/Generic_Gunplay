class GGAttachmentPolicyRuntime
{
	static Weapon_Base FindOwningWeapon(EntityAI parent)
	{
		for (int depth = 0; parent && depth < 32; depth++)
		{
			Weapon_Base weapon = Weapon_Base.Cast(parent);
			if (weapon) return weapon;
			parent = parent.GetHierarchyParent();
		}
		return null;
	}

	static bool Allows(EntityAI parent, EntityAI attachment)
	{
		if (!parent || !attachment) return true;
		Weapon_Base weapon = FindOwningWeapon(parent);
		if (!weapon) return true;
		return AllowsAttachmentTree(weapon, attachment, 0);
	}

	protected static bool AllowsAttachmentTree(Weapon_Base weapon, EntityAI attachment, int depth)
	{
		if (!weapon || !attachment) return true;
		if (!GetGGConfigManager().IsAttachmentAllowed(weapon.GetType(), attachment.GetType()))
		{
			string key = GGUtil.Key(weapon.GetType()) + "_" + GGUtil.Key(attachment.GetType());
			string message = "Blocked attachment placement. weapon=" + weapon.GetType() + " attachment=" + attachment.GetType();
			GGDebug.Once(8, "POLICY", key, message);
			GGDebug.ClientOnce(8, "POLICY", key, message);
			return false;
		}
		if (depth >= 32) return true;
		int childCount = attachment.GetInventory().AttachmentCount();
		for (int childIndex = 0; childIndex < childCount; childIndex++)
		{
			EntityAI child = attachment.GetInventory().GetAttachmentFromIndex(childIndex);
			if (!AllowsAttachmentTree(weapon, child, depth + 1)) return false;
		}
		return true;
	}
}

modded class ItemBase
{
	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (!super.CanReceiveAttachment(attachment, slotId)) return false;
		return GGAttachmentPolicyRuntime.Allows(this, attachment);
	}

	override bool CanPutAsAttachment(EntityAI parent)
	{
		if (!super.CanPutAsAttachment(parent)) return false;
		return GGAttachmentPolicyRuntime.Allows(parent, this);
	}
}
