#include "chams.hpp"
#include <fstream>

#include "../valve_sdk/csgostructs.hpp"
#include "../options.hpp"
#include "../hooks.hpp"
#include "../helpers/input.hpp"

Chams::Chams()
{
    std::ofstream("csgo\\materials\\simple_regular.vmt") << R"#("VertexLitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "0"
  "$envmap"       ""
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";
    std::ofstream("csgo\\materials\\simple_ignorez.vmt") << R"#("VertexLitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       ""
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";
    std::ofstream("csgo\\materials\\simple_flat.vmt") << R"#("UnlitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "0"
  "$envmap"       ""
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";
    std::ofstream("csgo\\materials\\simple_flat_ignorez.vmt") << R"#("UnlitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       ""
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";
	std::ofstream("csgo\\materials\\metallic_ignorez.vmt") << R"#("VertexLitGeneric" 
{ 
"$basetexture" "vgui/white_additive" 
"$ignorez" "1" 
"$envmap" "editor/cube_vertigo" 
"$normalmapalphaenvmapmask" "1" 
"$envmapcontrast" "1" 
"$reflectivity" "[1.0 1.0 1.0]" 
"$nofog" "1" 
"$model" "1" 
"$nocull" "0" 
"$selfillum" "1" 
"$halflambert" "1" 
"$znearer" "0" 
"$flat" "1" 
} 
)#";
	std::ofstream("csgo\\materials\\metallic_regular.vmt") << R"#("VertexLitGeneric" 
{ 
"$basetexture" "vgui/white_additive" 
"$ignorez" "0" 
"$envmap" "editor/cube_vertigo" 
"$normalmapalphaenvmapmask" "1" 
"$envmapcontrast" "1" 
"$reflectivity" "[1.0 1.0 1.0]" 
"$nofog" "1" 
"$model" "1" 
"$nocull" "0" 
"$selfillum" "1" 
"$halflambert" "1" 
"$znearer" "0" 
"$flat" "1" 
} 
)#";

    materialRegular = g_MatSystem->FindMaterial("simple_regular", TEXTURE_GROUP_MODEL);
    materialRegularIgnoreZ = g_MatSystem->FindMaterial("simple_ignorez", TEXTURE_GROUP_MODEL);
    materialFlatIgnoreZ = g_MatSystem->FindMaterial("simple_flat_ignorez", TEXTURE_GROUP_MODEL);
    materialFlat = g_MatSystem->FindMaterial("simple_flat", TEXTURE_GROUP_MODEL);
	materialMetalicIgnorez = g_MatSystem->FindMaterial("metallic_ignorez.vmt", TEXTURE_GROUP_MODEL);
	materialMetalic = g_MatSystem->FindMaterial("metallic_regular.vmt", TEXTURE_GROUP_MODEL);
}

Chams::~Chams()
{
    std::remove("csgo\\materials\\simple_regular.vmt");
    std::remove("csgo\\materials\\simple_ignorez.vmt");
    std::remove("csgo\\materials\\simple_flat.vmt");
    std::remove("csgo\\materials\\simple_flat_ignorez.vmt");
	std::remove("csgo\\materials\\metallic_regular.vmt");
	std::remove("csgo\\materials\\metallic_ignorez.vmt");
}

void Chams::OverrideMaterial(bool ignoreZ, bool flat, bool wireframe, bool metallic,bool glass, const Color& rgba)
{
    IMaterial* material = nullptr;

    if(flat) {
        if(ignoreZ)
            material = materialFlatIgnoreZ;
        else
            material = materialFlat;
	}
	else if (metallic) {
		if (ignoreZ)
			material = materialMetalicIgnorez;
		else
			material = materialMetalic;
	}
	else{
        if(ignoreZ)
            material = materialRegularIgnoreZ;
        else
            material = materialRegular;
    }


    if(glass) {
        material = materialFlat;
        material->AlphaModulate(0.45f);
    } else {
        material->AlphaModulate(
            rgba.a() / 255.0f);
    }

    material->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, wireframe);
    material->ColorModulate(
        rgba.r() / 255.0f,
        rgba.g() / 255.0f,
        rgba.b() / 255.0f);

    g_MdlRender->ForcedMaterialOverride(material);
}

void Chams::OnSceneEnd() {
	for (int i = 1; i < g_GlobalVars->maxClients; ++i) {
		auto ent = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(i));

		if (g_Options.chams_player_type == 2 && ent && ent->IsPlayer() && (ent->m_iHealth() > 0)) {

			C_BasePlayer* pLocalEntity = static_cast<C_BasePlayer*>(g_EntityList->GetClientEntity(g_EngineClient->GetLocalPlayer()));
			bool isEnemy = pLocalEntity->m_iTeamNum() != ent->m_iTeamNum();
			auto colorInVis = isEnemy ? Utils::Float3ToClr(g_Options.color_chams_player_enemy_occluded) : Utils::Float3ToClr( g_Options.color_chams_player_ally_occluded);
			auto colorVis = isEnemy ? Utils::Float3ToClr(g_Options.color_chams_player_enemy_visible) : Utils::Float3ToClr( g_Options.color_chams_player_ally_visible);
			if(!isEnemy && g_Options.chams_player_enemies_only)
				continue;
			if (!g_Options.chams_visible_only) {
				OverrideMaterial(true, g_Options.chams_player_style == 0, false, g_Options.chams_player_style == 2, false, colorInVis);
				ent->DrawModel(0x1, 255);
			}
			if (!Utils::LineGoesThroughSmoke(pLocalEntity->GetEyePos(), ent->GetBonePos(8))) {
				OverrideMaterial(false, g_Options.chams_player_style == 0, false, g_Options.chams_player_style == 2, false, colorVis);
				ent->DrawModel(0x1, 255);
			}

			g_MdlRender->ForcedMaterialOverride(nullptr);
		}
	}
}

void Chams::OnDrawModelExecute(
    IMatRenderContext* ctx,
    const DrawModelState_t &state,
    const ModelRenderInfo_t &info,
    matrix3x4_t *matrix)
{
    static auto fnDME = Hooks::mdlrender_hook.GetOriginalFunc<Hooks::DrawModelExecute>(index::DrawModelExecute);

    const auto mdl = info.pModel;

    bool is_arm = strstr(mdl->szName, "arms")             != nullptr;
    bool is_player = strstr(mdl->szName, "models/player") != nullptr;
    bool is_sleeve = strstr(mdl->szName, "sleeve")        != nullptr;
    //bool is_weapon = strstr(mdl->szName, "weapons/v_")  != nullptr;

	if (is_player && g_Options.chams_player_type == 1) {
		// 
		// Draw player Chams.
		// 
		auto ent = C_BasePlayer::GetPlayerByIndex(info.entity_index);

		if (ent && g_LocalPlayer && ent->IsAlive()) {
			const auto enemy = ent->m_iTeamNum() != g_LocalPlayer->m_iTeamNum();
			if (!enemy && g_Options.chams_player_enemies_only)
				return;
			auto colorInVis = enemy ? Utils::Float3ToClr(g_Options.color_chams_player_enemy_occluded) : Utils::Float3ToClr(g_Options.color_chams_player_ally_occluded);
			auto colorVis = enemy ? Utils::Float3ToClr(g_Options.color_chams_player_enemy_visible) : Utils::Float3ToClr(g_Options.color_chams_player_ally_visible);
			if (!g_Options.chams_visible_only) {
				OverrideMaterial(true, g_Options.chams_player_style == 0, false, g_Options.chams_player_style == 2, false, colorInVis);
				fnDME(g_MdlRender, ctx, state, info, matrix);
				OverrideMaterial(false, g_Options.chams_player_style == 0, false, g_Options.chams_player_style == 2, false, colorVis);
			}
			else {
				OverrideMaterial(false, g_Options.chams_player_style == 0, false, g_Options.chams_player_style == 2, false, colorVis);
			}
		}
	}
	else if(is_sleeve && g_Options.chams_arms_enabled) {
        auto material = g_MatSystem->FindMaterial(mdl->szName, TEXTURE_GROUP_MODEL);
        if(!material)
            return;
        // 
        // Remove sleeves when drawing Chams.
        // 
        material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
        g_MdlRender->ForcedMaterialOverride(material);
    } 
	else if(is_arm) {
        auto material = g_MatSystem->FindMaterial(mdl->szName, TEXTURE_GROUP_MODEL);
        if(!material)
            return;
        if(g_Options.misc_no_hands) {
            // 
            // No hands.
            // 
            material->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
            g_MdlRender->ForcedMaterialOverride(material);
        }
		else if(g_Options.chams_arms_enabled) {
            if(g_Options.chams_arms_ignorez) {
                OverrideMaterial(
                    true,
                    g_Options.chams_arms_flat,
                    g_Options.chams_arms_wireframe,
                    false,
					false,
                   Color(g_Options.color_chams_arms_occluded[0] * 255, g_Options.color_chams_arms_occluded[1] * 255, g_Options.color_chams_arms_occluded[2] * 255));
                fnDME(g_MdlRender, ctx, state, info, matrix);
                OverrideMaterial(
                    false,
                    g_Options.chams_arms_flat,
                    g_Options.chams_arms_wireframe,
                    false,
					false,
                    Color(g_Options.color_chams_arms_visible[0] * 255, g_Options.color_chams_arms_visible[1] * 255, g_Options.color_chams_arms_visible[2] * 255));
            } else {
                OverrideMaterial(
                    false,
                    g_Options.chams_arms_flat,
                    g_Options.chams_arms_wireframe,
                    g_Options.chams_arms_glass,
					false,
                   Color(g_Options.color_chams_arms_visible[0] * 255, g_Options.color_chams_arms_visible[1] * 255, g_Options.color_chams_arms_visible[2] * 255));
            }
        }
    }
}