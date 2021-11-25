#pragma once

#include <string>
#include "valve_sdk/Misc/Color.hpp"
#include "features\skins.hpp"


#include <vector>






#define OPTION(type, var, val) type var = val

class ConfigS
{
public:
	// 
	// ESP
	// 
	int Style = 0;
	OPTION(bool, esp_enabled, false);
	OPTION(int, bar_type, 0);//
	OPTION(bool, esp_enemies_only, false);
	OPTION(bool, esp_player_boxes, false);
	OPTION(bool, esp_skeleton, false);//
	OPTION(bool, esp_visible_only, false);//
	OPTION(bool, esp_show_flashed, false);//
	OPTION(bool, esp_player_names, false);
	OPTION(bool, esp_player_health, false);
	OPTION(bool, esp_player_armour, false);
	OPTION(bool, esp_show_ammo, false);//
	OPTION(bool, esp_player_snaplines, false);
	OPTION(bool, esp_crosshair, false);
	OPTION(bool, esp_sound, false);//
	OPTION(bool, esp_sniper_crosshair, false);//
	OPTION(bool, esp_rcs, false);//
	OPTION(bool, esp_fov, false);//
	OPTION(bool, esp_dropped_weapons, false);
	OPTION(bool, esp_defuse_kit, false);
	OPTION(bool, esp_planted_c4, false);
	OPTION(bool, esp_radar, false);//
	OPTION(int, esp_weapon_type, 0);//
	// 
	// GLOW
	// 
	OPTION(bool, glow_enabled, false);
	OPTION(bool, glow_enemies_only, false);
	OPTION(bool, glow_visible_only, false);//
	OPTION(bool, glow_players, false);

	OPTION(bool, glow_chickens, false);
	OPTION(bool, glow_c4_carrier, false);
	OPTION(bool, glow_planted_c4, false);
	OPTION(bool, glow_defuse_kits, false);
	OPTION(bool, glow_weapons, false);

	//
	// CHAMS
	//
	OPTION(bool, chams_visible_only, false);//
	OPTION(int, chams_player_type, 0);//
	OPTION(bool, chams_player_enemies_only, false);
	OPTION(int, chams_player_style, false);
	OPTION(bool, chams_player_wireframe, false);
	OPTION(bool, chams_player_flat, false);
	OPTION(bool, chams_player_ignorez, false);
	OPTION(bool, chams_player_metalic, false);
	OPTION(bool, chams_arms_enabled, false);
	OPTION(bool, chams_arms_wireframe, false);
	OPTION(bool, chams_arms_flat, false);
	OPTION(bool, chams_arms_ignorez, false);
	OPTION(bool, chams_arms_glass, false);

	//
	// MISC
	//
	OPTION(bool, misc_kb_enabled, false);//
	OPTION(bool, misc_auto_kb, false);//
	OPTION(bool, misc_full_kb, false);//
	OPTION(bool, misc_bhop, false);
	OPTION(bool, misc_nade_pred, false);//
	OPTION(bool, misc_autostrafe, false);//
	OPTION(bool, misc_noflash, false);//
	OPTION(float, misc_noflash_val, 0);//
	OPTION(bool, misc_speclist, false);//
	OPTION(bool, misc_speclist_you, false);//
	OPTION(bool, misc_no_hands, false);
	OPTION(bool, misc_thirdperson, false);
	OPTION(bool, misc_showranks, true);
	OPTION(bool, misc_watermark, true);
	OPTION(float, misc_thirdperson_dist, 50.f);
	OPTION(int, viewmodel_fov, 68);
	OPTION(float, mat_ambient_light_r, 0.0f);
	OPTION(float, mat_ambient_light_g, 0.0f);
	OPTION(float, mat_ambient_light_b, 0.0f);
	OPTION(bool, autoaccept, false);//
	OPTION(bool, medals, false);//
	OPTION(bool, fov_changer, false);//
	OPTION(float, ov_fov, 90.f);//
	OPTION(bool, radar, false);//
	OPTION(bool, rad_enemy_only, false);//
	OPTION(float, rad_alpha, 255.f);//
	OPTION(float, rad_zoom, 0.f);//
	// 
	// COLORS
	// 
	float color_nade_pred[3] = { 0.1f, 0.1f, 0.1f };//
	float color_esp_ally_visible[3] = { 0.1f, 0.1f, 0.1f};
	float color_esp_enemy_visible[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_ally_occluded[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_enemy_occluded[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_crosshair[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_weapons[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_defuse[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_c4[3] = { 0.1f, 0.1f, 0.1f };
	float color_esp_sound[3] = { 0.1f, 0.1f, 0.1f };//
	float color_esp_crosshair_rcs[3] = { 0.1f, 0.1f, 0.1f };//
	float color_esp_fov[3] = { 0.1f, 0.1f, 0.1f };//

	float color_glow_ally[3] = { 0.1f, 0.1f, 0.1f };
	float color_glow_ally_invis[3] = { 0.1f, 0.1f, 0.1f };//
	float color_glow_enemy[3] = { 0.1f, 0.1f, 0.1f };
	float color_glow_enemy_invis[3] = { 0.1f, 0.1f, 0.1f };//
	float color_glow_chickens[3] = { 0.1f, 0.1f, 0.1f };
	float color_glow_c4_carrier[3] = { 0.1f, 0.1f, 0.1f };
	float color_glow_planted_c4[3] = { 0.1f, 0.1f, 0.1f };
	float color_glow_defuse[3] = { 0.1f, 0.1f, 0.1f };
	float color_glow_weapons[3] = { 0.1f, 0.1f, 0.1f };

	float color_chams_player_ally_visible[3] = { 0.1f, 0.1f, 0.1f };
	float color_chams_player_ally_occluded[3] = { 0.1f, 0.1f, 0.1f };
	float color_chams_player_enemy_visible[3] = { 0.1f, 0.1f, 0.1f };
	float color_chams_player_enemy_occluded[3] = { 0.1f, 0.1f, 0.1f };
	float color_chams_arms_visible[3] = { 0.1f, 0.1f, 0.1f };
	float color_chams_arms_occluded[3] = { 0.1f, 0.1f, 0.1f };
	float color_watermark[3] = { 0.1f, 0.1f, 0.1f };
	float radar_vis_en[3] = { 0.1f, 0.1f, 0.1f };//
	float radar_vis_al[3] = { 0.1f, 0.1f, 0.1f };//
	OPTION(bool, Cloud_skins, false);
	OPTION(bool, Cloud_radar, false);
	OPTION(int, Conf_ini, 0);

	int rank;
	int wins;
	int friendly;
	int funny;
	int mentor;

	int private_rank;
		bool Russian;

};

extern ConfigS g_Options;
extern bool   g_Unload;


class Sticker_t
{
public:
	int iID = 0;
	float flWear = 0.f;
	float flScale = 1.f;
	int iRotation = 0;
};

class Weapon_t
{
public:
	OPTION(bool, Enabled, true);
	OPTION(float, Fov, 6);
	OPTION(int, Bone, 8);
	OPTION(bool, Nearest, false);
	OPTION(int, NearestType, 1);
	OPTION(float, Smooth, 10.f);
	OPTION(int, StartBullet, 0);
	OPTION(int, EndBullet, 0);
	OPTION(int, RcsX, 100);
	OPTION(int, RcsY, 100);
	OPTION(bool, pSilent, true);
	OPTION(int, pSilentPercentage, 70);
	OPTION(int, pSilentBullet, 1);
	OPTION(float, pSilentFov, 1.4);
	OPTION(float, pSilentSmooth, 0);

	int DelayBefore = 0;

	


	bool TriggerEnabled = false;
	bool TriggerHitboxHead = false;
	bool TriggerHitboxChest = false;
	bool TriggerHitboxStomach = false;
	bool TriggerHitboxArms = false;
	bool TriggerHitboxLegs = false;
	int TriggerHitChance = 0;
	int TriggerMinDamage = 0;

	bool ChangerEnabled = false;
	int ChangerSkin = 0;
	char ChangerName[32] = "";
	int ChangerStatTrak = 0;
	int ChangerSeed = 0;
	float ChangerWear = 0;
	static int iSlot;
	bool StickersEnabled = false;
	Sticker_t Stickers[5];
};
extern Weapon_t weapons[524];


class Legitbot_t
{
public:
	bool Enabled = true;
	bool DrawFov = false;
	bool Deathmatch = false;
	int key = false;
	bool SmokeCheck = true;
	bool FlashCheck = true;
	bool JumpCheck = true;
	bool Backtrack = false;
	bool AutoFire = false;
	bool AutoPistol = false;
	bool FastZoom[2] = { false, false };
	int AimType = 0;
	bool KillDelay = true;
	bool LegitAA = false;
	float LegitAA_Offset = 90.f;

	float KillDelayTime = 0.40f;
};
extern Legitbot_t Legitbot;
#include <set>
#include <unordered_map>
struct skinInfo
{
	int seed = -1;
	int paintkit;
	std::string tagName;
};

class Gvars_t
{
public:
	float KillDelayTime = 0;
	bool KillDelayEnd = false;
	float FireDelayTime = 0;
	int WeaponID;
	bool SendPacket;
	int iSlot;
	std::unordered_map<std::string, std::set<std::string>> weaponSkins;
	std::unordered_map<std::string, skinInfo> skinMap;
	std::unordered_map<std::string, std::string> skinNames;
};
extern Gvars_t G;

class Triggerbot_t
{
public:
	bool Enabled = false;
	bool Deathmatch = false;
	bool SmokeCheck = false;
	bool OnKey = false;
	int Key = 0;
	float Delay = 0;
};
extern Triggerbot_t Triggerbot;

class Misc_t
{
public:
	bool Autoaccept = false;
};
extern Misc_t Misc;

struct k_weapon_data {
	int itemDefinitionIndex;
	int paintKit;
	int rarity;
	int seed;
	float wear;
	char* name = "";
};
class Inventory_t
{
public:
	
		bool enabledInv;
		std::vector<k_weapon_data> weapons;
	
	
		bool enabledMed;
		std::vector<uint32_t> medals;
		uint32_t equipped_medal;
	
	
		int   Rang;
		int   Wins;
		int   Frandky;
		int   Leader;
		int   Teach;
		int   Level;
		int   XP;
	
};
extern Inventory_t Inventory;

class SkinChanger_t
{
public:

	bool EnabledSkin = true;
	bool EnabledKnife = true;
	bool EnabledGlove = true;
	int Knife = 4;
	int Knife_t = 4;
	int Glove = 0;
	int GloveSkin = 0;
	int Glove_t = 0; 
	int GloveSkin_t = 0;

};
extern SkinChanger_t SkinChanger;

