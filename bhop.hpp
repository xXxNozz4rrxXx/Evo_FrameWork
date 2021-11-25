#pragma once
#include "../singleton.hpp"
class C_BasePlayer;
class CUserCmd;
class Vector;

class Bhop : public Singleton<Bhop> {
	friend class Singleton<Bhop>;
	Bhop() = default;
	~Bhop() = default;
	bool KnifeBestTarget();
	void RunKnifeBot(CUserCmd* cmd);
	bool lastAttacked = false;
	int m_nBestIndex = -1;
	float m_nBestDist = -1;
	C_BasePlayer* pBestEntity;
public:
	void OnCreateMove(CUserCmd* cmd);
	
};
