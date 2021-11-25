#pragma once
#include <string>
#include <sstream>
#include <stdint.h>
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#include "features/visuals.hpp"
#include "singleton.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/directx9/imgui_impl_dx9.h"

#include "valve_sdk/misc/Color.hpp"

extern ImFont* g_pDefaultFont;
extern ImFont* g_pC4Font;
extern ImFont* g_pIconFont;



class Vector;

class Render
	: public Singleton<Render>
{
	friend class Singleton<Render>;

	Render();

private:
	PizdaDrawList* draw_list_rendering;
	PizdaDrawList* draw_list;

	ImU32 GetU32(Color _color)
	{
		return ((_color[3] & 0xff) << 24) + ((_color[2] & 0xff) << 16) + ((_color[1] & 0xff) << 8)
			+ (_color[0] & 0xff);
	}
	std::string spectators_temp = "";
public:
	PizdaDrawList* draw_list_act;
	int specs = 0;
	std::string spectators = "";
	void Initialize();
	void GetFonts();
	void ClearDrawList();
	PizdaDrawList * RenderScene();
	void SaveState(IDirect3DDevice9 * pDevice);
	void RestoreState(IDirect3DDevice9 * pDevice);
	void DrawWave(Vector loc, float radius, Color color);
	void BeginScene();
};