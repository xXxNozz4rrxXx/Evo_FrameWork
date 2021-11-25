#include "aimbot.hpp"

#include "../valve_sdk/sdk.hpp"

float Aimbot::GetFOV(Vector& viewangles, const Vector& vSrc, const Vector& vEnd)
{
	Vector delta = Utils::CalculateAngle(vSrc, vEnd) - viewangles;

	return sqrtf(powf(delta.x, 2.0f) + powf(delta.y, 2.0f));
}
Vector Aimbot::CurveSmooth(Vector& viewangles, Vector target, float factor)
{
	factor *= 10;

	if (factor <= 0.0f)
		return target;

	Vector delta = target - viewangles;


	float factorx = factor, factory = factor;

	if (delta.x < 0.0f)
	{
		if (factorx > fabs(delta.x))
			factorx = fabs(delta.x);

		target.x = viewangles.x - factorx;
	}
	else
	{
		if (factorx > delta.x)
			factorx = delta.x;

		target.x = viewangles.x + factorx;
	}

	if (delta.y < 0.0f)
	{
		if (factory > fabs(delta.y))
			factory = fabs(delta.y);

		target.y = viewangles.y - factory;
	}
	else
	{
		if (factory > delta.y)
			factory = delta.y;

		target.y = viewangles.y + factory;
	}
	return target;
}
Vector Aimbot::ClosestBone(C_BasePlayer *Entity, CUserCmd* cmd, Vector Rcs)
{
	Vector Aim = Vector(0.0f, 0.0f, 0.0f);
	auto pLocal = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));
	if (!pLocal || !Entity)
		return Aim;

	auto BoneIDByMenu = [](int MenuVar)
	{
		switch (MenuVar)
		{
		case 0:return 8; break;
		case 1:return 2; break;
		case 2:return 7; break;
		case 3:return 6; break;
		case 4:return 5; break;
		case 5:return 4; break;
		case 6:return 3; break;
		case 7:return 0; break;
		default:return MenuVar; break;
		}
	};
	if (weapons[G.WeaponID].Nearest)
	{
		float BestDist = 180.f;

		matrix3x4_t matrix[128];

		if (!Entity->SetupBones(matrix, 128, 0x00000100, 0.0f))
			return Aim;

		studiohdr_t* pStudioModel = g_MdlInfo->GetStudiomodel(Entity->GetModel());
		if (!pStudioModel)
			return Aim;

		mstudiohitboxset_t* set = pStudioModel->GetHitboxSet(Entity->m_nHitboxSet());
		if (!set)
			return Aim;

		for (int i = 0; i < set->numhitboxes; i++)
		{
			/*	if (i == HITBOX_RIGHT_THIGH || i == HITBOX_LEFT_THIGH || i == HITBOX_RIGHT_CALF
			|| i == HITBOX_LEFT_CALF || i == HITBOX_RIGHT_FOOT || i == HITBOX_LEFT_FOOT
			|| i == HITBOX_RIGHT_HAND || i == HITBOX_LEFT_HAND || i == HITBOX_RIGHT_UPPER_ARM
			|| i == HITBOX_RIGHT_FOREARM || i == HITBOX_LEFT_UPPER_ARM || i == HITBOX_LEFT_FOREARM)
			continue;*/

			if (i >= HITBOX_MAX) continue;

			mstudiobbox_t* hitbox = set->GetHitbox(i);

			if (!hitbox)
				continue;



			Vector Check = Vector(matrix[hitbox->bone][0][3], matrix[hitbox->bone][1][3], matrix[hitbox->bone][2][3]);
			if (!Utils::IsVisibleVector(Entity, pLocal, Check))continue;


			float thisdist = 360.f;
			if (Rcs.x > -0.01f && Rcs.x < 0.01f &&
				Rcs.y > -0.01f && Rcs.y < 0.01f)
			{
				thisdist = GetFOV(Vector(cmd->viewangles.pitch, cmd->viewangles.yaw, cmd->viewangles.roll), pLocal->GetEyePos(), Check);

			}
			else
			{
				thisdist = GetFOV(Vector(cmd->viewangles.pitch, cmd->viewangles.yaw, cmd->viewangles.roll) + Rcs, pLocal->GetEyePos(), Check);
			}




			if (thisdist < BestDist)
			{
				BestDist = thisdist;
				Aim = Check;
			}
		}
		return Aim;
	}
	else
	{

		Vector bonepos = Entity->GetBonePos(BoneIDByMenu(weapons[G.WeaponID].Bone));
		if (Utils::IsVisibleVector(Entity, pLocal, bonepos))
		{
			return bonepos;
		}
		else
		{
			return Vector(0.f, 0.f, 0.f);
		}
	}
}
float Aimbot::GetServerTime()
{
	auto pLocal = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));
	if (!pLocal)
		return 0.228f;
	return (float)pLocal->m_nTickBase() * g_GlobalVars->interval_per_tick;
}
bool Aimbot::bullettime(C_BaseCombatWeapon* weapon)
{
	if (!weapon)
		return false;
	float flNextPrimaryAttack = weapon->m_flNextPrimaryAttack();

	auto ServerTime = GetServerTime();
	if (ServerTime == 0.228f)
		return false;

	return flNextPrimaryAttack <= GetServerTime();
}
bool Aimbot::Shooting(CUserCmd* pCmd, C_BaseCombatWeapon* weapon)
{
	if (!weapon) return false;

	if (pCmd->buttons & IN_ATTACK && bullettime(weapon))
	{
		return true;
	}
	return false;
}
int Aimbot::FindTarget(CUserCmd* pCmd, Vector Rcs, Vector& NextBonePos)
{
	float flMax = 180.f;
	int flHeathMax = 150;
	int iMaxSpeed = 1000;
	int iBestTarget = -1;
	auto pLocal = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));
	if (!pLocal)
		return iBestTarget;


	if (pLocal && pLocal->m_iHealth() > 0) {
		for (int i = 1; i < g_EngineClient->GetMaxClients(); i++)
		{
			if (i > 64) break;
			auto pEntityPlayer = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(i));
			if (!pEntityPlayer)
				continue;
			if (pEntityPlayer == pLocal)
				continue;
			if (pEntityPlayer->m_iHealth() <= 0)
				continue;
			if (pEntityPlayer->IsDormant())
				continue;
			//if (pEntityPlayer->GetImmune())
			//	continue;
			if (pEntityPlayer->m_iTeamNum() == pLocal->m_iTeamNum() && !Legitbot.Deathmatch)
				continue;

			Vector PreVectorPos = ClosestBone(pEntityPlayer, pCmd, Rcs);
			if (PreVectorPos.x > -0.01f && PreVectorPos.x < 0.01f &&
				PreVectorPos.y > -0.01f && PreVectorPos.y < 0.01f)continue;


			float flFoV = GetFOV(Vector(pCmd->viewangles.pitch, pCmd->viewangles.yaw, pCmd->viewangles.roll) + Rcs, pLocal->GetEyePos(), PreVectorPos);
			if (flFoV < flMax) {
				flMax = flFoV;
				NextBonePos = PreVectorPos;
				iBestTarget = i;
			}

		}
	}
	return iBestTarget;
}
float Aimbot::RandomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}
void Aimbot::Run_Aim(CUserCmd* cmd, bool bPistol)
{

	auto pLocal = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));

	if (!pLocal) return;

	if (!InputSys::Get().IsKeyDown(Legitbot.key))
		return;

	if (Legitbot.KillDelay /*&& !AutoFire*/ && (g_GlobalVars->curtime <= G.KillDelayTime))
		return;

	if (Legitbot.FlashCheck && pLocal->IsFlashed())
		return;

	if (Legitbot.JumpCheck && !(pLocal->m_fFlags() & FL_ONGROUND))
		return;





	int ShootFired = bPistol ? BulletsFire : pLocal->m_iShotsFired();


	bool psile = (weapons[G.WeaponID].pSilent && ShootFired <= 1) ? true : false;

	bool PSilent = (weapons[G.WeaponID].pSilent && ShootFired < 1) ? true : false;

	if (psile)
	{
		if (!(Choking < 2 && (weapons[G.WeaponID].pSilentPercentage > RandomFloat(0.f, 101.f) || weapons[G.WeaponID].pSilentPercentage == 100.f)))
		{
			psile = false;
		}
	}
	Vector Rcs = Vector(0.f, 0.f, 0.f);

	if (G.WeaponID != WEAPON_AWP && G.WeaponID != WEAPON_SSG08)
	{
		Rcs.x = pLocal->m_aimPunchAngle().pitch * (weapons[G.WeaponID].RcsX / 50.f);
		Rcs.y = pLocal->m_aimPunchAngle().roll * (weapons[G.WeaponID].RcsY / 50.f);
	}




	//	if (g_Options.NewLegitbot.Weapon[WeaponID].Aimtype == 0)
	//	{

	if (!(cmd->buttons & IN_ATTACK))
		return;

	if (weapons[G.WeaponID].StartBullet > 0 && weapons[G.WeaponID].EndBullet > 0)
	{

		if (ShootFired <= (weapons[G.WeaponID].StartBullet - 1) && !psile)
			return;

		if (weapons[G.WeaponID].EndBullet != 0 && ShootFired >= weapons[G.WeaponID].EndBullet)
			return;
	}
	else
	{
		if (weapons[G.WeaponID].EndBullet <= 0 && weapons[G.WeaponID].StartBullet > 0)
		{
			if (ShootFired <= (weapons[G.WeaponID].StartBullet - 1) && !psile)
				return;
		}
	}



	//	}
	//	if (g_Options.NewLegitbot.Weapon[WeaponID].Aimtype == 1)
	//	{
	//		if (!GetAsyncKeyState(g_Options.NewLegitbot.Aimbot.Key) || g_Options.NewLegitbot.Aimbot.Key <= 0)
	//			return;
	//	}



	Vector vecBone = Vector(0.f, 0.f, 0.f);

	int pID = FindTarget(cmd, Rcs, vecBone);


	if (pID <= 0 ||
		(vecBone.x > -0.01f && vecBone.x < 0.01f &&
			vecBone.y > -0.01f && vecBone.y < 0.01f
			))
		return;

	if (Legitbot.SmokeCheck && Utils::LineGoesThroughSmoke(vecBone, pLocal->GetEyePos()))
		return;



	float fov = psile ? weapons[G.WeaponID].pSilentFov : weapons[G.WeaponID].Fov;

	if (GetFOV(Vector(cmd->viewangles.pitch, cmd->viewangles.yaw, cmd->viewangles.roll) + Rcs, pLocal->GetEyePos(), vecBone) > fov)
		return;

	Vector dst = Utils::CalculateAngle(pLocal->GetEyePos(), vecBone);


	float smooth = psile ? 0.f : weapons[G.WeaponID].Smooth;


	dst -= Rcs;



	if (smooth > 1.f && !psile)
	{

		if (Legitbot.AimType == 0 || Legitbot.AimType == 1)
		{
			Vector delta = Vector(cmd->viewangles.pitch, cmd->viewangles.yaw, cmd->viewangles.roll) - dst;
			if (!(
				delta.x > -0.01f && delta.x < 0.01f &&
				delta.y > -0.01f && delta.y < 0.01f))
			{
				delta.Normalized();
				dst = Vector(cmd->viewangles.pitch, cmd->viewangles.yaw, cmd->viewangles.roll) - delta / smooth;
			}
		}
		else if (Legitbot.AimType == 2)
		{
			dst = CurveSmooth(Vector(cmd->viewangles.pitch, cmd->viewangles.yaw, cmd->viewangles.roll), dst, smooth / 10.f);
		}

	}



	//Misc::Normalize(dst);

	if (Legitbot.FastZoom[G.WeaponID == WEAPON_AWP ? 1 : 0] && (G.WeaponID == WEAPON_AWP || G.WeaponID == WEAPON_SSG08))
	{
		if (cmd->buttons & IN_ATTACK)
		{
			if (!pLocal->m_bIsScoped())
			{
				cmd->buttons &= ~IN_ATTACK;
				cmd->buttons |= IN_ZOOM;
			}
		}
	}


	G.SendPacket = !psile;

	cmd->viewangles = QAngle(dst.x, dst.y, dst.z);
	Math::ClampAngles(cmd->viewangles);

	if (!psile)
	{
		g_EngineClient->SetViewAngles(cmd->viewangles);
	}
	if (psile)
		Choking = -1;

	
}
void Aimbot::Run(CUserCmd* pCmd)
{

	if (Menu::Get().IsVisible())
		return;

	auto pLocal = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));

	if (!pLocal) return;
	if (pLocal->m_iHealth() <= 0) return;
	auto LocalWeapon = pLocal->m_hActiveWeapon();
	if (!LocalWeapon)
		return;
	if (!weapons[G.WeaponID].Enabled)
		return;
	if (Utils::IsNonAimWeapon(G.WeaponID))
		return;
	if (LocalWeapon->m_iClip1() < 1)
		return;
	if (LocalWeapon->IsReloading())
		return;


	auto IsPistol = Utils::IsWeaponPistol(G.WeaponID);



	if (Shooting(pCmd, LocalWeapon) && IsPistol)
	{
		curtime = g_GlobalVars->curtime + 0.195f;
		BulletsFire++;
	}
	else
	{
		if (g_GlobalVars->curtime >= curtime || !IsPistol)
		{
			BulletsFire = 0;
		}
	}


	Run_Aim(pCmd, IsPistol);


	if (!G.SendPacket)
	{
		Choking = -1;
	}
}
const int Aimbot::RandomInt1(int low, int high) {
	return rand() % (high - low + 1) + low;
}

void Aimbot::Autopistol_Run(CUserCmd* cmd)
{
	if (G.WeaponID == WEAPON_CZ75A 
		|| G.WeaponID == WEAPON_TEC9 
		|| G.WeaponID == WEAPON_GLOCK 
		|| G.WeaponID == WEAPON_USP_SILENCER 
		|| G.WeaponID == WEAPON_FIVESEVEN 
		|| G.WeaponID == WEAPON_ELITE 
		|| G.WeaponID == WEAPON_HKP2000
		|| G.WeaponID == WEAPON_DEAGLE)
	{
		if (cmd->buttons & IN_ATTACK)
		{
			static bool WasFiring = false;
			WasFiring = !WasFiring;

			if (WasFiring)
			{
				cmd->buttons &= ~IN_ATTACK;
			}
		}
	}
}

void Aimbot::Triggerbot_Run(CUserCmd* cmd)
{
	static auto _trigger_delay = 0.f;
	static auto update_delay = [](float& del) {
		del = g_GlobalVars->curtime + Triggerbot.Delay;
	};
	if (!Triggerbot.Enabled)
		return;

	if (Triggerbot.Key != 0 && !InputSys::Get().IsKeyDown(Triggerbot.Key))
		return;


	auto pLocalPlayer = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));

	if (!pLocalPlayer) return;
	if (pLocalPlayer->m_iHealth() <= 0) return;
	auto pLocalWeapon = pLocalPlayer->m_hActiveWeapon();

	if (!pLocalWeapon)
		return;

	auto pWeaponData = pLocalWeapon->GetCSWeaponData();

	if (!pWeaponData)
		return;


	if (!weapons[G.WeaponID].TriggerEnabled)
		return;

	Vector vecSource = pLocalPlayer->GetEyePos();
	QAngle angSource = cmd->viewangles + (pLocalPlayer->m_aimPunchAngle() * 2.0f);
	Vector vecDest;
	Math::AngleVectors(angSource, vecDest);
	if (G.WeaponID != WEAPON_TABLET)
	vecDest = vecSource + (vecDest * pWeaponData->flRange);

	if (Triggerbot.SmokeCheck && Utils::LineGoesThroughSmoke(vecSource, vecDest))
		return;

	trace_t tr;
	Ray_t ray;
	ray.Init(vecSource, vecDest);


	CTraceFilter filter;
	filter.pSkip = pLocalPlayer;
	g_EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

	if (tr.allsolid || tr.startsolid)
		return;

	bool didHit = false;

	if (tr.hitgroup == HITGROUP_GENERIC || tr.hitgroup == HITGROUP_GEAR)
		return;

	if (weapons[G.WeaponID].TriggerHitboxHead && tr.hitgroup == HITGROUP_HEAD)
		didHit = true;
	if (weapons[G.WeaponID].TriggerHitboxChest && tr.hitgroup == HITGROUP_CHEST)
		didHit = true;
	if (weapons[G.WeaponID].TriggerHitboxStomach && tr.hitgroup == HITGROUP_STOMACH)
		didHit = true;
	if (weapons[G.WeaponID].TriggerHitboxArms && (tr.hitgroup == HITGROUP_LEFTARM || tr.hitgroup == HITGROUP_RIGHTARM))
		didHit = true;
	if (weapons[G.WeaponID].TriggerHitboxLegs && (tr.hitgroup == HITGROUP_LEFTLEG || tr.hitgroup == HITGROUP_RIGHTLEG))
		didHit = true;

	IClientEntity* pBaseEntity = tr.hit_entity;

	if (!pBaseEntity)
		return;

	if (pBaseEntity->GetClientClass()->m_ClassID != CCSPlayer)
		return;

	C_BasePlayer* pBasePlayer = (C_BasePlayer*)pBaseEntity;

	if (pBasePlayer->m_iHealth() < 1)
		return;

	if (!Triggerbot.Deathmatch && pLocalPlayer->m_iTeamNum() == pBasePlayer->m_iTeamNum())
		return;

	if (weapons[G.WeaponID].TriggerHitChance > 0 && !Utils::HitChance_Run(pLocalPlayer, cmd->viewangles, weapons[G.WeaponID].TriggerHitChance))
		return;

	static bool attack = false;

	if (didHit)
	{
		if (_trigger_delay - g_GlobalVars->curtime < -0.5f)
			update_delay(_trigger_delay);
		if (_trigger_delay <= g_GlobalVars->curtime)
			cmd->buttons |= IN_ATTACK;
	}

	attack = !attack;
}
void Aimbot::LegitAA(CUserCmd* pCmd) {
	auto weapon = g_LocalPlayer->m_hActiveWeapon().Get();
	auto winfo = weapon->GetCSWeaponData();
	if (!winfo)
		return;

	if ((pCmd->buttons & IN_USE) ||
		(pCmd->buttons & IN_ATTACK) ||
		!g_LocalPlayer ||
		!g_LocalPlayer->IsAlive() ||
		g_LocalPlayer->m_nMoveType() == MOVETYPE_LADDER ||
		g_LocalPlayer->m_nMoveType() == MOVETYPE_NOCLIP ||
		!weapon ||
		winfo->iWeaponType == WEAPONTYPE_GRENADE ||
		(g_GameRules && *g_GameRules && (*g_GameRules)->m_bFreezePeriod())
		)
		return;

	if (g_EngineClient->GetNetChannelInfo()->GetAvgLoss(FLOW_OUTGOING) > 3) {
		return;
	}
	auto latency = g_EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_OUTGOING) + g_EngineClient->GetNetChannelInfo()->GetAvgLatency(FLOW_INCOMING);
	if (latency >= 70) {

		return;
	}
	QAngle temp = pCmd->viewangles;
	if (!pCmd->buttons & IN_ATTACK)
	{
		Choking++;
		if (Choking < 1)
		{
			G.SendPacket = true;
		}
		else
		{
			G.SendPacket = false;
			Choking = -1;
		}
		if (G.SendPacket) {
			QAngle va;
			g_EngineClient->GetViewAngles(va);
			temp.yaw = va.yaw;
		}
		else {
			temp.yaw += Legitbot.LegitAA_Offset;//115.f;
		}
		Math::ClampAngles(temp);
		pCmd->viewangles = temp;

	}
}