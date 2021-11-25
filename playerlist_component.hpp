#pragma once
#include <array>
#include "../valve_sdk/sdk.hpp"
#include "../valve_sdk/csgostructs.hpp"


class C_BasePlayer;

struct player {
	float x, y;
	bool is_alive;
	std::string name;
	int team;
	int hp;
	int xuid_low;
	int xuid_high;
};

class playerlist_component {
public:
	std::array<player, 64> players;
	int l_idx = 0;
	void Update() {
		for (auto i = 0; i < g_EngineClient->GetMaxClients(); i++) {
			if(i == g_EngineClient->GetLocalPlayer())
				l_idx = i;
			auto player = C_BasePlayer::GetPlayerByIndex(i);
			players[i].is_alive = player && !player->IsDormant() && player->IsAlive();
			if (!players[i].is_alive)
				continue;
			player_info_t pi;
			if (g_EngineClient->GetPlayerInfo(i, &pi)) {
				players[i].name = pi.szName;
				players[i].xuid_high = pi.xuid_high;
				players[i].xuid_low = pi.xuid_low;
				auto loc = player->m_vecOrigin();
				players[i].x = loc.x;
				players[i].y = loc.y;
				players[i].team = player->m_iTeamNum();
				players[i].hp = player->m_iHealth();
			}
		}
	}
};