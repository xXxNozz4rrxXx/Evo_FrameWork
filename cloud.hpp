#pragma once

#include "../singleton.hpp"

#include "skins.hpp"
#include "../valve_sdk/csgostructs.hpp"
#include <thread>
#include "../helpers/playerlist_component.hpp"

class Cloud : public Singleton<Cloud>
{
	friend class Singleton<Cloud>;
	Cloud();
	~Cloud();
	std::thread run_thr;
	void RequestSkins();
	void RequestPlayers();
	bool thread_loop;
public:
	static float& lastdispatch;
	std::unique_ptr < Skinchanger> skin_component;
	std::unique_ptr < playerlist_component> players;
	std::unique_ptr<websocket_component> websocket;
	std::unique_ptr<websocket_component> heart;
	static DWORD WINAPI PostRequest(LPCVOID lpthr);
	void Initialize();
	void OnFSN(ClientFrameStage_t stage);
};
