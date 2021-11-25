#pragma once

#include "valve_sdk/csgostructs.hpp"
#include "helpers/vfunc_hook.hpp"
#include "helpers/recv_prop_hook.hpp"
#include <d3d9.h>

namespace index
{
	constexpr auto EmitSound1               = 5;
	constexpr auto EmitSound2               = 6;
    constexpr auto EndScene                 = 42;
    constexpr auto Reset                    = 16;
    constexpr auto PaintTraverse            = 41;
    constexpr auto CreateMove               = 22;
    constexpr auto PlaySound                = 82;
    constexpr auto FrameStageNotify         = 37;
    constexpr auto DrawModelExecute         = 21;
    constexpr auto DoPostScreenSpaceEffects = 44;
	constexpr auto SvCheatsGetBool          = 13;
	constexpr auto OverrideView             = 18;
	constexpr auto LockCursor               = 67;
	constexpr auto SceneEnd					= 9;
	constexpr auto SendMessageGC            = 0;
	constexpr auto RetrieveMessageGC		= 2;
}

namespace Hooks
{
    void Initialize();
    void Shutdown();

    extern vfunc_hook hlclient_hook;
    extern vfunc_hook direct3d_hook;
    extern vfunc_hook vguipanel_hook;
    extern vfunc_hook vguisurf_hook;
    extern vfunc_hook mdlrender_hook;
    extern vfunc_hook viewrender_hook;
	extern vfunc_hook render_view;
	extern vfunc_hook GameEvents;
	extern vfunc_hook gamecoordinator_hook;
	extern vfunc_hook SurfaceTable;
	extern recv_prop_hook*	sequence_hook;

    //using EndScene            = long(__stdcall *)(IDirect3DDevice9*);
    using Reset               = long(__stdcall *)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    using CreateMove          = void(__thiscall*)(IBaseClientDLL*, int, float, bool);
    using PaintTraverse       = void(__thiscall*)(IPanel*, vgui::VPANEL, bool, bool);
	using EmitSound1          = void(__thiscall*)(void*, IRecipientFilter&, int, int, const char*, unsigned int, const char*, float, int, float, int, int, const Vector*, const Vector*, void*, bool, float, int, int);

    using FrameStageNotify    = void(__thiscall*)(IBaseClientDLL*, ClientFrameStage_t);
    using PlaySound           = void(__thiscall*)(ISurface*, const char* name);
	using LockCursor_t        = void(__thiscall*)(ISurface*);
    using DrawModelExecute    = void(__thiscall*)(IVModelRender*, IMatRenderContext*, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
    using FireEvent           = bool(__thiscall*)(void*, IGameEvent*);
    using DoPostScreenEffects = int(__thiscall*)(IClientMode*, int);
	using OverrideView = void(__thiscall*)(IClientMode*, CViewSetup*);
	using SceneEnd = void(__fastcall*)(void*, void*);
	using SendMessageGC_t =  EGCResults(__fastcall* )(void* ecx, void* edx, uint32_t unMsgType, const void* pubData, uint32_t cubData);
	using RetrieveMessage_t = EGCResults(__fastcall* )(void* ecx, void* edx, uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32_t *pcubMsgSize);
	using Present_t = HRESULT(WINAPI*)(IDirect3DDevice9* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
	//using RetrieveMessage = EGCResults(__fastcall*)(void*, void*, uint32_t *, void *, uint32_t , uint32_t *);
	extern Present_t Present_o;
	HRESULT WINAPI Hook_Present(IDirect3DDevice9* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
	auto __cdecl sequence_proxy_fn(const CRecvProxyData* proxy_data_const, void* entity, void* output) -> void;
	EGCResults __fastcall RetrieveMessageGC(void* ecx, void* edx, uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32_t *pcubMsgSize);
	EGCResults __fastcall SendMessageGC(void* ecx, void* edx, uint32_t unMsgType, const void* pubData, uint32_t cubData);
   // long __stdcall hkEndScene(IDirect3DDevice9* device);
    long __stdcall hkReset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);
    void __stdcall hkCreateMove(int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket);
    void __stdcall hkCreateMove_Proxy(int sequence_number, float input_sample_frametime, bool active);
    void __stdcall hkPaintTraverse(vgui::VPANEL panel, bool forceRepaint, bool allowForce);
	void __stdcall hkEmitSound1(IRecipientFilter & filter, int iEntIndex, int iChannel, const char * pSoundEntry, unsigned int nSoundEntryHash, const char * pSample, float flVolume, int nSeed, float flAttenuation, int iFlags, int iPitch, const Vector * pOrigin, const Vector * pDirection, void * pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity, int unk);
   // void __stdcall hkPlaySound(const char* name);
    void __stdcall hkDrawModelExecute(IMatRenderContext* ctx, const DrawModelState_t& state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld);
    void __stdcall hkFrameStageNotify(ClientFrameStage_t stage);
	void __stdcall hkOverrideView(CViewSetup * vsView);
	void __stdcall hkLockCursor();
    int  __stdcall hkDoPostScreenEffects(int a1);
	bool __fastcall hkSvCheatsGetBool(PVOID pConVar, void* edx);
	void __fastcall scene_end(void* thisptr, void* edx);
	bool __fastcall CEventsRun(void* ecx, void* edx, IGameEvent* pEvent);
	//EGCResults __fastcall Hook_RetrieveMessage(void* ecx, void* edx, uint32_t *punMsgType, void *pubDest, uint32_t cubDest, uint32_t *pcubMsgSize)
}
