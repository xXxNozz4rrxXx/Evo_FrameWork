#include "sdk.hpp"

#include "../Helpers/Utils.hpp"
#include "..\helpers\kit_parser.hpp"
#include "..\Parser.h"

DWORD C_LocalPlayer::xuid_low, C_LocalPlayer::xuid_high;
IVEngineClient*       	 g_EngineClient = nullptr;
IBaseClientDLL*       	 g_CHLClient = nullptr;
IClientEntityList*    	 g_EntityList = nullptr;
CGlobalVarsBase*      	 g_GlobalVars = nullptr;
IEngineTrace*         	 g_EngineTrace = nullptr;
ICvar*                	 g_CVar = nullptr;
IPanel*               	 g_VGuiPanel = nullptr;
IClientMode*          	 g_ClientMode = nullptr;
IVDebugOverlay*       	 g_DebugOverlay = nullptr;
ISurface*             	 g_VGuiSurface = nullptr;
CInput*               	 g_Input = nullptr;
IVModelInfoClient*    	 g_MdlInfo = nullptr;
IVModelRender*        	 g_MdlRender = nullptr;
IVRenderView*         	 g_RenderView = nullptr;
IMaterialSystem*      	 g_MatSystem = nullptr;
IGameEventManager2*   	 g_GameEvents = nullptr;
IMoveHelper*          	 g_MoveHelper = nullptr;
IMDLCache*            	 g_MdlCache = nullptr;
IPrediction*          	 g_Prediction = nullptr;
CGameMovement*        	 g_GameMovement = nullptr;
IEngineSound*         	 g_EngineSound = nullptr;
CGlowObjectManager*   	 g_GlowObjManager = nullptr;
IViewRender*          	 g_ViewRender = nullptr;
IDirect3DDevice9*     	 g_D3DDevice9 = nullptr;
CClientState*         	 g_ClientState = nullptr;
IPhysicsSurfaceProps* 	 g_PhysSurface = nullptr;
C_LocalPlayer         	 g_LocalPlayer;
CCSGameRules**		  	 g_GameRules = nullptr;
ILocalize*				 g_Localize = nullptr;
ISteamClient *			 g_SteamClient = nullptr;
ISteamHTTP*				 g_SteamHTTP = nullptr;
ISteamUser*				 g_SteamUser = nullptr;
ISteamFriends*			 g_SteamFriends = nullptr;
ISteamInventory*		 g_SteamInventory = nullptr;
ISteamGameCoordinator*	 g_GameCoordinator = nullptr;
ISteamMatchmaking*		 g_SteamMatchMaking = nullptr;


namespace Interfaces
{
	CreateInterfaceFn get_module_factory(HMODULE module)
	{
		return reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module, "CreateInterface"));
	}

	template<typename T>
	T* get_interface(CreateInterfaceFn f, const char* szInterfaceVersion)
	{
		auto result = reinterpret_cast<T*>(f(szInterfaceVersion, nullptr));

		if (!result) {
			throw std::runtime_error(std::string("[get_interface] Failed to GetOffset interface: ") + szInterfaceVersion);
		}

		return result;
	}

	void Initialize()
	{
		auto engineFactory = get_module_factory(GetModuleHandleW(L"engine.dll"));
		auto clientFactory = get_module_factory(GetModuleHandleW(L"client_panorama.dll"));
		auto valveStdFactory = get_module_factory(GetModuleHandleW(L"vstdlib.dll"));
		auto vguiFactory = get_module_factory(GetModuleHandleW(L"vguimatsurface.dll"));
		auto vgui2Factory = get_module_factory(GetModuleHandleW(L"vgui2.dll"));
		auto matSysFactory = get_module_factory(GetModuleHandleW(L"materialsystem.dll"));
		auto dataCacheFactory = get_module_factory(GetModuleHandleW(L"datacache.dll"));
		auto vphysicsFactory = get_module_factory(GetModuleHandleW(L"vphysics.dll"));
		auto hSteamUser = ((HSteamUser(__cdecl*)(void))GetProcAddress(GetModuleHandle(L"steam_api.dll"), "SteamAPI_GetHSteamUser"))();
		auto hSteamPipe = ((HSteamPipe(__cdecl*)(void))GetProcAddress(GetModuleHandle(L"steam_api.dll"), "SteamAPI_GetHSteamPipe"))();

		g_CHLClient = get_interface<IBaseClientDLL>(clientFactory, "VClient018");
		g_EntityList = get_interface<IClientEntityList>(clientFactory, "VClientEntityList003");
		g_Prediction = get_interface<IPrediction>(clientFactory, "VClientPrediction001");
		g_GameMovement = get_interface<CGameMovement>(clientFactory, "GameMovement001");
		g_MdlCache = get_interface<IMDLCache>(dataCacheFactory, "MDLCache004");
		g_EngineClient = get_interface<IVEngineClient>(engineFactory, "VEngineClient014");
		g_MdlInfo = get_interface<IVModelInfoClient>(engineFactory, "VModelInfoClient004");
		g_MdlRender = get_interface<IVModelRender>(engineFactory, "VEngineModel016");
		g_RenderView = get_interface<IVRenderView>(engineFactory, "VEngineRenderView014");
		g_EngineTrace = get_interface<IEngineTrace>(engineFactory, "EngineTraceClient004");
		g_DebugOverlay = get_interface<IVDebugOverlay>(engineFactory, "VDebugOverlay004");
		g_GameEvents = get_interface<IGameEventManager2>(engineFactory, "GAMEEVENTSMANAGER002");
		g_EngineSound = get_interface<IEngineSound>(engineFactory, "IEngineSoundClient003");
		g_MatSystem = get_interface<IMaterialSystem>(matSysFactory, "VMaterialSystem080");
		g_CVar = get_interface<ICvar>(valveStdFactory, "VEngineCvar007");
		g_VGuiPanel = get_interface<IPanel>(vgui2Factory, "VGUI_Panel009");
		g_VGuiSurface = get_interface<ISurface>(vguiFactory, "VGUI_Surface031");
		g_PhysSurface = get_interface<IPhysicsSurfaceProps>(vphysicsFactory, "VPhysicsSurfaceProps001");
		g_Localize = get_interface<ILocalize>(get_module_factory(GetModuleHandleW(L"localize.dll")), "Localize_001");
		g_SteamClient = ((ISteamClient*(__cdecl*)(void))GetProcAddress(GetModuleHandle(L"steam_api.dll"), "SteamClient"))();
		g_SteamHTTP = g_SteamClient->GetISteamHTTP(hSteamUser, hSteamPipe, "STEAMHTTP_INTERFACE_VERSION002");
		g_SteamUser = g_SteamClient->GetISteamUser(hSteamUser, hSteamPipe, "SteamUser019");
		g_SteamFriends = g_SteamClient->GetISteamFriends(hSteamUser, hSteamPipe, "SteamFriends015");
		g_SteamInventory = g_SteamClient->GetISteamInventory(hSteamUser, hSteamPipe, "STEAMINVENTORY_INTERFACE_V002");
		g_GameCoordinator = (ISteamGameCoordinator*)g_SteamClient->GetISteamGenericInterface(hSteamUser, hSteamPipe, "SteamGameCoordinator001");
		g_SteamMatchMaking = g_SteamClient->GetISteamMatchmaking(hSteamUser, hSteamPipe, STEAMMATCHMAKING_INTERFACE_VERSION);

		auto client = GetModuleHandleW(L"client_panorama.dll");
		auto engine = GetModuleHandleW(L"engine.dll");
		auto dx9api = GetModuleHandleW(L"shaderapidx9.dll");

		g_GlobalVars = **(CGlobalVarsBase * **)((*(DWORD * *)g_CHLClient)[0] + 0x1B);
		g_ClientMode = *(IClientMode * *)(Utils::PatternScan(client, "B9 ? ? ? ? E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 53") + 1);
		g_Input = *(CInput**)(Utils::PatternScan(client, "B9 ? ? ? ? F3 0F 11 04 24 FF 50 10") + 1);
		g_MoveHelper = **(IMoveHelper***)(Utils::PatternScan(client, "8B 0D ? ? ? ? 8B 45 ? 51 8B D4 89 02 8B 01") + 2);
		g_GlowObjManager = *(CGlowObjectManager**)(Utils::PatternScan(client, "0F 11 05 ? ? ? ? 83 C8 01 C7 05 ? ? ? ? 00 00 00 00") + 3);
		g_ViewRender = *(IViewRender**)(Utils::PatternScan(client, "A1 ? ? ? ? B9 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? FF 10") + 1);
		g_D3DDevice9 = **(IDirect3DDevice9***)(Utils::PatternScan(dx9api, "A1 ? ? ? ? 50 8B 08 FF 51 0C") + 1);
		g_ClientState = **(CClientState***)(Utils::PatternScan(engine, "A1 ? ? ? ? 8B 80 ? ? ? ? C3") + 1);
		g_GameRules = *reinterpret_cast<CCSGameRules***>(Utils::PatternScan(client, "A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 80 B8 ? ? ? ? ? 0F 84 ? ? ? ? 0F 10 05") + 1);
		g_LocalPlayer = *(C_LocalPlayer*)(Utils::PatternScan(client, "8B 0D ? ? ? ? 83 FF FF 74 07") + 2);
		game_data::initialize_kits();
	}

	void Dump()
	{
		// Ugly macros ugh
#define STRINGIFY_IMPL(s) #s
#define STRINGIFY(s)      STRINGIFY_IMPL(s)
#define PRINT_INTERFACE(name) Utils::ConsolePrint("%-20s: %p\n", STRINGIFY(name), name)

		PRINT_INTERFACE(g_CHLClient);
		PRINT_INTERFACE(g_EntityList);
		PRINT_INTERFACE(g_Prediction);
		PRINT_INTERFACE(g_GameMovement);
		PRINT_INTERFACE(g_MdlCache);
		PRINT_INTERFACE(g_EngineClient);
		PRINT_INTERFACE(g_MdlInfo);
		PRINT_INTERFACE(g_MdlRender);
		PRINT_INTERFACE(g_RenderView);
		PRINT_INTERFACE(g_EngineTrace);
		PRINT_INTERFACE(g_DebugOverlay);
		PRINT_INTERFACE(g_GameEvents);
		PRINT_INTERFACE(g_EngineSound);
		PRINT_INTERFACE(g_MatSystem);
		PRINT_INTERFACE(g_CVar);
		PRINT_INTERFACE(g_VGuiPanel);
		PRINT_INTERFACE(g_VGuiSurface);
		PRINT_INTERFACE(g_PhysSurface);
		PRINT_INTERFACE(g_Localize);
	}
}