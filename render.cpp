#include "render.hpp"

#include <mutex>

#include "features/visuals.hpp"
#include "valve_sdk/csgostructs.hpp"
#include "helpers/input.hpp"
#include "menu.hpp"
#include "options.hpp"
#include "droid.hpp"
#include "helpers/math.hpp"

ImFont* g_pDefaultFont;
ImFont* g_pC4Font;
ImFont* g_pIconFont;

//CRITICAL_SECTION render_cs;

Render::Render()
{
	//InitializeCriticalSection(&render_cs);

}


std::mutex render_mutex;

void Render::Initialize()
{
	ImGui::CreateContext();

	ImGui_ImplDX9_Init(InputSys::Get().GetMainWindow(), g_D3DDevice9);


	draw_list = new PizdaDrawList();
	draw_list_act = new PizdaDrawList();
	draw_list_rendering = new PizdaDrawList();

	GetFonts();
}

void Render::GetFonts() {
	ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Droid_compressed_data, Droid_compressed_size, 14.f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
	g_pDefaultFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Droid_compressed_data, Droid_compressed_size, 18.f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
	g_pC4Font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(Droid_compressed_data, Droid_compressed_size, 32.f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

	g_pIconFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(undefeated_compressed_data, undefeated_compressed_size, 14.f);
	ImGui::GetIO().Fonts->Build();
}

void Render::ClearDrawList() {
	render_mutex.lock();
	draw_list_act->Clear();
	render_mutex.unlock();
}


void Render::BeginScene()
{
	draw_list_act->Clear();

	static int alpha;
	if (Menu::Get().IsVisible() && alpha < 155)
		alpha += 5;
	else if (!Menu::Get().IsVisible() && alpha > 0)
		alpha -= 5;

	int screenWidth, screenHeight;
	g_EngineClient->GetScreenSize(screenWidth, screenHeight);
	draw_list_act->AddRectangleFilled(Vector2D(0, 0), Vector2D(screenWidth, screenHeight), Color(0, 0, 0, alpha));
	 

	if (g_Options.misc_watermark) //lol
		draw_list_act->AddText(Vector2D(10, 5), false, false, Utils::Float3ToClr(g_Options.color_watermark), "Calibri", 14, "Ev0 Framework");

	if (g_EngineClient->IsInGame() && g_LocalPlayer && g_Options.esp_enabled)
		Visuals::Get().AddToDrawList();

	if (g_EngineClient->IsInGame() && g_LocalPlayer && g_Options.misc_speclist) {
		spectators_temp.clear();
		int specs = 0;
		auto pLocalEntity = g_LocalPlayer;
		for (int i = 1; i < g_EngineClient->GetMaxClients(); i++)
		{
			auto pBaseEntity = C_BasePlayer::GetPlayerByIndex(i);
			player_info_t pinfo;

			// The entity isn't some laggy peice of shit or something
			if (pBaseEntity &&  pBaseEntity != pLocalEntity)
			{
				if (!pLocalEntity)
					continue;
				if (g_EngineClient->GetPlayerInfo(i, &pinfo) && !pBaseEntity->m_iHealth() > 0 && !pBaseEntity->IsDormant())
				{
					auto obs = pBaseEntity->m_hObserverTarget().Get();

					if (obs)
					{
						auto pTarget = obs;
						player_info_t pinfo2;
						if (pTarget)
						{
							if (g_EngineClient->GetPlayerInfo(pTarget->GetIndex(), &pinfo2))
							{
								if (g_Options.misc_speclist_you)
								{
									if (pTarget->GetIndex() == pLocalEntity->GetIndex())
									{
										spectators_temp += pinfo.szName;
										spectators_temp += "->";
										spectators_temp += pinfo2.szName;
										spectators_temp += "\n";
										specs++;
									}
								}
								else
								{
									spectators_temp += pinfo.szName;
									spectators_temp += "->";
									spectators_temp += pinfo2.szName;
									spectators_temp += "\n";
									specs++;
								}
							}
						}
					}
				}
			}
		}
	}

	static auto NadePrediction = []() {
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
		if (!g_LocalPlayer || !g_Options.misc_nade_pred)
			return;
		C_BaseCombatWeapon* pWeapon = g_LocalPlayer->m_hActiveWeapon().Get();
		const float TIMEALIVE = 5.f;
		const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.4f;

		float fStep = 0.1f;
		float fGravity = 800.0f / 8.f;

		auto GP = g_Options.color_nade_pred;
		auto G = Color(int(GP[0] * 255.0f), int(GP[1] * 255.0f), int(GP[2] * 255.0f));
		Vector vPos;
		Vector vStart;
		QAngle vThrow;
		Vector vThrow2;
		int iCollisions = 0;

		QAngle vViewAngles;
		g_EngineClient->GetViewAngles(vViewAngles);

		vThrow[0] = vViewAngles[0];
		vThrow[1] = vViewAngles[1];
		vThrow[2] = vViewAngles[2];

		if (vThrow[0] < 0)
			vThrow[0] = -10 + vThrow[0] * ((90 - 10) / 90.0);
		else
			vThrow[0] = -10 + vThrow[0] * ((90 + 10) / 90.0);

		float fVel = (90 - vThrow[0]) * 4;
		if (fVel > 500)
			fVel = 500;

		Math::AngleVectors(vThrow, vThrow2);

		Vector vEye = g_LocalPlayer->GetEyePos();
		vStart[0] = vEye[0] + vThrow2[0] * 16;
		vStart[1] = vEye[1] + vThrow2[1] * 16;
		vStart[2] = vEye[2] + vThrow2[2] * 16;

		vThrow2[0] = (vThrow2[0] * fVel) + g_LocalPlayer->m_vecVelocity()[0];
		vThrow2[1] = (vThrow2[1] * fVel) + g_LocalPlayer->m_vecVelocity()[1];
		vThrow2[2] = (vThrow2[2] * fVel) + g_LocalPlayer->m_vecVelocity()[2]; // casualhacker for da magic calc help

		for (float fTime = 0.0f; fTime < TIMEALIVE; fTime += fStep)
		{
			vPos = vStart + vThrow2 * fStep;

			Ray_t ray;
			trace_t tr;

			CTraceFilter loc;
			loc.pSkip = g_LocalPlayer;

			ray.Init(vStart, vPos);
			g_EngineTrace->TraceRay(ray, MASK_SOLID, &loc, &tr);// FIX
			static auto DotProduct = [](const Vector& a, const Vector& b)
			{
				return(a.x*b.x + a.y*b.y + a.z*b.z);
			};
			if (tr.DidHit()) // if the trace hits a surface
			{
				//float proj = DotProduct(throwvec, tr.plane.normal);
				vThrow2 = tr.plane.normal * -2.0f  * DotProduct(vThrow2, tr.plane.normal) + vThrow2;
				vThrow2 *= GRENADE_COEFFICIENT_OF_RESTITUTION;

				iCollisions++;
				if (iCollisions > 2)
					break;

				vPos = vStart + vThrow2 * tr.fraction*fStep;

				fTime += (fStep * (1 - tr.fraction));
			} // end of bounce

			Vector vOutStart, vOutEnd;
			if (g_LocalPlayer->m_hActiveWeapon().Get() && g_LocalPlayer->m_hActiveWeapon().Get()->IsGrenade())
			{
				if (!g_DebugOverlay->ScreenPosition(vStart, vOutStart) && !g_DebugOverlay->ScreenPosition(vPos, vOutEnd))
				{
					Render::Get().draw_list_act->AddLine(Vector2D(vOutStart.x, vOutStart.y), Vector2D(vOutEnd.x, vOutEnd.y), G, 1.0f);
				}

				vStart = vPos;
				vThrow2.z -= fGravity * tr.fraction*fStep;
			}
		}
	};
	if (g_EngineClient->IsInGame() && g_LocalPlayer)
		NadePrediction();

	if (g_EngineClient->IsInGame() && g_LocalPlayer &&g_Options.esp_sound)
		for (auto& elem : Visuals::Get().Sound) {
			static float Step = M_PI * 2.0f / 40; //step size
			Vector prev;
			Vector tmp;
			int j = 0;
			vector<Vector2D> points;
			points.resize(45);
			if (!g_DebugOverlay->ScreenPosition(elem.vOrigin, tmp)) {

				for (float lat = 0; lat <= M_PI * 2.0f + M_PI / 12; lat += Step)//360		
				{


					float sin1 = sin(lat);
					float cos1 = cos(lat);


					float sin3 = sin(0.0);
					float cos3 = cos(0.0);

					Vector point1;
					point1 = Vector(sin1 * cos3, cos1, sin1 * sin3) * (20 - (GetTickCount64() - elem.dwTime) / 40);

					Vector point3 = elem.vOrigin;
					Vector Out;

					point3 += point1;


					if (!g_DebugOverlay->ScreenPosition(point3, Out))
					{
						points[j++] = Vector2D(Out.x, Out.y);
						//CRender::Get().DrawLine(prev.x + i, prev.y + i, Out.x + i, Out.y + i, colint);
						prev = Out;
					}
					else
					{
						points.clear();
						j = 0;
					}
				}
				if(j)
					Render::Get().draw_list_act->AddMultipointLine(points, j, true, elem.cl_sound_color, 2);
			}
		}
	render_mutex.lock();
	*draw_list = *draw_list_act;
	spectators = spectators_temp;
	this->specs = specs;
	render_mutex.unlock();
}

PizdaDrawList* Render::RenderScene() {

	if (render_mutex.try_lock()) {
		*draw_list_rendering = *draw_list;
		render_mutex.unlock();
	}

	return draw_list_rendering;
}