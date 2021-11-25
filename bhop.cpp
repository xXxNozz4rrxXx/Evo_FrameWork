#include "bhop.hpp"
#include "../options.hpp"

#include "../valve_sdk/csgostructs.hpp"

void Bhop::OnCreateMove(CUserCmd* cmd)
{
  RunKnifeBot(cmd);
  if (!g_Options.misc_bhop)
	  return;
  static bool jumped_last_tick = false;
  static bool should_fake_jump = false;
  if(!jumped_last_tick && should_fake_jump) {
    should_fake_jump = false;
    cmd->buttons |= IN_JUMP;
  } else if(cmd->buttons & IN_JUMP) {
    if(g_LocalPlayer->m_fFlags() & FL_ONGROUND) {
      jumped_last_tick = true;
      should_fake_jump = true;
    } else {
      cmd->buttons &= ~IN_JUMP;
      jumped_last_tick = false;
    }
  } else {
    jumped_last_tick = false;
    should_fake_jump = false;
  }
  if (!(g_LocalPlayer->m_fFlags() & FL_ONGROUND) && g_Options.misc_autostrafe)
	  if (cmd->mousedx > 1 || cmd->mousedx < -1)
		  cmd->sidemove = cmd->mousedx < 0.f ? -400.f : 400.f;
}
Vector m_nAngle;
bool Bhop::KnifeBestTarget() {
	float bestDist = 75;
	for (int i = 1; i < g_EngineClient->GetMaxClients(); i++)
	{
		auto pBaseEntity = C_BasePlayer::GetPlayerByIndex(i);
		if (!pBaseEntity)
			continue;
		if (!pBaseEntity->IsAlive() || pBaseEntity->IsDormant())
			continue;
		if (pBaseEntity == g_LocalPlayer)
			continue;
		if (pBaseEntity->m_iTeamNum() == g_LocalPlayer->m_iTeamNum())
			continue;

		Vector localPos = g_LocalPlayer->m_vecOrigin(); localPos.z += 50;
		Vector basePos = pBaseEntity->m_vecOrigin(); basePos.z += 50;
		if (!pBaseEntity->IsVisible(false))
			continue;

		float curDist = localPos.DistTo(basePos);
		if (curDist < bestDist)
		{
			bestDist = curDist;
			m_nBestIndex = i;
			pBestEntity = pBaseEntity;
		}
	}

	m_nBestDist = bestDist;
	return m_nBestIndex != -1;
}
void Bhop::RunKnifeBot(CUserCmd* cmd) {
	m_nBestIndex = -1;
	auto pLocalEntity = g_LocalPlayer;
	if (!pLocalEntity || !g_Options.misc_kb_enabled)
		return;

	int cw = G.WeaponID;
	if (cw == 0)
		return;

	if (!Utils::IsWeaponKnife(cw))
		return;

	if (!this->KnifeBestTarget())
		return;

	Vector tempLocalOrigin = pLocalEntity->m_vecOrigin();
	Vector tempBestOrigin = pBestEntity->m_vecOrigin();
	tempBestOrigin.z = tempLocalOrigin.z;
	QAngle entAng = Math::CalcAngle(tempLocalOrigin, tempBestOrigin);

	if (!lastAttacked)
	{
		bool stab = false;
		if (g_Options.misc_auto_kb)
		{
			int health = pBestEntity->m_iHealth();
			if (pBestEntity->m_ArmorValue())
			{
				if (health <= 55 &&
					health > 34)
					stab = true;
			}
			else
			{
				if (health <= 65 &&
					health > 40)
					stab = true;
			}

			stab = stab && m_nBestDist < 60;

			if (stab)
				cmd->buttons |= IN_ATTACK2;
			else
				cmd->buttons |= IN_ATTACK;
		}
		else
		{
			stab = cmd->buttons & IN_ATTACK2;
		}

		if (g_Options.misc_full_kb)
		{
			auto weap = pLocalEntity->m_hActiveWeapon().Get();
			float server_time = pLocalEntity->m_nTickBase() * g_GlobalVars->interval_per_tick;
			float next_shot = (stab ? weap->m_flNextSecondaryAttack() : weap->m_flNextPrimaryAttack()) - server_time;
			if (!(next_shot > 0) && (g_Options.misc_auto_kb || cmd->buttons & IN_ATTACK || cmd->buttons & IN_ATTACK2))
			{
				cmd->viewangles = entAng;
				G.SendPacket = false;
			}
		}
	}

	lastAttacked = !lastAttacked;
}