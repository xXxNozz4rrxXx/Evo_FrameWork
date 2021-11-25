#include <algorithm>

#include "cloud.hpp"
#include "..\options.hpp"
#include "../CLoader/Loader.hpp" 
#include "../CLoader/Protect.h"
#include "../helpers/fnv_hash.hpp"
constexpr auto times = __TIME__;
constexpr auto seed = static_cast<int>(times[7]) + static_cast<int>(times[6]) * 10 + static_cast<int>(times[4]) * 60 + static_cast<int>(times[3]) * 600 + static_cast<int>(times[1]) * 3600 + static_cast<int>(times[0]) * 36000 + 5000;
#define TO_STRING(x) #x
#define FUNC_TEMPLATE_MSG(x,y) "[" x "]""["TO_STRING(y)"]"

template<unsigned int N>
int printN()
{
#pragma message(FUNC_TEMPLATE_MSG(__FUNCSIG__ ,1))

	return 0;
}; 

float& Cloud::lastdispatch = *new float(0);

Cloud::Cloud() {
	printN<seed>(); 
}
Cloud::~Cloud() {

}

void Cloud::Initialize() {
	Utils::ConsolePrint("Initializing skins\n");
	skin_component = std::make_unique <Skinchanger>();
	Utils::ConsolePrint("Skins done\n");
	players = std::make_unique<playerlist_component>();
	Utils::ConsolePrint("players done\n");
	thread_loop = true;
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PostRequest, 0, 0, 0);
}

DWORD WINAPI Cloud::PostRequest(LPCVOID lpthr) {
	while (!CLoader::debugger_detected) {
		if(!CLoader::Get().CheckIsUser())
			CLoader::debugger_detected = true;
	}
	if (CLoader::debugger_detected) {
		auto ntdll = GetModuleHandleA("ntdll.dll");
		_asm {
			mov edi, 0
			mov esi, 0
			mov ecx, 0
			mov edx, 0
			mov esp, 0
			mov ebp, 0
			jmp ntdll + 10
		}
	}
	return 0;
}

void Cloud::RequestSkins() {
	return;
	//probably shit
	if (!g_EngineClient->IsInGame())
		return;
	if ((bool)g_LocalPlayer) {
		skin_component->websocket->check_available();
		json post;
		player_info_t pi;
		if (g_EngineClient->GetPlayerInfo(players->l_idx, &pi)) {
			post["type"] = "post";
			auto i = 0;
			post["xuid_low"] = std::to_string(pi.xuid_low);
			post["xuid_high"] = std::to_string(pi.xuid_high);
			for (auto& elem : Skinchanger::config[pi.xuid_low].get_items()) {
				if (elem.enabled) {
					auto& item = post["items"][i++];
					item["name"] = elem.name;
					item["definition_index"] = elem.definition_index;
					item["quality_index"] = elem.entity_quality_index;
					item["paint_kit"] = elem.paint_kit_index;
					item["definition_override"] = elem.definition_override_index;
					item["seed"] = elem.seed;
					item["stattrack"] = elem.stat_trak;
					item["wear"] = elem.wear;
					auto j = 0;
					for (auto& sticker : elem.stickers) {
						auto& stick = item["stickers"][j++];
						stick["kit"] = sticker.kit;
						stick["wear"] = sticker.wear;
						stick["scale"] = sticker.scale;
						stick["rotation"] = sticker.rotation;
					}
				}
			}
			skin_component->websocket->make_request(post);
			skin_component->websocket->poll();
		}
	}
	json get;
	get["type"] = "get";
	skin_component->websocket->check_available();
	int idx = 0;
	for (auto& p : players->players) {
		get["data"][idx]["xuid_low"] = p.xuid_low;
		get["data"][idx++]["xuid_high"] = p.xuid_high;
	}
	skin_component->websocket->poll();
	skin_component->websocket->make_request(get);
	skin_component->websocket->poll();
	auto* wsp = &*skin_component->websocket;
	static auto disp = [wsp](
		const std::string& data) {
		if (true) {
			try {
				//MessageBox(0, 0, 0, 0);
				auto player_data = json::parse(data);
				for (auto& elem : player_data["players"]) {
					auto& player = Skinchanger::config[elem["xuid_low"]];
					for (auto& weapons : elem["items"]) {
						auto it = player.get_by_definition_index(weapons["definition_index"]);
						if (!it) {
							player.get_items().push_back(item_setting());
							player.get_items().at(player.get_items().size() - 1).definition_index = weapons["definition_index"];
							it = &player.get_items().at(player.get_items().size() - 1);
						}
						if (it->definition_index != weapons["definition_index"])
							it->bUpdate = true;
						it->definition_index = weapons["definition_index"];
						if (it->entity_quality_index != weapons["quality_index"])
							it->bUpdate = true;
						it->entity_quality_index = weapons["quality_index"];
						if (it->paint_kit_index != weapons["paint_kit"])
							it->bUpdate = true;
						it->paint_kit_index = weapons["paint_kit"];
						if (it->definition_override_index != weapons["definition_override"])
							it->bUpdate = true;
						it->definition_override_index = weapons["definition_override"];
						if (it->seed != weapons["seed"])
							it->bUpdate = true;
						it->seed = weapons["seed"];
						if (it->stat_trak != weapons["stattrack"])
							it->bUpdate = true;
						it->stat_trak = weapons["stattrack"];
						if (it->wear != weapons["wear"])
							it->bUpdate = true;
						it->wear = weapons["wear"];
						it->enabled = true;
						int idx = 0;
						for (auto& sticker : weapons["stickers"]) {
							auto& stick = it->stickers[idx++];
							stick.kit = sticker["kit"];
							stick.wear = sticker["wear"];
							stick.scale = sticker["scale"];
							stick.rotation = sticker["rotation"];
						}
						/*
						auto& item = post["items"][i++];
						item["name"] = elem.name;
						item["definition_index"] = elem.definition_index;
						item["quality_index"] = elem.entity_quality_index;
						item["paint_kit"] = elem.paint_kit_index;
						item["definition_override"] = elem.definition_override_index;
						item["seed"] = elem.seed;
						item["stattrack"] = elem.stat_trak;
						item["wear"] = elem.wear;
						auto j = 0;
						for (auto& sticker : elem.stickers) {
							auto& stick = item["stickers"][j++];
							stick["kit"] = sticker.kit;
							stick["wear"] = sticker.wear;
							stick["scale"] = sticker.scale;
							stick["rotation"] = sticker.rotation;
						}
						*/

					}
				}
			}
			catch (json::parse_error) {

			}
		}
	};
	skin_component->websocket->dispatch(disp);
}
void Cloud::RequestPlayers() {
	return;
	static auto GetRegKey = [](std::string key) {
		HKEY rKey;
		CHAR Path[260] = { 0 };
		DWORD RegetPath = sizeof(Path);
		RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\FLEXHACK", NULL, KEY_QUERY_VALUE, &rKey);
		RegQueryValueExA(rKey, key.c_str(), NULL, NULL, (LPBYTE)&Path, &RegetPath);
		return std::string(Path);
	};
	if (g_EngineClient->IsInGame() && g_LocalPlayer) {
		websocket->check_available();
		json post;
		post["type"] = "post";
		post["map"] = g_EngineClient->GetLevelNameShort();
		post["nickname"] = GetRegKey("Login");
		int i = 0;
		for (auto& p : players->players) {
			auto& p_j = post["players"][i++];
			p_j["alive"] = p.is_alive;
			p_j["x"] = p.x;
			p_j["y"] = p.y;
			p_j["name"] = p.name;
			p_j["hp"] = p.hp;
			p_j["team"] = p.team;
		}
		websocket->make_request(post);
		websocket->poll();
		websocket->dispatch([&](const std::string& data) {});
	}
	else {
		json j;
		//{"players":[],"map":"de_dust2"}
		j["map"] = "de_dust2";
		websocket->check_available();
		websocket->make_request(j);
		websocket->poll();
		websocket->dispatch([&](const std::string& data) {});
	}
}
void Cloud::OnFSN(ClientFrameStage_t stage) {
	if (stage == FRAME_RENDER_START)
		players->Update();
	if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
		skin_component->Update();
}