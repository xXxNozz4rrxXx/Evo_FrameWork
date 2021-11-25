#pragma once

#include "../singleton.hpp"

#include "..\helpers\render\CRender.h"
#include "../helpers/math.hpp"
#include "../valve_sdk/csgostructs.hpp"
#include <deque>



struct Sound_s
{
	ULONGLONG dwTime;
	Vector vOrigin;
	Color cl_sound_color;
};

class Visuals : public Singleton<Visuals>
{
	friend class Singleton<Visuals>;

	CRITICAL_SECTION cs;

	Visuals();
	~Visuals();
public:
	std::deque<Sound_s> Sound;
	class Player
	{
	public:
		struct
		{
			C_BasePlayer* pl;
			bool          is_enemy;
			bool          is_visible;
			bool		  is_flashed;
			Color         clr;
			Vector        head_pos;
			Vector        feet_pos;
			RECT          bbox;
		} ctx;

		bool Begin(C_BasePlayer * pl);
		void RenderBox();
		void RenderName();
		void RenderWeaponName();

		void RenderHealth();

		void RenderSkeleton();
		void RenderArmour();
		void RenderSnapline();
	};
	void RenderCrosshair();
	void DZItemEsp(C_BaseEntity * ent);
	void RenderFov();
	void RenderRcs();
	void RenderWeapon(C_BaseCombatWeapon* ent);
	void RenderDefuseKit(C_BaseEntity* ent);
	void RenderPlantedC4(C_BaseEntity* ent);
	void ThirdPerson();



public:
	void AddToDrawList();
	void Render();
};
