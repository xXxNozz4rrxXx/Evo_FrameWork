#pragma once
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		10

#include "visuals.hpp"

#include "../options.hpp"
#include "../helpers/math.hpp"
#include "../helpers/utils.hpp"
#include "../menu.hpp"
#include "../helpers/input.hpp"


class Aimbot : public Singleton<Aimbot> {
	friend class Singleton <Aimbot>;
	Aimbot() = default;
	~Aimbot() = default;
	int BulletsFire;
	float curtime = 0;
	int Choking = 0;
	float GetFOV(Vector& viewangles, const Vector& vSrc, const Vector& vEnd);
	Vector CurveSmooth(Vector& viewangles, Vector target, float factor);
	Vector ClosestBone(C_BasePlayer *Entity, CUserCmd* cmd, Vector Rcs);
	float GetServerTime();
	bool bullettime(C_BaseCombatWeapon* weapon);
	bool Shooting(CUserCmd* pCmd, C_BaseCombatWeapon* weapon);
	int FindTarget(CUserCmd* pCmd, Vector Rcs, Vector& NextBonePos);
	float RandomFloat(float a, float b);
	void Run_Aim(CUserCmd* cmd, bool bPistol);
	inline const int RandomInt1(int low, int high);
public:
	void LegitAA(CUserCmd* pCmd);
	void Run(CUserCmd* pCmd);
	void Triggerbot_Run(CUserCmd* cmd);
	void Autopistol_Run(CUserCmd* cmd);
};
	
