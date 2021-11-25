#include "Config.h"
#include "../options.hpp"
#include "../helpers/math.hpp"
#include "../helpers/utils.hpp"
#include "../menu.hpp"
#include "../helpers/input.hpp"
#include "features\cloud.hpp"

char* GetConfigName(int weaponid)
{
	switch (weaponid)
	{
	case WEAPON_DEAGLE:
		return "deagle";
	case WEAPON_ELITE:
		return "elite";
	case WEAPON_FIVESEVEN:
		return "fiveseven";
	case WEAPON_GLOCK:
		return "glock18";
	case WEAPON_AK47:
		return "ak47";
	case WEAPON_AUG:
		return "aug";
	case WEAPON_AWP:
		return "awp";
	case WEAPON_FAMAS:
		return "famas";
	case WEAPON_G3SG1:
		return "g3sg1";
	case WEAPON_GALILAR:
		return "galil";
	case WEAPON_M249:
		return "m249";
	case WEAPON_M4A1:
		return "m4a4";
	case WEAPON_MAC10:
		return "mac10";
	case WEAPON_P90:
		return "p90";
	case WEAPON_UMP45:
		return "ump45";
	case WEAPON_XM1014:
		return "xm1014";
	case WEAPON_BIZON:
		return "bizon";
	case WEAPON_MAG7:
		return "mag7";
	case WEAPON_NEGEV:
		return "negev";
	case WEAPON_SAWEDOFF:
		return "sawedoff";
	case WEAPON_TEC9:
		return "tec9";
	case WEAPON_HKP2000:
		return "p2000";
	case WEAPON_MP7:
		return "mp7";
	case WEAPON_MP9:
		return "mp9";
	case WEAPON_NOVA:
		return "nova";
	case WEAPON_P250:
		return "p250";
	case WEAPON_SCAR20:
		return "scar20";
	case WEAPON_SG556:
		return "sg556";
	case WEAPON_SSG08:
		return "ssg08";
	case WEAPON_M4A1_SILENCER:
		return "m4a1s";
	case WEAPON_USP_SILENCER:
		return "usps";
	case WEAPON_CZ75A:
		return "cz75a";
	case WEAPON_REVOLVER:
		return "revolver";
	case WEAPON_BAYONET:
		return "knife_bayonet";
	case WEAPON_KNIFE_FLIP:
		return "knife_flip";
	case WEAPON_KNIFE_GUT:
		return "knife_gut";
	case WEAPON_KNIFE_KARAMBIT:
		return "knife_karambit";
	case WEAPON_KNIFE_M9_BAYONET:
		return "knife_m9";
	case WEAPON_KNIFE_TACTICAL:
		return "knife_huntsman";
	case WEAPON_KNIFE_FALCHION:
		return "knife_falchion";
	case WEAPON_KNIFE_BUTTERFLY:
		return "knife_butterfly";
	case WEAPON_KNIFE_PUSH:
		return "knife_bowie";
	case 519:
		return "knife_ursus";
	case 520:
		return "knife_gypsy_jackknife";
	case 522:
		return "knife_stiletto";
	case 523:
		return "knife_widowmaker";

	default:
		return "";
	}
	return "";
}
#define STRINGIFY_IMPL(s) #s
#define STRINGIFY(s)      STRINGIFY_IMPL(s)
#define SAVE_SKIN(name,param) WritePrivateProfileStringA("Skins", (name+STRINGIFY(param)).c_str(), param, file.c_str());
#define SAVE_VAL(name,param) WritePrivateProfileStringA("Skins", (name+STRINGIFY(param)).c_str(), std::to_string(param).c_str(), file.c_str());
#define LOAD_SKIN(name,param) GetPrivateProfileStringA("Skins", (name+STRINGIFY(param)).c_str(),"", value_l,32, file.c_str());
#define LOAD_VAL(name,param) GetPrivateProfileStringA("Skins", (name+STRINGIFY(param)).c_str(),"", value_l,32, file.c_str()); \
							 param = atof(value_l);
void CConfig::SaveSkins() {
	std::string  file;

	CreateDirectoryA("C:\\Ev0\\", NULL);

	if (g_Options.Conf_ini == 0)
		file = "C:\\Ev0\\Legit.ini";
	if (g_Options.Conf_ini == 1)
		file = "C:\\Ev0\\SameRage.ini";
	if (g_Options.Conf_ini == 2)
		file = "C:\\Ev0\\Rage.ini";
	auto& entries = Cloud::Get().skin_component->config[C_LocalPlayer::xuid_low].get_items();
	WritePrivateProfileStringA("Skins", "size", std::to_string(entries.size()).c_str(), file.c_str());
	int i = 0;
	for (auto& entry : entries) {

		auto name = std::string("entry#" + std::to_string(i++) + "#");
		SAVE_SKIN(name, entry.custom_name);
		SAVE_VAL(name, entry.definition_index);
		SAVE_VAL(name, entry.definition_override_index);
		SAVE_VAL(name, entry.enabled);
		SAVE_VAL(name, entry.entity_quality_index);
		SAVE_SKIN(name, entry.name);
		SAVE_VAL(name, entry.paint_kit_index);
		SAVE_VAL(name, entry.seed);
		SAVE_VAL(name, entry.stat_trak);
		SAVE_VAL(name, entry.wear);
		SAVE_VAL(name, entry.paint_kit_vector_index);
		SAVE_VAL(name, entry.definition_override_vector_index);
		SAVE_VAL(name, entry.definition_vector_index);
		SAVE_VAL(name, entry.entity_quality_vector_index);
		int j = 0;
		for (auto& st : entry.stickers) {
			auto names = std::string(name + "#sticker#" + std::to_string(j++));
			SAVE_VAL(names, st.kit);
			SAVE_VAL(names, st.rotation);
			SAVE_VAL(names, st.wear);
			SAVE_VAL(names, st.scale);
			SAVE_VAL(names, st.kit_vector_index);
		}
	}
}

void CConfig::LoadSkins() {
	static TCHAR path[MAX_PATH];
	std::string folder, file;



	CreateDirectoryA("C:\\Ev0\\", NULL);

	if (g_Options.Conf_ini == 0)
		file = "C:\\Ev0\\Legit.ini";
	if (g_Options.Conf_ini == 1)
		file = "C:\\Ev0\\SameRage.ini";
	if (g_Options.Conf_ini == 2)
		file = "C:\\Ev0\\Rage.ini";

	char value_l[32] = { '\0' };
	//GetPrivateProfileStringA(value->category.c_str(), value->name.c_str(), "", value_l, 32, file.c_str());

	auto& entries = Cloud::Get().skin_component->config[C_LocalPlayer::xuid_low].get_items();
	GetPrivateProfileStringA("Skins", "size", "", value_l,32, file.c_str());
	entries.resize(atoi(value_l));
	int i = 0;
	for (auto& entry : entries) {

		auto name = std::string("entry#" + std::to_string(i++) + "#");
		LOAD_SKIN(name, entry.custom_name);
		LOAD_VAL(name, entry.definition_index);
		LOAD_VAL(name, entry.definition_override_index);
		LOAD_VAL(name, entry.enabled);
		LOAD_VAL(name, entry.entity_quality_index);
		LOAD_VAL(name, entry.paint_kit_vector_index);
		LOAD_VAL(name, entry.definition_override_vector_index);
		LOAD_VAL(name, entry.definition_vector_index);
		LOAD_VAL(name, entry.entity_quality_vector_index);
		LOAD_SKIN(name, entry.name);
		LOAD_VAL(name, entry.paint_kit_index);
		LOAD_VAL(name, entry.seed);
		LOAD_VAL(name, entry.stat_trak);
		LOAD_VAL(name, entry.wear);
		int j = 0;
		for (auto& st : entry.stickers) {
			auto names = std::string(name + "#sticker#" + std::to_string(j++));
			LOAD_VAL(names, st.kit);
			LOAD_VAL(names, st.rotation);
			LOAD_VAL(names, st.wear);
			LOAD_VAL(names, st.scale);
			LOAD_VAL(names, st.kit_vector_index);
		}
		entry.update<sync_type::VALUE_TO_KEY>();
	}
}

void CConfig::Setup()
{
//OPTION(bool, radar, false);//
//OPTION(bool, rad_enemy_only, false);//
//OPTION(float, rad_alpha, 255.f);//
//OPTION(float, rad_zoom, 0.f);//
	SetupValue(g_Options.radar, false, ("Visuals"), ("radar"));
	SetupValue(g_Options.rad_enemy_only, false, ("Visuals"), ("rad_enemy_only"));
	SetupValue(g_Options.rad_alpha, 0.f, ("Visuals"), ("rad_alpha"));
	SetupValue(g_Options.rad_zoom, 0.f, ("Visuals"), ("rad_zoom"));
	SetupValue(g_Options.misc_autostrafe, false, ("Visuals"), ("misc_autostrafe"));
	SetupValue(g_Options.misc_noflash, false, ("Visuals"), ("misc_noflash"));
	SetupValue(g_Options.misc_noflash_val, 0.f, ("Visuals"), ("misc_noflash_val"));
	SetupValue(g_Options.misc_speclist, false, ("Visuals"), ("misc_speclist"));
	SetupValue(g_Options.chams_visible_only, false, ("Visuals"), ("chams_visible_only"));
	SetupValue(g_Options.misc_speclist_you, false, ("Visuals"), ("misc_speclist_you"));
	SetupValue(g_Options.autoaccept, false, ("Visuals"), ("autoaccept"));
	SetupValue(g_Options.medals, false, ("Visuals"), ("medals"));
	SetupValue(g_Options.fov_changer, false, ("Visuals"), ("fov_changer"));
	SetupValue(g_Options.esp_fov, false, ("Visuals"), ("esp_fov"));
	SetupValue(g_Options.ov_fov, 90.f, ("Visuals"), ("ov_fov"));
	SetupValue(g_Options.misc_nade_pred, false, ("Visuals"), ("misc_nade_pred"));
	SetupValue(g_Options.misc_full_kb, false, ("Visuals"), ("misc_full_kb"));
	SetupValue(g_Options.misc_auto_kb, false, ("Visuals"), ("misc_auto_kb"));
	SetupValue(g_Options.misc_kb_enabled, false, ("Visuals"), ("misc_kb_enabled"));
	SetupValue(g_Options.chams_visible_only, false, ("Visuals"), ("chams_visible_only"));
	SetupValue(g_Options.esp_show_ammo, false, ("Visuals"), ("esp_show_ammo"));
	SetupValue(g_Options.esp_sound, false, ("Visuals"), ("esp_sound"));
	SetupValue(g_Options.esp_sniper_crosshair, false, ("Visuals"), ("esp_sniper_crosshair"));
	SetupValue(g_Options.esp_rcs, false, ("Visuals"), ("esp_rcs"));
	SetupValue(g_Options.esp_radar, false, ("Visuals"), ("esp_radar"));
	SetupValue(g_Options.glow_visible_only, false, ("Visuals"), ("glow_visible_only"));
	SetupValue(g_Options.esp_enabled, false, ( "Visuals" ), ( "esp_enabled" ) );
	SetupValue(g_Options.Style, 0, ( "Visuals" ), ( "Style" ) );
	SetupValue(g_Options.esp_enemies_only, false, ("Visuals"), ("esp_enemies_only"));
	SetupValue(g_Options.esp_visible_only, false, ("Visuals"), ("esp_visible_only"));
	SetupValue(g_Options.esp_player_boxes, false, ("Visuals"), ("esp_player_boxes"));
	SetupValue(g_Options.esp_skeleton, false, ("Visuals"), ("esp_skeleton"));
	SetupValue(g_Options.esp_show_flashed, false, ("Visuals"), ("esp_show_flashed"));
	SetupValue(g_Options.esp_player_names, false, ("Visuals"), ("esp_player_names"));
	SetupValue(g_Options.esp_player_health, false, ("Visuals"), ("esp_player_health"));
	SetupValue(g_Options.esp_player_armour, false, ("Visuals"), ("esp_player_armour"));
	SetupValue(g_Options.esp_weapon_type, 0, ("Visuals"), ("esp_player_weapons"));
	SetupValue(g_Options.bar_type, 0, ("Visuals"), ("esp_player_bars"));
	SetupValue(g_Options.esp_player_snaplines, false, ("Visuals"), ("esp_player_snaplines"));
	SetupValue(g_Options.esp_crosshair, false, ("Visuals"), ("esp_crosshair"));
	SetupValue(g_Options.esp_dropped_weapons, false, ("Visuals"), ("esp_dropped_weapons"));
	SetupValue(g_Options.esp_defuse_kit, false, ("Visuals"), ("esp_defuse_kit"));
	SetupValue(g_Options.esp_planted_c4, false, ("Visuals"), ("esp_planted_c4"));
	SetupValue(g_Options.glow_enabled, false, ("Visuals"), ("glow_enabled"));
	SetupValue(g_Options.glow_enemies_only, false, ("Visuals"), ("glow_enemies_only"));
	SetupValue(g_Options.glow_players, false, ("Visuals"), ("glow_players"));
	SetupValue(g_Options.glow_chickens, false, ("Visuals"), ("glow_chickens"));
	SetupValue(g_Options.glow_c4_carrier, false, ("Visuals"), ("glow_c4_carrier"));
	SetupValue(g_Options.glow_planted_c4, false, ("Visuals"), ("glow_planted_c4"));
	SetupValue(g_Options.glow_defuse_kits, false, ("Visuals"), ("glow_defuse_kits"));
	SetupValue(g_Options.glow_weapons, false, ("Visuals"), ("glow_weapons"));
	SetupValue(g_Options.chams_player_type, 0, ("VisualsPlayers"), ("chams_player_enabled"));
	SetupValue(g_Options.chams_player_enemies_only, false, ("VisualsPlayers"), ("chams_player_enemies_only"));
	SetupValue(g_Options.chams_player_style, 0, ("VisualsPlayers"), ("chams_player_style"));
	SetupValue(g_Options.chams_arms_wireframe, false, ("VisualsArms"), ("chams_arms_wireframe"));
	SetupValue(g_Options.chams_arms_enabled, false, ("VisualsArms"), ("chams_arms_enabled"));
	SetupValue(g_Options.chams_arms_flat, false, ("VisualsArms"), ("chams_arms_flat"));
	SetupValue(g_Options.chams_arms_ignorez, false, ("VisualsArms"), ("chams_arms_ignorez"));
	SetupValue(g_Options.chams_arms_glass, false, ("VisualsArms"), ("chams_arms_glass"));

	SetupValue(g_Options.misc_bhop, false, ("MISC"), ("misc_bhop"));
	SetupValue(g_Options.misc_thirdperson, false, ("MISC"), ("misc_thirdperson"));
	SetupValue(g_Options.misc_thirdperson_dist, 0, ("MISC"), ("misc_thirdperson_dist"));
	SetupValue(g_Options.misc_no_hands, false, ("MISC"), ("misc_no_hands"));
	SetupValue(g_Options.misc_showranks, false, ("MISC"), ("misc_showranks"));
	SetupValue(g_Options.misc_watermark, false, ("MISC"), ("misc_watermark"));
	SetupValue(g_Options.viewmodel_fov, 0, ("MISC"), ("viewmodel_fov"));
	SetupValue(g_Options.mat_ambient_light_r, 0, ("MISC"), ("mat_ambient_light_r"));
	SetupValue(g_Options.mat_ambient_light_g, 0, ("MISC"), ("mat_ambient_light_g"));
	SetupValue(g_Options.mat_ambient_light_b, 0, ("MISC"), ("mat_ambient_light_b"));


	SetupValue(Legitbot.Enabled, false, ("Legitbot"), ("Enabled"));
	SetupValue(Legitbot.key, 0, ("Legitbot"), ("key"));
	SetupValue(Legitbot.Deathmatch, false, ("Legitbot"), ("Deathmatch"));
	SetupValue(Legitbot.Backtrack, false, ("Legitbot"), ("Backtrack"));
	SetupValue(Legitbot.SmokeCheck, false, ("Legitbot"), ("SmokeCheck"));
	SetupValue(Legitbot.JumpCheck, false, ("Legitbot"), ("JumpCheck"));
	SetupValue(Legitbot.FlashCheck, false, ("Legitbot"), ("FlashCheck"));
	SetupValue(Legitbot.AutoPistol, false, ("Legitbot"), ("AutoPistol"));
	SetupValue(Legitbot.AimType, 0, ("Legitbot"), ("AimType"));
	SetupValue(Legitbot.KillDelay, false, ("Legitbot"), ("KillDelay"));
	SetupValue(Legitbot.KillDelayTime, 0, ("Legitbot"), ("KillDelayTime"));
	SetupValue(Legitbot.LegitAA, false, ("Legitbot"), ("LegitAA"));
	SetupValue(Legitbot.LegitAA_Offset, 0, ("Legitbot"), ("LegitAA_Offset"));
	SetupValue(Legitbot.FastZoom[1], false, ("Legitbot"), ("FastZoom1"));
	SetupValue(Legitbot.FastZoom[0], false, ("Legitbot"), ("FastZoom0"));

	SetupValue(Triggerbot.Enabled, false, ("Triggerbot"), ("FastZoom0"));
	SetupValue(Triggerbot.Key, false, ("Triggerbot"), ("Key"));
	SetupValue(Triggerbot.Deathmatch, false, ("Triggerbot"), ("Deathmatch"));
	SetupValue(Triggerbot.SmokeCheck, false, ("Triggerbot"), ("SmokeCheck"));
	SetupValue(Triggerbot.Delay, false, ("Triggerbot"), ("Delay"));

	SetupValue(g_Options.Cloud_radar, false, ("Cloud"), ("Radar"));
	SetupValue(g_Options.Cloud_skins, false, ("Cloud"), ("Skins"));


	SetupValue(g_Options.rank, 0, ("Cloud"), ("rank"));
	SetupValue(g_Options.wins, 0, ("Cloud"), ("wins"));
	SetupValue(g_Options.friendly, 0, ("Cloud"), ("friendly"));
	SetupValue(g_Options.funny, 0, ("Cloud"), ("funny"));
	SetupValue(g_Options.mentor, 0, ("Cloud"), ("mentor"));
	SetupValue(g_Options.private_rank, 0, ("Cloud"), ("private_rank"));

	SetupValue(g_Options.color_esp_ally_visible[0], 0, ("g_Options"), ("color_esp_ally_visible_r"));
	SetupValue(g_Options.color_esp_ally_visible[1], 0, ("g_Options"), ("color_esp_ally_visible_g"));
	SetupValue(g_Options.color_esp_ally_visible[2], 0, ("g_Options"), ("color_esp_ally_visible_b"));

	SetupValue(g_Options.color_esp_enemy_visible[0], 0, ("g_Options"), ("color_esp_enemy_visible_r"));
	SetupValue(g_Options.color_esp_enemy_visible[1], 0, ("g_Options"), ("color_esp_enemy_visible_g"));
	SetupValue(g_Options.color_esp_enemy_visible[2], 0, ("g_Options"), ("color_esp_enemy_visible_b"));

	SetupValue(g_Options.color_esp_ally_occluded[0], 0, ("g_Options"), ("color_esp_ally_occluded_r"));
	SetupValue(g_Options.color_esp_ally_occluded[1], 0, ("g_Options"), ("color_esp_ally_occluded_g"));
	SetupValue(g_Options.color_esp_ally_occluded[2], 0, ("g_Options"), ("color_esp_ally_occluded_b"));

	SetupValue(g_Options.color_esp_enemy_occluded[0], 0, ("g_Options"), ("color_esp_enemy_occluded_r"));
	SetupValue(g_Options.color_esp_enemy_occluded[1], 0, ("g_Options"), ("color_esp_enemy_occluded_g"));
	SetupValue(g_Options.color_esp_enemy_occluded[2], 0, ("g_Options"), ("color_esp_enemy_occluded_b"));

	SetupValue(g_Options.color_esp_weapons[0], 0, ("g_Options"), ("color_esp_weapons_r"));
	SetupValue(g_Options.color_esp_weapons[1], 0, ("g_Options"), ("color_esp_weapons_g"));
	SetupValue(g_Options.color_esp_weapons[2], 0, ("g_Options"), ("color_esp_weapons_b"));

	SetupValue(g_Options.color_esp_defuse[0], 0, ("g_Options"), ("color_esp_defuse_r"));
	SetupValue(g_Options.color_esp_defuse[1], 0, ("g_Options"), ("color_esp_defuse_g"));
	SetupValue(g_Options.color_esp_defuse[2], 0, ("g_Options"), ("color_esp_defuse_b"));

	SetupValue(g_Options.color_esp_c4[0], 0, ("g_Options"), ("color_esp_c4_r"));
	SetupValue(g_Options.color_esp_c4[1], 0, ("g_Options"), ("color_esp_c4_g"));
	SetupValue(g_Options.color_esp_c4[2], 0, ("g_Options"), ("color_esp_c4_b"));

	SetupValue(g_Options.color_glow_ally[0], 0, ("g_Options"), ("color_glow_ally_r"));
	SetupValue(g_Options.color_glow_ally[1], 0, ("g_Options"), ("color_glow_ally_g"));
	SetupValue(g_Options.color_glow_ally[2], 0, ("g_Options"), ("color_glow_ally_b"));

	SetupValue(g_Options.color_glow_enemy[0], 0, ("g_Options"), ("color_glow_enemy_r"));
	SetupValue(g_Options.color_glow_enemy[1], 0, ("g_Options"), ("color_glow_enemy_g"));
	SetupValue(g_Options.color_glow_enemy[2], 0, ("g_Options"), ("color_glow_enemy_b"));

	SetupValue(g_Options.color_glow_chickens[0], 0, ("g_Options"), ("color_glow_chickens_r"));
	SetupValue(g_Options.color_glow_chickens[1], 0, ("g_Options"), ("color_glow_chickens_g"));
	SetupValue(g_Options.color_glow_chickens[2], 0, ("g_Options"), ("color_glow_chickens_b"));

	SetupValue(g_Options.color_glow_c4_carrier[0], 0, ("g_Options"), ("color_glow_c4_carrier_r"));
	SetupValue(g_Options.color_glow_c4_carrier[1], 0, ("g_Options"), ("color_glow_c4_carrier_g"));
	SetupValue(g_Options.color_glow_c4_carrier[2], 0, ("g_Options"), ("color_glow_c4_carrier_b"));

	SetupValue(g_Options.color_glow_planted_c4[0], 0, ("g_Options"), ("color_glow_planted_c4_r"));
	SetupValue(g_Options.color_glow_planted_c4[1], 0, ("g_Options"), ("color_glow_planted_c4_g"));
	SetupValue(g_Options.color_glow_planted_c4[2], 0, ("g_Options"), ("color_glow_planted_c4_b"));

	SetupValue(g_Options.color_glow_defuse[0], 0, ("g_Options"), ("color_glow_defuse_r"));
	SetupValue(g_Options.color_glow_defuse[1], 0, ("g_Options"), ("color_glow_defuse_g"));
	SetupValue(g_Options.color_glow_defuse[2], 0, ("g_Options"), ("color_glow_defuse_b"));

	SetupValue(g_Options.color_glow_weapons[0], 0, ("g_Options"), ("color_glow_weapons_r"));
	SetupValue(g_Options.color_glow_weapons[1], 0, ("g_Options"), ("color_glow_weapons_g"));
	SetupValue(g_Options.color_glow_weapons[2], 0, ("g_Options"), ("color_glow_weapons_b"));
#define SAVE_COL(f3,name)	SetupValue(f3[0], 0, ("g_Options"), std::string(name)+"_r"); \
                            SetupValue(f3[1], 0, ("g_Options"), std::string(name)+"_g"); \
                            SetupValue(f3[2], 0, ("g_Options"), std::string(name)+"_b");
    SAVE_COL(g_Options.color_nade_pred,"color_nade_pred")
	SAVE_COL(g_Options.color_esp_sound, "color_esp_sound")
	SAVE_COL(g_Options.color_esp_crosshair_rcs, "color_esp_crosshair_rcs")
	SAVE_COL(g_Options.color_esp_crosshair,"color_crosshair")
	SAVE_COL(g_Options.color_esp_fov, "color_esp_fov")
	SAVE_COL(g_Options.color_glow_ally_invis, "color_glow_ally_invis")
	SAVE_COL(g_Options.color_glow_enemy_invis, "color_glow_enemy_invis")
	SAVE_COL(g_Options.radar_vis_en, "radar_vis_en")
	SAVE_COL(g_Options.radar_vis_al, "radar_vis_al")
	SAVE_COL(g_Options.color_watermark,"watermark_color")
	SetupValue(g_Options.color_chams_player_ally_visible[0], 0, ("g_Options"), ("color_chams_player_ally_visible_r"));
	SetupValue(g_Options.color_chams_player_ally_visible[1], 0, ("g_Options"), ("color_chams_player_ally_visible_g"));
	SetupValue(g_Options.color_chams_player_ally_visible[2], 0, ("g_Options"), ("color_chams_player_ally_visible_b"));

	SetupValue(g_Options.color_chams_player_ally_occluded[0], 0, ("g_Options"), ("color_chams_player_ally_occluded_r"));
	SetupValue(g_Options.color_chams_player_ally_occluded[1], 0, ("g_Options"), ("color_chams_player_ally_occluded_g"));
	SetupValue(g_Options.color_chams_player_ally_occluded[2], 0, ("g_Options"), ("color_chams_player_ally_occluded_b"));

	SetupValue(g_Options.color_chams_player_enemy_visible[0], 0, ("g_Options"), ("color_chams_player_enemy_visible_r"));
	SetupValue(g_Options.color_chams_player_enemy_visible[1], 0, ("g_Options"), ("color_chams_player_enemy_visible_g"));
	SetupValue(g_Options.color_chams_player_enemy_visible[2], 0, ("g_Options"), ("color_chams_player_enemy_visible_b"));

	SetupValue(g_Options.color_chams_player_enemy_occluded[0], 0, ("g_Options"), ("color_chams_player_enemy_occluded_r"));
	SetupValue(g_Options.color_chams_player_enemy_occluded[1], 0, ("g_Options"), ("color_chams_player_enemy_occluded_g"));
	SetupValue(g_Options.color_chams_player_enemy_occluded[2], 0, ("g_Options"), ("color_chams_player_enemy_occluded_b"));

	SetupValue(g_Options.color_chams_arms_visible[0], 0, ("g_Options"), ("color_chams_arms_visible_r"));
	SetupValue(g_Options.color_chams_arms_visible[1], 0, ("g_Options"), ("color_chams_arms_visible_g"));
	SetupValue(g_Options.color_chams_arms_visible[2], 0, ("g_Options"), ("color_chams_arms_visible_b"));

	SetupValue(g_Options.color_chams_arms_occluded[0], 0, ("g_Options"), ("color_chams_arms_occluded_r"));
	SetupValue(g_Options.color_chams_arms_occluded[1], 0, ("g_Options"), ("color_chams_arms_occluded_g"));
	SetupValue(g_Options.color_chams_arms_occluded[2], 0, ("g_Options"), ("color_chams_arms_occluded_b"));

	for (int i = 1; i < 100; i++)
	{
		char* section = GetConfigName(i);
		if (strcmp(section, "") == 0)
			continue;

		SetupValue(weapons[i].Enabled, false, (section), ("Enabled"));
		SetupValue(weapons[i].Nearest, false, (section), ("Nearest"));
		SetupValue(weapons[i].Bone, 0, (section), ("Bone"));
		SetupValue(weapons[i].Fov, 0, (section), ("Fov"));
		SetupValue(weapons[i].Smooth, 0, (section), ("Smooth"));
		SetupValue(weapons[i].RcsX, 0, (section), ("RcsX"));
		SetupValue(weapons[i].RcsY, 0, (section), ("RcsY"));
		SetupValue(weapons[i].StartBullet, 0, (section), ("StartBullet"));
		SetupValue(weapons[i].EndBullet, 0, (section), ("EndBullet"));
		SetupValue(weapons[i].pSilent, false, (section), ("pSilent"));
		SetupValue(weapons[i].pSilentPercentage, 0, (section), ("pSilentPercentage"));
		SetupValue(weapons[i].pSilentFov, 0, (section), ("pSilentFov"));

		SetupValue(weapons[i].TriggerEnabled, false, (section), ("TriggerEnabled"));
		SetupValue(weapons[i].TriggerHitboxHead, false, (section), ("TriggerHitboxHead"));
		SetupValue(weapons[i].TriggerHitboxChest, false, (section), ("TriggerHitboxChest"));
		SetupValue(weapons[i].TriggerHitboxStomach, false, (section), ("TriggerHitboxStomach"));
		SetupValue(weapons[i].TriggerHitboxArms, false, (section), ("TriggerHitboxArms"));
		SetupValue(weapons[i].TriggerHitboxLegs, false, (section), ("TriggerHitboxLegs"));
		SetupValue(weapons[i].TriggerHitChance, 0, (section), ("TriggerHitChance"));
	}


	
}

void CConfig::SetupValue( int& value, int def, std::string category, std::string name )
{
	value = def;
	ints.push_back( new ConfigValue< int >( category, name, &value ) );
}

void CConfig::SetupValue(char* value, char* def, std::string category, std::string name)
{
	value = def;
	chars.push_back(new ConfigValue< char >(category, name, value));
}



void CConfig::SetupValue( float& value, float def, std::string category, std::string name )
{
	value = def;
	floats.push_back( new ConfigValue< float >( category, name, &value ) );
}

void CConfig::SetupValue( bool& value, bool def, std::string category, std::string name )
{
	value = def;
	bools.push_back( new ConfigValue< bool >( category, name, &value ) );
}

void CConfig::Save()
{
	std::string  file;
	SaveSkins();
	CreateDirectoryA("C:\\Ev0\\", NULL );
	
	if (g_Options.Conf_ini == 0)
	    file = "C:\\Ev0\\Legit.ini";
	if (g_Options.Conf_ini == 1)
		file = "C:\\Ev0\\SameRage.ini";
	if (g_Options.Conf_ini == 2)
		file = "C:\\Ev0\\Rage.ini";


	for( auto value : ints )
	WritePrivateProfileStringA( value->category.c_str(), value->name.c_str(), std::to_string( *value->value ).c_str(), file.c_str() );

	for( auto value : floats )
	WritePrivateProfileStringA( value->category.c_str(), value->name.c_str(), std::to_string( *value->value ).c_str(), file.c_str() );

	for( auto value : bools )
	WritePrivateProfileStringA( value->category.c_str(), value->name.c_str(), *value->value ? "true" : "false", file.c_str() );
}
#include "features/profile.hpp"
#include "helpers/utils.hpp"
void CConfig::Load()
{
	static TCHAR path[MAX_PATH];
	std::string folder, file;

	LoadSkins();
	Profile::Get().Update();
	Utils::ForceFullUpdate();
	CreateDirectoryA("C:\\Ev0\\", NULL);

	if (g_Options.Conf_ini == 0)
		file = "C:\\Ev0\\Legit.ini";
	if (g_Options.Conf_ini == 1)
		file = "C:\\Ev0\\SameRage.ini";
	if (g_Options.Conf_ini == 2)
		file = "C:\\Ev0\\Rage.ini";

	char value_l[32] = { '\0' };

	for( auto value : ints )
	{
		GetPrivateProfileStringA( value->category.c_str(), value->name.c_str(), "", value_l, 32, file.c_str() );
		*value->value = atoi( value_l );
	}

	for( auto value : floats )
	{
		GetPrivateProfileStringA( value->category.c_str(), value->name.c_str(), "", value_l, 32, file.c_str() );
		*value->value = atof( value_l );
	}

	for( auto value : bools )
	{
		GetPrivateProfileStringA( value->category.c_str(), value->name.c_str(), "", value_l, 32, file.c_str() );
		*value->value = !strcmp( value_l, "true" );
	}
}

CConfig* Config = new CConfig();
