#include "hooks.hpp"
#include <intrin.h>  

//#include "..\inventory.hpp"
#include "render.hpp"
#include "menu.hpp"
#include "options.hpp"
#include "helpers/input.hpp"
#include "helpers/utils.hpp"
#include "features/bhop.hpp"
#include "features/chams.hpp"
#include "features/cloud.hpp"
#include "features/visuals.hpp"
#include "features/glow.hpp"
#include "features/aimbot.hpp"
#include "features/profile.hpp"
#include "helpers/fnv_hash.hpp"
#include <deque>
#include "helpers/render/CRender.h"
#include <mutex>
#pragma intrinsic(_ReturnAddress)  

namespace Hooks
{
	vfunc_hook hlclient_hook;
	vfunc_hook direct3d_hook;
	vfunc_hook vguipanel_hook;
	vfunc_hook vguisurf_hook;
	vfunc_hook sound_hook;
	vfunc_hook mdlrender_hook;
	vfunc_hook clientmode_hook;
	vfunc_hook sv_cheats;
	vfunc_hook render_view;
	vfunc_hook GameEvents;
	vfunc_hook gamecoordinator_hook;
	recv_prop_hook*	sequence_hook;
	Present_t Present_o;

	void Initialize()
	{
		hlclient_hook.setup(g_CHLClient);
		direct3d_hook.setup(g_D3DDevice9, "shaderapidx9.dll");
		vguipanel_hook.setup(g_VGuiPanel);
		vguisurf_hook.setup(g_VGuiSurface);
		sound_hook.setup(g_EngineSound);
		mdlrender_hook.setup(g_MdlRender);
		clientmode_hook.setup(g_ClientMode);
		GameEvents.setup(g_GameEvents);
		ConVar* sv_cheats_con = g_CVar->FindVar("sv_cheats");
		sv_cheats.setup(sv_cheats_con);
		render_view.setup(g_RenderView);
		gamecoordinator_hook.setup(g_GameCoordinator, "steam_api.dll");

		//direct3d_hook.hook_index(index::EndScene, hkEndScene);
		direct3d_hook.hook_index(index::Reset, hkReset);

		hlclient_hook.hook_index(index::FrameStageNotify, hkFrameStageNotify);
		hlclient_hook.hook_index(index::CreateMove, hkCreateMove_Proxy);

		vguipanel_hook.hook_index(index::PaintTraverse, hkPaintTraverse);

		sound_hook.hook_index(index::EmitSound1, hkEmitSound1);
		vguisurf_hook.hook_index(index::LockCursor, hkLockCursor);

		mdlrender_hook.hook_index(index::DrawModelExecute, hkDrawModelExecute);

		clientmode_hook.hook_index(index::DoPostScreenSpaceEffects, hkDoPostScreenEffects);
		clientmode_hook.hook_index(index::OverrideView, hkOverrideView);

		GameEvents.hook_index(9, CEventsRun);

		gamecoordinator_hook.hook_index(index::SendMessageGC, SendMessageGC);
		gamecoordinator_hook.hook_index(index::RetrieveMessageGC, RetrieveMessageGC);
																								//"
		sv_cheats.hook_index(index::SvCheatsGetBool, hkSvCheatsGetBool);													//x????x????xxxx????xxxxxxx
		DWORD_PTR** dwPresent_o = (DWORD_PTR**)(Utils::PatternScan(GetModuleHandleW(L"GameOverlayRenderer.dll"), "A3 ? ? ? ? 68 ? ? ? ? FF 76 54 E8 ? ? ? ? 83 C4 08 84 C0 75 17") + 1);
		Present_o = (Present_t)(**dwPresent_o);
		**dwPresent_o = (DWORD_PTR)(&Hook_Present);
		render_view.hook_index(index::SceneEnd, scene_end);
		sequence_hook = new recv_prop_hook(C_BaseViewModel::m_nSequence(), &sequence_proxy_fn);
	}
	//--------------------------------------------------------------------------------
	void Shutdown()
	{
		hlclient_hook.unhook_all();
		direct3d_hook.unhook_all();
		vguipanel_hook.unhook_all();
		vguisurf_hook.unhook_all();
		mdlrender_hook.unhook_all();
		clientmode_hook.unhook_all();
		render_view.unhook_all();

		Glow::Get().Shutdown();
	}
	//--------------------------------------------------------------------------------

	HRESULT WINAPI Hook_Present(IDirect3DDevice9* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
	{
		static auto viewmodel_fov = g_CVar->FindVar("viewmodel_fov");
		static auto mat_ambient_light_r = g_CVar->FindVar("mat_ambient_light_r");
		static auto mat_ambient_light_g = g_CVar->FindVar("mat_ambient_light_g");
		static auto mat_ambient_light_b = g_CVar->FindVar("mat_ambient_light_b");
		static auto crosshair_cvar = g_CVar->FindVar("crosshair");

		viewmodel_fov->m_fnChangeCallbacks.m_Size = 0;
		if(g_Options.fov_changer)
			viewmodel_fov->SetValue(g_Options.viewmodel_fov);
		mat_ambient_light_r->SetValue(g_Options.mat_ambient_light_r);
		mat_ambient_light_g->SetValue(g_Options.mat_ambient_light_g);
		mat_ambient_light_b->SetValue(g_Options.mat_ambient_light_b);
		crosshair_cvar->SetValue(!g_Options.esp_crosshair);

		DWORD colorwrite, srgbwrite;
		pDevice->GetRenderState(D3DRS_COLORWRITEENABLE, &colorwrite);
		pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgbwrite);

		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
		//removes the source engine color correction
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

		pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		pDevice->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		pDevice->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, NULL);


		CRender::Get().BeginRender();
        CRender::Get().RenderDrawList(Render::Get().RenderScene());
		CRender::Get().EndRender();

		ImGui_ImplDX9_NewFrame();


		Menu::Get().Render();


		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	
		
		
		
		pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite);
		pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, srgbwrite);

		return Present_o(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	}
	//--------------------------------------------------------------------------------
	long __stdcall hkReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		auto oReset = direct3d_hook.GetOriginalFunc<Reset>(index::Reset);

		Menu::Get().OnDeviceLost();
		CRender::Get().OnLostDevice();

		auto hr = oReset(device, pPresentationParameters);

		if (hr >= 0)
		{
			Menu::Get().OnDeviceReset();
			CRender::Get().OnResetDevice();
		}

		return hr;
	}

	//--------------------------------------------------------------------------------
	void __stdcall hkCreateMove(int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket)
	{
		auto oCreateMove = hlclient_hook.GetOriginalFunc<CreateMove>(index::CreateMove);

		G.SendPacket = true;

		oCreateMove(g_CHLClient, sequence_number, input_sample_frametime, active);

		auto cmd = g_Input->GetUserCmd(sequence_number);
		auto verified = g_Input->GetVerifiedCmd(sequence_number);

		if (!cmd || !cmd->command_number)
			return;

		G.SendPacket = bSendPacket;

		auto pLocal = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));
		if (!pLocal || !pLocal->IsAlive())
			return;


		Bhop::Get().OnCreateMove(cmd);


		if (pLocal && pLocal->IsAlive())
			G.WeaponID = pLocal->m_hActiveWeapon()->m_Item().m_iItemDefinitionIndex();

		if (Legitbot.Enabled)
			Aimbot::Get().Run(cmd);

		if (Legitbot.AutoPistol)
			Aimbot::Get().Autopistol_Run(cmd);

		if (Triggerbot.Enabled)
			Aimbot::Get().Triggerbot_Run(cmd);

		if (Legitbot.LegitAA)
			Aimbot::Get().LegitAA(cmd);
		Math::ClampAngles(cmd->viewangles);
		bSendPacket = G.SendPacket;
		verified->m_cmd = *cmd;
		verified->m_crc = cmd->GetChecksum();
	}
	//--------------------------------------------------------------------------------
	__declspec(naked) void __stdcall hkCreateMove_Proxy(int sequence_number, float input_sample_frametime, bool active)
	{
		__asm
		{
			push ebp
			mov  ebp, esp
			push ebx
			lea  ecx, [esp]
			push ecx
			push dword ptr[active]
			push dword ptr[input_sample_frametime]
			push dword ptr[sequence_number]
			call Hooks::hkCreateMove
			pop  ebx
			pop  ebp
			retn 0Ch
		}
	}
	//--------------------------------------------------------------------------------
	void __stdcall hkPaintTraverse(vgui::VPANEL panel, bool forceRepaint, bool allowForce)
	{
		static auto panelId = vgui::VPANEL{ 0 };
		static auto oPaintTraverse = vguipanel_hook.GetOriginalFunc<PaintTraverse>(index::PaintTraverse);

		oPaintTraverse(g_VGuiPanel, panel, forceRepaint, allowForce);

		if (!panelId) {
			const auto panelName = g_VGuiPanel->GetName(panel);
			if (!strcmp(panelName, "FocusOverlayPanel")) {
				panelId = panel;
			}
		}
		else if (panelId == panel) {
			//Ignore 50% cuz it called very often
			static bool bSkip = false;
			bSkip = !bSkip;

			if (bSkip)
				return;

			if (g_LocalPlayer && InputSys::Get().IsKeyDown(VK_TAB) && g_Options.misc_showranks)
				Utils::RankRevealAll();

			Render::Get().BeginScene();
		}
	}
	//--------------------------------------------------------------------------------
	void __stdcall hkEmitSound1(IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char *pSample, float flVolume, int nSeed, float flAttenuation, int iFlags, int iPitch, const Vector* pOrigin, const Vector* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unk) {
		static auto ofunc = sound_hook.GetOriginalFunc<EmitSound1>(index::EmitSound1);


		if (!strcmp(pSoundEntry, "UIPanorama.popup_accept_match_beep")) {
			static auto fnAccept = reinterpret_cast<bool(__stdcall*)(const char*)>(Utils::PatternScan(GetModuleHandleA("client_panorama.dll"), "55 8B EC 83 E4 F8 8B 4D 08 BA ? ? ? ? E8 ? ? ? ? 85 C0 75 12"));

			if (fnAccept) {

				fnAccept("");

				//This will flash the CSGO window on the taskbar
				//so we know a game was found (you cant hear the beep sometimes cause it auto-accepts too fast)
				FLASHWINFO fi;
				fi.cbSize = sizeof(FLASHWINFO);
				fi.hwnd = InputSys::Get().GetMainWindow();
				fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
				fi.uCount = 0;
				fi.dwTimeout = 0;
				FlashWindowEx(&fi);
			}
		}

		ofunc(g_EngineSound, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, nSeed, flAttenuation, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity, unk);

	}
	//--------------------------------------------------------------------------------
	int __stdcall hkDoPostScreenEffects(int a1)
	{
		auto oDoPostScreenEffects = clientmode_hook.GetOriginalFunc<DoPostScreenEffects>(index::DoPostScreenSpaceEffects);

		if (g_LocalPlayer && g_Options.glow_enabled)
			Glow::Get().Run();

		return oDoPostScreenEffects(g_ClientMode, a1);
	}
	auto random_sequence(const int low, const int high) -> int
	{
		return rand() % (high - low + 1) + low;
	}

	auto get_new_animation(const fnv::hash model, const int sequence) -> int
	{
		enum ESequence
		{
			SEQUENCE_DEFAULT_DRAW = 0,
			SEQUENCE_DEFAULT_IDLE1 = 1,
			SEQUENCE_DEFAULT_IDLE2 = 2,
			SEQUENCE_DEFAULT_LIGHT_MISS1 = 3,
			SEQUENCE_DEFAULT_LIGHT_MISS2 = 4,
			SEQUENCE_DEFAULT_HEAVY_MISS1 = 9,
			SEQUENCE_DEFAULT_HEAVY_HIT1 = 10,
			SEQUENCE_DEFAULT_HEAVY_BACKSTAB = 11,
			SEQUENCE_DEFAULT_LOOKAT01 = 12,

			SEQUENCE_BUTTERFLY_DRAW = 0,
			SEQUENCE_BUTTERFLY_DRAW2 = 1,
			SEQUENCE_BUTTERFLY_LOOKAT01 = 13,
			SEQUENCE_BUTTERFLY_LOOKAT03 = 15,

			SEQUENCE_FALCHION_IDLE1 = 1,
			SEQUENCE_FALCHION_HEAVY_MISS1 = 8,
			SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP = 9,
			SEQUENCE_FALCHION_LOOKAT01 = 12,
			SEQUENCE_FALCHION_LOOKAT02 = 13,

			SEQUENCE_DAGGERS_IDLE1 = 1,
			SEQUENCE_DAGGERS_LIGHT_MISS1 = 2,
			SEQUENCE_DAGGERS_LIGHT_MISS5 = 6,
			SEQUENCE_DAGGERS_HEAVY_MISS2 = 11,
			SEQUENCE_DAGGERS_HEAVY_MISS1 = 12,

			SEQUENCE_BOWIE_IDLE1 = 1,
		};

		// Hashes for best performance.
		switch (model)
		{
		case FNV("models/weapons/v_knife_butterfly.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_DRAW:
				return random_sequence(SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2);
			case SEQUENCE_DEFAULT_LOOKAT01:
				return random_sequence(SEQUENCE_BUTTERFLY_LOOKAT01, SEQUENCE_BUTTERFLY_LOOKAT03);
			default:
				return sequence + 1;
			}
		}
		case FNV("models/weapons/v_knife_falchion_advanced.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_IDLE2:
				return SEQUENCE_FALCHION_IDLE1;
			case SEQUENCE_DEFAULT_HEAVY_MISS1:
				return random_sequence(SEQUENCE_FALCHION_HEAVY_MISS1, SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP);
			case SEQUENCE_DEFAULT_LOOKAT01:
				return random_sequence(SEQUENCE_FALCHION_LOOKAT01, SEQUENCE_FALCHION_LOOKAT02);
			case SEQUENCE_DEFAULT_DRAW:
			case SEQUENCE_DEFAULT_IDLE1:
				return sequence;
			default:
				return sequence - 1;
			}
		}
		case FNV("models/weapons/v_knife_push.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_IDLE2:
				return SEQUENCE_DAGGERS_IDLE1;
			case SEQUENCE_DEFAULT_LIGHT_MISS1:
			case SEQUENCE_DEFAULT_LIGHT_MISS2:
				return random_sequence(SEQUENCE_DAGGERS_LIGHT_MISS1, SEQUENCE_DAGGERS_LIGHT_MISS5);
			case SEQUENCE_DEFAULT_HEAVY_MISS1:
				return random_sequence(SEQUENCE_DAGGERS_HEAVY_MISS2, SEQUENCE_DAGGERS_HEAVY_MISS1);
			case SEQUENCE_DEFAULT_HEAVY_HIT1:
			case SEQUENCE_DEFAULT_HEAVY_BACKSTAB:
			case SEQUENCE_DEFAULT_LOOKAT01:
				return sequence + 3;
			case SEQUENCE_DEFAULT_DRAW:
			case SEQUENCE_DEFAULT_IDLE1:
				return sequence;
			default:
				return sequence + 2;
			}
		}
		case FNV("models/weapons/v_knife_survival_bowie.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_DRAW:
			case SEQUENCE_DEFAULT_IDLE1:
				return sequence;
			case SEQUENCE_DEFAULT_IDLE2:
				return SEQUENCE_BOWIE_IDLE1;
			default:
				return sequence - 1;
			}
		}
		case FNV("models/weapons/v_knife_ursus.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_DRAW:
				return random_sequence(SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2);
			case SEQUENCE_DEFAULT_LOOKAT01:
				return random_sequence(SEQUENCE_BUTTERFLY_LOOKAT01, 14);
			default:
				return sequence + 1;
			}
		}
		case FNV("models/weapons/v_knife_stiletto.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_LOOKAT01:
				return random_sequence(12, 13);
			}
		}
		case FNV("models/weapons/v_knife_widowmaker.mdl"):
		{
			switch (sequence)
			{
			case SEQUENCE_DEFAULT_LOOKAT01:
				return random_sequence(14, 15);
			}
		}
		default:
			return sequence;
		}
	}

	auto do_sequence_remapping(CRecvProxyData* data, C_BaseViewModel* entity) -> void
	{
		const auto local = C_BasePlayer::GetPlayerByIndex(g_EngineClient->GetLocalPlayer());//static_cast<CBasePlayer*>(CInterfaces::Get().EntityList->GetClientEntity(CInterfaces::Get().EngineClient->GetLocalPlayer()));

		if (!local)
			return;

		if (local->m_lifeState() != LIFE_ALIVE)
			return;

		const auto owner = entity->m_hOwner().Get();//(CBasePlayer*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)entity->GetOwner());

		if (owner != local)
			return;

		const auto view_model_weapon = entity->m_hWeapon().Get();//(CBaseAttributableItem*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)entity->GetWeapon());

		if (!view_model_weapon)
			return;

		const auto weapon_info = Cloud::Get().skin_component->get_weapon_info(view_model_weapon->m_Item().m_iItemDefinitionIndex());

		if (!weapon_info)
			return;

		const auto override_model = weapon_info->model;

		auto& sequence = data->m_Value.m_Int;
		sequence = get_new_animation(fnv::hash_runtime(override_model), sequence);
	}

	auto __cdecl sequence_proxy_fn(const CRecvProxyData* proxy_data_const, void* entity, void* output) -> void
	{
		static auto original_fn = sequence_hook->get_original_function();

		// Remove the constness from the proxy data allowing us to make changes.
		const auto proxy_data = const_cast<CRecvProxyData*>(proxy_data_const);

		const auto view_model = static_cast<C_BaseViewModel*>(entity);

		do_sequence_remapping(proxy_data, view_model);

		// Call the original function with our edited data.
		original_fn(proxy_data_const, entity, output);
	}
	//--------------------------------------------------------------------------------
	void __stdcall hkFrameStageNotify(ClientFrameStage_t stage)
	{
		static auto ofunc = hlclient_hook.GetOriginalFunc<FrameStageNotify>(index::FrameStageNotify);

		static auto AddSound = [](Vector vOrigin, Color cl_sound_color) {
			auto find = [](const Sound_s& what) {
				for (auto& elem : Visuals::Get().Sound) {
					if (elem.vOrigin.x == what.vOrigin.x && elem.vOrigin.y == what.vOrigin.y)
						return true;
				}
				return false;
			};
			static Sound_s sound_entry;
			sound_entry.dwTime = GetTickCount64();
			sound_entry.cl_sound_color = cl_sound_color;
			sound_entry.vOrigin = vOrigin;
			if (!find(sound_entry) && vOrigin.DistTo(g_LocalPlayer->GetEyePos()) > 50)
				Visuals::Get().Sound.push_back(sound_entry);
		};
		Cloud::Get().OnFSN(stage);
		if (g_EngineClient->IsInGame() && g_LocalPlayer) {
			if (g_Options.esp_sound) {
				static CUtlVector<SndInfo_t> active_sounds;
				active_sounds.RemoveAll();
				g_EngineSound->GetActiveSounds(active_sounds);
				for (auto& sound : active_sounds) {
					if (sound.m_bUpdatePositions) {//CSoundEsp::Get().AddSound(vOrigin, Team == CMe::Get().Team ? CSoundEsp::Get().colors[0].ReturnEngineColor() : CSoundEsp::Get().colors[1].ReturnEngineColor(), Team);
						if (sound.m_nChannel == 4)
						{
							if (sound.m_nSoundSource > 0 && sound.m_nSoundSource < g_EngineClient->GetMaxClients()) {
								auto plr = C_BasePlayer::GetPlayerByIndex(sound.m_nSoundSource);
								if (plr && !plr->IsDormant() && plr->IsAlive() && plr != g_LocalPlayer && (!g_Options.esp_enemies_only || plr->m_iTeamNum() != g_LocalPlayer->m_iTeamNum()))
									AddSound(*sound.m_pOrigin, Utils::Float3ToClr(g_Options.color_esp_sound));
							}
						}
					}
				}
				Visuals::Get().Sound.erase(remove_if(Visuals::Get().Sound.begin(), Visuals::Get().Sound.end(), [](Sound_s x) {return x.dwTime + 800 <= GetTickCount64(); }), Visuals::Get().Sound.end());
			}
		}
		if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START && g_LocalPlayer)
		{
			auto maxAlpha = g_LocalPlayer->m_flFlashMaxAlpha();

			if (g_Options.misc_noflash)
				*maxAlpha = g_Options.misc_noflash_val;
			else
				*maxAlpha = 255.f;
		}
		ofunc(g_CHLClient, stage);
	}
	//--------------------------------------------------------------------------------
	void __stdcall hkOverrideView(CViewSetup* vsView)
	{
		static auto ofunc = clientmode_hook.GetOriginalFunc<OverrideView>(index::OverrideView);

		if (g_EngineClient->IsInGame() && vsView)
			Visuals::Get().ThirdPerson();

		if (g_LocalPlayer && g_EngineClient->IsInGame())
		{
			if (!g_LocalPlayer->m_bIsScoped())//IsScoped())
				if (g_Options.fov_changer) vsView->fov = g_Options.ov_fov;
		}

		ofunc(g_ClientMode, vsView);
	}
	//--------------------------------------------------------------------------------
	void __stdcall hkLockCursor()
	{
		static auto ofunc = vguisurf_hook.GetOriginalFunc<LockCursor_t>(index::LockCursor);

		if (Menu::Get().IsVisible()) {
			g_VGuiSurface->UnlockCursor();
			return;
		}
		ofunc(g_VGuiSurface);

	}
	//--------------------------------------------------------------------------------
	void __stdcall hkDrawModelExecute(IMatRenderContext* ctx, const DrawModelState_t& state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld)
	{
		static auto ofunc = mdlrender_hook.GetOriginalFunc<DrawModelExecute>(index::DrawModelExecute);

		Chams::Get().OnDrawModelExecute(ctx, state, pInfo, pCustomBoneToWorld);

		ofunc(g_MdlRender, ctx, state, pInfo, pCustomBoneToWorld);

		g_MdlRender->ForcedMaterialOverride(nullptr);
	}
	//--------------------------------------------------------------------------------
	bool __fastcall CEventsRun(void* ecx, void* edx, IGameEvent* pEvent)
	{
		static auto FireEventClientSide = GameEvents.GetOriginalFunc<FireEvent>(9);
		if (!pEvent)
			return FireEventClientSide(ecx, pEvent);

		const char* szEventName = pEvent->GetName();


		if (!strcmp(szEventName, "game_newmap"))
		{
			G.KillDelayEnd = false;
			G.KillDelayTime = 0;
		}

		if (!strcmp(szEventName, "player_death"))
		{
			if (g_EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker")) == g_EngineClient->GetLocalPlayer())
			{
				G.KillDelayTime = g_GlobalVars->curtime + Legitbot.KillDelayTime;
				std::string weapon = pEvent->GetString("weapon");
			}
		}
		return FireEventClientSide(ecx, pEvent);
	}
	//--------------------------------------------------------------------------------
	void __fastcall scene_end(void* thisptr, void* edx)
	{
		static auto ofunc = clientmode_hook.GetOriginalFunc<SceneEnd>(index::SceneEnd);

		ofunc(thisptr, edx);

		Chams::Get().OnSceneEnd();

	}
	//--------------------------------------------------------------------------------


	auto dwCAM_Think = Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "85 C0 75 30 38 86");
	typedef bool(__thiscall *svc_get_bool_t)(PVOID);
	bool __fastcall hkSvCheatsGetBool(PVOID pConVar, void* edx)
	{
		static auto ofunc = sv_cheats.GetOriginalFunc<svc_get_bool_t>(13);
		if (!ofunc)
			return false;

		if (reinterpret_cast<DWORD>(_ReturnAddress()) == reinterpret_cast<DWORD>(dwCAM_Think))
			return true;
		return ofunc(pConVar);
	}

	EGCResults __fastcall RetrieveMessageGC(void* ecx, void* edx, uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32_t *pcubMsgSize) {
		static auto RMGC_O = gamecoordinator_hook.GetOriginalFunc<RetrieveMessage_t>(2);
		EGCResults status;
		status = RMGC_O(ecx, edx, punMsgType, pubDest, cubDest, pcubMsgSize);
		std::ostringstream ss;
		ss << "[->] Message got from GC: " << (*punMsgType & 0x7FFFFFFF) << "\"";
		std::cout << ss.str() << std::endl;
		if (status != k_EGCResultOK) {
			return status;
		}
		Profile::Get().OnRecieveMessage(punMsgType, pubDest, cubDest, pcubMsgSize);
		return status;
	}
	EGCResults __fastcall SendMessageGC(void* ecx, void* edx, uint32_t unMsgType, const void* pubData, uint32_t cubData) {
		static auto SMGC_O = gamecoordinator_hook.GetOriginalFunc<SendMessageGC_t>(0);
		EGCResults status;
		bool sendMessage = true;
		std::ostringstream ss;
		ss << "[<-] Message sent to GC: " << (unMsgType & 0x7FFFFFFF) << "\"";
		std::cout << ss.str() << std::endl;
		sendMessage = Profile::Get().OnSendMessage(unMsgType, (void*)pubData, cubData);
		if (!sendMessage) { return k_EGCResultOK; }

		status = SMGC_O(ecx, edx, unMsgType, pubData, cubData);

		if (status != k_EGCResultOK) {
			return status;
		}
		//if (unMsgType & 0x7FFFFFFF == k_EMsgGCCStrike15_v2_MatchmakingStop) { LobbyLord::Get().SendClientHello(); }

		return status;
	}
}
