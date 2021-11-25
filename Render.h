#pragma once
#include "Engine.h"
#include "PizdaDrawList.h"
#include "PizdaFont.h"
#include <memory>
#include <unordered_map>

#define D3DFVF_CUSTOM_TEXT ( D3DFVF_XYZ | D3DFVF_DIFFUSE  )
#define D3DFVF_CUSTOM_VERTEX ( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 )
#define DEFCOLOR_SRC(name, r, g, b) \
	static Color name##(uint8_t a = 255){ return Color(r, g, b, a); }

class PizdaFont;
class PizdaDrawList;
struct UVTable;

class CRender : public Singleton<CRender>
{
	friend class																			Singleton<CRender>;
	CRender() = default;
	~CRender();
	LPDIRECT3DSTATEBLOCK9																	m_pStateBlockDraw;
	DWORD																					dwOld_D3DRS_COLORWRITEENABLE,
		dwOld_D3DRS_SRGBWRITEENABLE;
	int																						g_VertexBufferSize,
		g_IndexBufferSize;
public:
	LPDIRECT3DTEXTURE9																		CursorsTexture;
	UVTable																					MiscUvs[8];
	LPDIRECT3DDEVICE9																		m_pDevice;
	unordered_map<string, PizdaFont*>														font_map;
	D3DMATRIX																				last_world,
		last_view,
		last_projection;
	LPDIRECT3DVERTEXBUFFER9																	_VertexBuffer;
	LPDIRECT3DINDEXBUFFER9																	_IndexBuffer;
	//LPDIRECT3DSURFACE9																		backbuffer;
	//D3DSURFACE_DESC																			desc;
	//LPDIRECT3DPIXELSHADER9																	pixelshader;
	//LPDIRECT3DTEXTURE9																		sil_texture;
	//LPDIRECT3DSURFACE9																		ppSurfaceLevel;
	unordered_map<uint32_t, pair<LPDIRECT3DTEXTURE9, LPDIRECT3DTEXTURE9> >					wtex_map;
public:
	void																					Initialize(IDirect3DDevice9* pDevice);
	void																					OnLostDevice();
	void																					OnResetDevice();
	void																					BeginRender();
	void																					EndRender();
	void																					RenderDrawList(PizdaDrawList* list);
private:
	bool																					CreateObject();
	void																					SetVertexState();
};


namespace D3D9
{
	enum TABLE
	{
		QueryInterface,
		AddRef,
		Release,
		TestCooperativeLevel,
		GetAvailableTextureMem,
		EvictManagedResources,
		GetDirect3D,
		GetDeviceCaps,
		GetDisplayMode,
		GetCreationParameters,
		SetCursorProperties,
		SetCursorPosition,
		ShowCursor,
		CreateAdditionalSwapChain,
		GetSwapChain,
		GetNumberOfSwapChains,
		Reset,
		Present,
		GetBackBuffer,
		GetRasterStatus,
		SetDialogBoxMode,
		SetGammaRamp,
		GetGammaRamp,
		CreateTexture,
		CreateVolumeTexture,
		CreateCubeTexture,
		CreateVertexBuffer,
		CreateIndexBuffer,
		CreateRenderTarget,
		CreateDepthStencilSurface,
		UpdateSurface,
		UpdateTexture,
		GetRenderTargetData,
		GetFrontBufferData,
		StretchRect,
		ColorFill,
		CreateOffscreenPlainSurface,
		SetRenderTarget,
		GetRenderTarget,
		SetDepthStencilSurface,
		GetDepthStencilSurface,
		BeginScene,
		EndScene,
		Clear,
		SetTransform,
		GetTransform,
		MultiplyTransform,
		SetViewport,
		GetViewport,
		SetMaterial,
		GetMaterial,
		SetLight,
		GetLight,
		LightEnable,
		GetLightEnable,
		SetClipPlane,
		GetClipPlane,
		SetRenderState,
		GetRenderState,
		CreateStateBlock,
		BeginStateBlock,
		EndStateBlock,
		SetClipStatus,
		GetClipStatus,
		GetTexture,
		SetTexture,
		GetTextureStageState,
		SetTextureStageState,
		GetSamplerState,
		SetSamplerState,
		ValidateDevice,
		SetPaletteEntries,
		GetPaletteEntries,
		SetCurrentTexturePalette,
		GetCurrentTexturePalette,
		SetScissorRect,
		GetScissorRect,
		SetSoftwareVertexProcessing,
		GetSoftwareVertexProcessing,
		SetNPatchMode,
		GetNPatchMode,
		DrawPrimitive,
		DrawIndexedPrimitive,
		DrawPrimitiveUP,
		DrawIndexedPrimitiveUP,
		ProcessVertices,
		CreateVertexDeclaration,
		SetVertexDeclaration,
		GetVertexDeclaration,
		SetFVF,
		GetFVF,
		CreateVertexShader,
		SetVertexShader,
		GetVertexShader,
		SetVertexShaderConstantF,
		GetVertexShaderConstantF,
		SetVertexShaderConstantI,
		GetVertexShaderConstantI,
		SetVertexShaderConstantB,
		GetVertexShaderConstantB,
		SetStreamSource,
		GetStreamSource,
		SetStreamSourceFreq,
		GetStreamSourceFreq,
		SetIndices,
		GetIndices,
		CreatePixelShader,
		SetPixelShader,
		GetPixelShader,
		SetPixelShaderConstantF,
		GetPixelShaderConstantF,
		SetPixelShaderConstantI,
		GetPixelShaderConstantI,
		SetPixelShaderConstantB,
		GetPixelShaderConstantB,
		DrawRectPatch,
		DrawTriPatch,
		DeletePatch,
		CreateQuery
	};
}
