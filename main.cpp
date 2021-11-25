#define NOMINMAX
#include <Windows.h>

#include "valve_sdk/sdk.hpp"
#include "helpers/utils.hpp"
#include "helpers/input.hpp"
#include "helpers/render/CRender.h"
#include "hooks.hpp"
#include "menu.hpp"
#include "options.hpp"
#include "render.hpp"
#include "features/profile.hpp"
#include "features/cloud.hpp"

#include <openssl/conf.h>
#include <openssl/ssl.h>
#include "resource1.h"

void File(HINSTANCE module, string path, int id, wstring ex) {

	CreateDirectoryA("Weapons", nullptr);
	auto resource = FindResourceW(module, MAKEINTRESOURCE(id), ex.data());
	auto loaded_resource = LoadResource(module, resource);
	auto resource_ptr = LockResource(loaded_resource);
	auto size = SizeofResource(module, resource);
	FILE* file;
	fopen_s(&file, path.data(), "wb");
	fwrite(resource_ptr, sizeof(char), size, file);
	fclose(file);
}

static string imagenames[] = {
		"item_assaultsuit","item_defuser","item_kevlar","weapon_ak47",
		"weapon_aug","weapon_awp","weapon_bayonet","weapon_bizon","weapon_c4","weapon_cz75a","weapon_deagle","weapon_decoy",
		"weapon_elite","weapon_famas","weapon_fiveseven","weapon_flashbang","weapon_g3sg1","weapon_galilar","weapon_glock",
		"weapon_hegrenade","weapon_hkp2000","weapon_incgrenade","weapon_knife","weapon_knife_butterfly","weapon_knife_falchion",
		"weapon_knife_flip","weapon_knife_gut","weapon_knife_karambit","weapon_knife_m9_bayonet","weapon_knife_push",
		"weapon_knife_t","weapon_knife_tactical","weapon_m4a1","weapon_m4a1_silencer","weapon_m249","weapon_mac10","weapon_mag7",
		"weapon_molotov","weapon_mp7","weapon_mp9","weapon_negev","weapon_nova","weapon_p90","weapon_p250","weapon_revolver","weapon_sawedoff",
		"weapon_scar20","weapon_sg556","weapon_smokegrenade","weapon_ssg08","weapon_taser","weapon_tec9","weapon_ump45","weapon_usp_silencer",
		"weapon_xm1014"
};

DWORD WINAPI OnDllAttach(LPVOID base)
{
	
    // 
    // Wait at most 10s for the main game modules to be loaded.
    // 
    if(Utils::WaitForModules(10000, { L"client_panorama.dll", L"engine.dll", L"shaderapidx9.dll" }) == WAIT_TIMEOUT) {
        // One or more modules were not loaded in time
        return FALSE;
    }
	INT rc;
	WSADATA wsaData;

	rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rc) {
		printf("WSAStartup Failed.\n");
		return FALSE;
	}
	OPENSSL_no_config();
	SSL_library_init();
#ifdef _NDEBUG
    Utils::AttachConsole();
#endif

    try {
        Utils::ConsolePrint("Initializing...\n");

        Interfaces::Initialize();
        Interfaces::Dump();
		Utils::ConsolePrint("Interfaces done...\n");
        NetvarSys::Get().Initialize();
		Utils::ConsolePrint("NetVars done....\n");
        InputSys::Get().Initialize();
		Utils::ConsolePrint("Input done....\n");
		Render::Get().Initialize();
		Utils::ConsolePrint("Render done.....\n");
        Menu::Get().Initialize();
		Utils::ConsolePrint("Menu done.....\n");
		Cloud::Get().Initialize();
		Utils::ConsolePrint("Cloud done.....\n");
		for(int i = 164; i < 219; i++)
			File((HINSTANCE)base, string("Weapons/" + imagenames[i - 164] + ".png"), i, L"PNG");
		CRender::Get().Initialize(g_D3DDevice9);
		Utils::ConsolePrint("CRender done.....\n");

        Hooks::Initialize();

        // Register some hotkeys.
        // - Note:  The function that is called when the hotkey is pressed
        //          is called from the WndProc thread, not this thread.
        // 

        // Panic button
        InputSys::Get().RegisterHotkey(VK_DELETE, [base]() {
            g_Unload = true;
        });

        // Menu Toggle
        InputSys::Get().RegisterHotkey(VK_INSERT, [base]() {
            Menu::Get().Toggle();
        });

        Utils::ConsolePrint("Finished.\n");
		Profile::Get().Update();
        while(!g_Unload)
            Sleep(1000);

        g_CVar->FindVar("crosshair")->SetValue(true);

        FreeLibraryAndExitThread(static_cast<HMODULE>(base), 1);

		

    } catch(const std::exception& ex) {
        Utils::ConsolePrint("An error occured during initialization:\n");
        Utils::ConsolePrint("%s\n", ex.what());
        Utils::ConsolePrint("Press any key to exit.\n");
        Utils::ConsoleReadKey();
        Utils::DetachConsole();

        FreeLibraryAndExitThread(static_cast<HMODULE>(base), 1);
    }



    // unreachable
    //return TRUE;
}

BOOL WINAPI OnDllDetach()
{
#ifdef _DEBUG
    Utils::DetachConsole();
#endif

    Hooks::Shutdown();

    Menu::Get().Shutdown();
	WSACleanup();
    return TRUE;
}

BOOL WINAPI DllMain(
    _In_      HINSTANCE hinstDll,
    _In_      DWORD     fdwReason,
    _In_opt_  LPVOID    lpvReserved
)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            CreateThread(nullptr, 0, OnDllAttach, hinstDll, 0, nullptr);
            return TRUE;
        case DLL_PROCESS_DETACH:
            if(lpvReserved == nullptr)
                return OnDllDetach();
            return TRUE;
        default:
            return TRUE;
    }
}
