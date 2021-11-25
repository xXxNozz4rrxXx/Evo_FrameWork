#include <algorithm>

#include "visuals.hpp"

#include "../options.hpp"
#include "../helpers/math.hpp"
#include "../helpers/utils.hpp"



RECT GetBBox(C_BaseEntity* ent)
{
	RECT rect{};
	auto collideable = ent->GetCollideable();

	if (!collideable)
		return rect;

	auto min = collideable->OBBMins();
	auto max = collideable->OBBMaxs();

	const matrix3x4_t& trans = ent->m_rgflCoordinateFrame();

	Vector points[] = {
		Vector(min.x, min.y, min.z),
		Vector(min.x, max.y, min.z),
		Vector(max.x, max.y, min.z),
		Vector(max.x, min.y, min.z),
		Vector(max.x, max.y, max.z),
		Vector(min.x, max.y, max.z),
		Vector(min.x, min.y, max.z),
		Vector(max.x, min.y, max.z)
	};

	Vector pointsTransformed[8];
	for (int i = 0; i < 8; i++) {
		Math::VectorTransform(points[i], trans, pointsTransformed[i]);
	}

	Vector screen_points[8] = {};

	for (int i = 0; i < 8; i++) {
		if (!Math::WorldToScreen(pointsTransformed[i], screen_points[i]))
			return rect;
	}

	auto left = screen_points[0].x;
	auto top = screen_points[0].y;
	auto right = screen_points[0].x;
	auto bottom = screen_points[0].y;

	for (int i = 1; i < 8; i++) {
		if (left > screen_points[i].x)
			left = screen_points[i].x;
		if (top < screen_points[i].y)
			top = screen_points[i].y;
		if (right < screen_points[i].x)
			right = screen_points[i].x;
		if (bottom > screen_points[i].y)
			bottom = screen_points[i].y;
	}
	return RECT{ (long)left, (long)top, (long)right, (long)bottom };
}

Visuals::Visuals()
{
	InitializeCriticalSection(&cs);
}

Visuals::~Visuals() {
	DeleteCriticalSection(&cs);
}

//--------------------------------------------------------------------------------
void Visuals::Render() {
}
//--------------------------------------------------------------------------------
bool Visuals::Player::Begin(C_BasePlayer* pl)
{
	if (pl->IsDormant() || !pl->IsAlive())
		return false;

	ctx.pl = pl;
	ctx.is_enemy = g_LocalPlayer->m_iTeamNum() != pl->m_iTeamNum();
	ctx.is_visible = pl->IsVisible(true);
	ctx.is_flashed = pl->IsFlashed();
	if (!ctx.is_enemy && g_Options.esp_enemies_only)
		return false;

	if (!ctx.is_visible && g_Options.esp_visible_only)
		return false;
	
	ctx.clr = ctx.is_enemy ? (ctx.is_visible ? Color(g_Options.color_esp_enemy_visible[0] * 255, g_Options.color_esp_enemy_visible[1] * 255, g_Options.color_esp_enemy_visible[2] * 255) : Color(g_Options.color_esp_enemy_occluded[0] * 255, g_Options.color_esp_enemy_occluded[1] * 255, g_Options.color_esp_enemy_occluded[2] * 255)) : (ctx.is_visible ? Utils::Float3ToClr(g_Options.color_esp_ally_visible) : Utils::Float3ToClr(g_Options.color_esp_ally_occluded));

	auto head = pl->GetHitboxPos(HITBOX_HEAD);
	auto origin = pl->m_vecOrigin();

	head.z += 15;

	if (!Math::WorldToScreen(head, ctx.head_pos) ||
		!Math::WorldToScreen(origin, ctx.feet_pos))
		return false;

	auto h = fabs(ctx.head_pos.y - ctx.feet_pos.y);
	auto w = h / 2.f;

	ctx.bbox.left = static_cast<long>(ctx.feet_pos.x - w * 0.5f);
	ctx.bbox.right = static_cast<long>(ctx.bbox.left + w);
	ctx.bbox.bottom = static_cast<long>(ctx.feet_pos.y);
	ctx.bbox.top = static_cast<long>(ctx.head_pos.y);

	return true;
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderBox() {
	if (g_Options.Style == 1)
	{
		Render::Get().draw_list_act->AddRectangle(Vector2D(ctx.bbox.left, ctx.bbox.top), Vector2D(ctx.bbox.right - ctx.bbox.left, ctx.bbox.bottom - ctx.bbox.top), ctx.clr);
		Render::Get().draw_list_act->AddRectangle(Vector2D(ctx.bbox.left - 1, ctx.bbox.top - 1), Vector2D(ctx.bbox.right - ctx.bbox.left + 2, ctx.bbox.bottom - ctx.bbox.top + 2), ctx.clr);
		Render::Get().draw_list_act->AddRectangle(Vector2D(ctx.bbox.left + 1, ctx.bbox.top + 1), Vector2D(ctx.bbox.right - ctx.bbox.left - 2, ctx.bbox.bottom - ctx.bbox.top - 2), ctx.clr);
	}
	else if (g_Options.Style == 2)
	{
		Render::Get().draw_list_act->AddRectangle(Vector2D(ctx.bbox.left, ctx.bbox.top), Vector2D(ctx.bbox.right - ctx.bbox.left, ctx.bbox.bottom - ctx.bbox.top), ctx.clr);
	}
	else if (g_Options.Style == 3)
	{
		Render::Get().draw_list_act->AddOutlineBox(Vector2D(ctx.bbox.left, ctx.bbox.top), Vector2D(ctx.bbox.right - ctx.bbox.left, ctx.bbox.bottom - ctx.bbox.top), ctx.clr);
	}
	else if (g_Options.Style == 4)
	{
		Render::Get().draw_list_act->AddCoalBox(Vector2D(ctx.bbox.left, ctx.bbox.top), Vector2D(ctx.bbox.right - ctx.bbox.left, ctx.bbox.bottom - ctx.bbox.top), ctx.clr);
	}
	else if (g_Options.Style == 5)
	{
		Render::Get().draw_list_act->AddOutlineCoalBox(Vector2D(ctx.bbox.left, ctx.bbox.top), Vector2D(ctx.bbox.right - ctx.bbox.left, ctx.bbox.bottom - ctx.bbox.top), ctx.clr);

	}


}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderName()
{
	if ((g_Options.esp_player_names)) {
		player_info_t info = ctx.pl->GetPlayerInfo();

		auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, info.szName);

		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x - sz.x / 2, ctx.head_pos.y - sz.y * 2), true, false, Color(255, 255, 255), "Calibri", 14, info.szName);
		if(g_Options.esp_show_flashed && ctx.is_flashed)
		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x - sz.x / 2, ctx.head_pos.y - sz.y), false, false, Color(255, 255, 255), "Calibri", 14, "[FLASHED]");
	}
	else {
		auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, "[FLASHED]");
		if (g_Options.esp_show_flashed && ctx.is_flashed)
		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x - sz.x / 2, ctx.head_pos.y - sz.y), false, false, Color(255, 255, 255), "Calibri", 14, "[FLASHED]");
	}
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderHealth()
{
	auto  hp = ctx.pl->m_iHealth();
	float box_h = (float)fabs(ctx.bbox.bottom - ctx.bbox.top);
	//float off = (box_h / 6.f) + 5;
	float off = 8;

	int height = (box_h * hp) / 100;

	int green = int(hp * 2.55f);
	int red = 255 - green;

	int x = ctx.bbox.left - off;
	int y = ctx.bbox.top;
	int w = 4;
	int h = box_h;
	Render::Get().draw_list_act->AddRectangleFilled(Vector2D(x, y),Vector2D( w, h), Color::Black());
	Render::Get().draw_list_act->AddRectangleFilled(Vector2D(x + 1, y + 1), Vector2D(w - 1, height - 2), Color(red, green, 0, 255));
	if (g_Options.bar_type == 1) {
		int sep = h / 10;
		if (sep > 0) {
			for (int cord = y + h - sep; cord >= y; cord -= sep)
			Render::Get().draw_list_act->AddLine(Vector2D(x + 1, cord), Vector2D(x + w - 1, cord), Color::Black());
		}
	}
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderArmour()
{
	auto  armour = ctx.pl->m_ArmorValue();
	float box_h = (float)fabs(ctx.bbox.bottom - ctx.bbox.top);
	//float off = (box_h / 6.f) + 5;
	float off = 4;

	int height = (((box_h * armour) / 100));

	int x = ctx.bbox.right + off;
	int y = ctx.bbox.top;
	int w = 4;
	int h = box_h;
	Render::Get().draw_list_act->AddRectangleFilled(Vector2D(x, y), Vector2D(w,  h), Color::Black());
	Render::Get().draw_list_act->AddRectangleFilled(Vector2D(x + 1, y + 1), Vector2D(w - 1, height - 2), Color(0, 50, 255, 255));
	if (g_Options.bar_type == 1) {
		int sep = h / 10;
		if (sep > 0) {
			for (int cord = y + h - sep; cord >= y; cord -= sep)
			Render::Get().draw_list_act->AddLine(Vector2D(x + 1, cord), Vector2D(x + w - 1, cord), Color::Black());
		}
	}
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderWeaponName()
{
	auto weapon = ctx.pl->m_hActiveWeapon().Get();

	if (!weapon) return;
	auto GetGunIcon = [](short cw)
	{
		switch (cw)
		{
		case WEAPON_KNIFE:
		case WEAPON_KNIFE_T:
		case 500:
		case 505:
		case 506:
		case 507:
		case 508:
		case 509:
		case 512:
		case 514:
		case 515:
		case 516:
		case 519:
		case 520:
		case 522:
		case 523:
			return "]";
		case WEAPON_DEAGLE:
			return "A";
		case WEAPON_ELITE:
			return "B";
		case WEAPON_FIVESEVEN:
			return "C";
		case WEAPON_GLOCK:
			return "D";
		case WEAPON_HKP2000:
			return "E";
		case WEAPON_P250:
			return "F";
		case WEAPON_USP_SILENCER:
			return "G";
		case WEAPON_TEC9:
			return "H";
		case WEAPON_CZ75A:
			return "I";
		case WEAPON_REVOLVER:
			return "J";
		case WEAPON_MAC10:
			return "K";
		case WEAPON_UMP45:
			return "L";
		case WEAPON_BIZON:
			return "M";
		case WEAPON_MP7:
			return "N";
		case WEAPON_MP9:
			return "O";
		case WEAPON_P90:
			return "P";
		case WEAPON_GALILAR:
			return "Q";
		case WEAPON_FAMAS:
			return "R";
		case WEAPON_M4A1_SILENCER:
			return "S";
		case WEAPON_M4A1:
			return "T";
		case WEAPON_AUG:
			return "U";
		case WEAPON_SG556:
			return "V";
		case WEAPON_AK47:
			return "W";
		case WEAPON_G3SG1:
			return "X";
		case WEAPON_SCAR20:
			return "Y";
		case WEAPON_AWP:
			return "Z";
		case WEAPON_SSG08:
			return "a";
		case WEAPON_XM1014:
			return "b";
		case WEAPON_SAWEDOFF:
			return "c";
		case WEAPON_MAG7:
			return "d";
		case WEAPON_NOVA:
			return "e";
		case WEAPON_NEGEV:
			return "f";
		case WEAPON_M249:
			return "g";
		case WEAPON_TASER:
			return "h";
		case WEAPON_FLASHBANG:
			return "i";
		case WEAPON_HEGRENADE:
			return "j";
		case WEAPON_SMOKEGRENADE:
			return "k";
		case WEAPON_MOLOTOV:
			return "l";
		case WEAPON_DECOY:
			return "m";
		case WEAPON_INCGRENADE:
			return "n";
		case WEAPON_C4:
			return "o";
		default:
			return "";
		}
	};
	if (g_Options.esp_weapon_type == 1) {
		std::string text = std::string(weapon->GetCSWeaponData()->szWeaponName + 7);
		auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, text.c_str());
		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x, ctx.feet_pos.y), true, false, Color(255, 255, 255), "Calibri", 14, text.c_str());
		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x + sz.x, ctx.feet_pos.y), false, false, Color(255, 255, 255), "Calibri", 14, std::string(g_Options.esp_show_ammo ? "[" + std::to_string(weapon->m_iClip1()) + "]" : "").c_str());
	}
	else if (g_Options.esp_weapon_type == 2) {
		auto text = std::string(GetGunIcon(weapon->m_Item().m_iItemDefinitionIndex()));
		auto sz = g_pIconFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, text.c_str());
		Render::Get().draw_list_act->AddImage(Vector2D(ctx.feet_pos.x - 10, ctx.feet_pos.y),Vector2D(0.9f, 0.4f), weapon->m_Item().m_iItemDefinitionIndex(), Color(255, 255, 255));
		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x + sz.x, ctx.feet_pos.y), false, false, Color(255, 255, 255), "Calibri", 14, std::string(g_Options.esp_show_ammo ? "[" + std::to_string(weapon->m_iClip1()) + "]" : "").c_str());
	
	}
	else {
		Render::Get().draw_list_act->AddText(Vector2D(ctx.feet_pos.x, ctx.feet_pos.y), false, false, Color(255, 255, 255), "Calibri", 14, std::string(g_Options.esp_show_ammo ? "[" + std::to_string(weapon->m_iClip1()) + "]" : "").c_str());
	}
}
//--------------------------------------------------------------------------------
void Visuals::Player::RenderSnapline()
{

	int screen_w, screen_h;
	g_EngineClient->GetScreenSize(screen_w, screen_h);

	Render::Get().draw_list_act->AddLine(Vector2D(screen_w / 2.f, (float)screen_h), Vector2D(ctx.feet_pos.x, ctx.feet_pos.y), ctx.clr);
}
//--------------------------------------------------------------------------------
void Visuals::RenderCrosshair()
{
	int w, h;

	g_EngineClient->GetScreenSize(w, h);

	int cx = w / 2;
	int cy = h / 2;
	Render::Get().draw_list_act->AddLine(Vector2D(cx - 25, cy), Vector2D(cx + 25, cy), Utils::Float3ToClr(g_Options.color_esp_crosshair));
	Render::Get().draw_list_act->AddLine(Vector2D(cx, cy - 25), Vector2D(cx, cy + 25), Utils::Float3ToClr(g_Options.color_esp_crosshair));
}
//--------------------------------------------------------------------------------
void Visuals::DZItemEsp(C_BaseEntity* ent)
{
	if (!(ent->GetClientClass()->m_ClassID == CPhysPropAmmoBox ||
		ent->GetClientClass()->m_ClassID == CPhysPropLootCrate ||
		ent->GetClientClass()->m_ClassID == CPhysPropRadarJammer ||
		ent->GetClientClass()->m_ClassID == CPhysPropWeaponUpgrade))
		return;

	std::string itemstr = "Undefined";
	const model_t * itemModel = ent->GetModel();
	if (!itemModel)
		return;
	studiohdr_t * hdr = g_MdlInfo->GetStudiomodel(itemModel);
	if (!hdr)
		return;
	itemstr = hdr->szName;

	if (itemstr.find("case_pistol") != std::string::npos)
		itemstr = "Pistol Case";
	else if (itemstr.find("case_light_weapon") != std::string::npos) // Reinforced!
		itemstr = "Light Case";
	else if (itemstr.find("case_heavy_weapon") != std::string::npos)
		itemstr = "Heavy Case";
	else if (itemstr.find("case_explosive") != std::string::npos)
		itemstr = "Explosive Case";
	else if (itemstr.find("case_tools") != std::string::npos)
		itemstr = "Tools Case";
	else if (itemstr.find("random") != std::string::npos)
		itemstr = "Airdrop";
	else if (itemstr.find("dz_armor_helmet") != std::string::npos)
		itemstr = "Full Armor";
	else if (itemstr.find("dz_helmet") != std::string::npos)
		itemstr = "Helmet";
	else if (itemstr.find("dz_armor") != std::string::npos)
		itemstr = "Armor";
	else if (itemstr.find("upgrade_tablet") != std::string::npos)
		itemstr = "Tablet Upgrade";
	else if (itemstr.find("briefcase") != std::string::npos)
		itemstr = "Briefcase";
	else if (itemstr.find("parachutepack") != std::string::npos)
		itemstr = "Parachute";
	else if (itemstr.find("dufflebag") != std::string::npos)
		itemstr = "Cash Dufflebag";
	else if (itemstr.find("ammobox") != std::string::npos)
		itemstr = "Ammobox";
	auto bbox = GetBBox(ent);
	if (bbox.right == 0 || bbox.bottom == 0)
		return;

	Render::Get().draw_list_act->AddRectangle(Vector2D(bbox.left, bbox.top), Vector2D(bbox.left - bbox.right, bbox.top - bbox.bottom), Color(0, 255, 0));
	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, itemstr.c_str());
	int w = bbox.right - bbox.left;


	Render::Get().draw_list_act->AddText(Vector2D((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), false, false, Color(255, 255, 255), "Calibri", 14, itemstr.c_str());
}
//--------------------------------------------------------------------------------


void Visuals::RenderWeapon(C_BaseCombatWeapon* ent)
{
	auto clean_item_name = [](const char* name) -> const char* {
		if (name[0] == 'C')
			name++;

		auto start = strstr(name, "Weapon");
		if (start != nullptr)
			name = start + 6;

		return name;
	};


	// We don't want to Render weapons that are being held
	if (ent->m_hOwnerEntity().IsValid())
		return;

	auto bbox = GetBBox(ent);

	if (bbox.right == 0 || bbox.bottom == 0)
		return;

	Render::Get().draw_list_act->AddRectangle(Vector2D(bbox.left, bbox.top), Vector2D(bbox.left - bbox.right, bbox.top - bbox.bottom), Utils::Float3ToClr(g_Options.color_esp_weapons));

	auto name = clean_item_name(ent->GetClientClass()->m_pNetworkName);

	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, name);
	int w = bbox.right - bbox.left;


	Render::Get().draw_list_act->AddText(Vector2D((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), false, false, Utils::Float3ToClr(g_Options.color_esp_weapons), "Calibri", 14, name);
}
//--------------------------------------------------------------------------------
void Visuals::RenderDefuseKit(C_BaseEntity* ent)
{
	if (ent->m_hOwnerEntity().IsValid())
		return;

	auto bbox = GetBBox(ent);

	if (bbox.right == 0 || bbox.bottom == 0)
		return;

	Render::Get().draw_list_act->AddRectangle(Vector2D(bbox.left, bbox.top), Vector2D(bbox.right - bbox.left, bbox.bottom - bbox.top), Utils::Float3ToClr(g_Options.color_esp_defuse));

	auto name = "Defuse Kit";
	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, name);
	int w = bbox.right - bbox.left;
	Render::Get().draw_list_act->AddText(Vector2D((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), false, false, Utils::Float3ToClr(g_Options.color_esp_defuse), "Calibri", 14, name);
}
//--------------------------------------------------------------------------------
void Visuals::RenderPlantedC4(C_BaseEntity* ent)
{
	auto bbox = GetBBox(ent);

	if (bbox.right == 0 || bbox.bottom == 0)
		return;


	Render::Get().draw_list_act->AddRectangle(Vector2D(bbox.left, bbox.top), Vector2D(bbox.right - bbox.left, bbox.bottom - bbox.top), Utils::Float3ToClr(g_Options.color_esp_c4));

	int bombTimer = std::ceil(ent->m_flC4Blow() - g_GlobalVars->curtime);
	std::string timer = std::to_string(bombTimer);

	auto name = (bombTimer < 0.f) ? "Bomb" : timer;
	auto sz = g_pDefaultFont->CalcTextSizeA(14.f, FLT_MAX, 0.0f, name.c_str());
	int w = bbox.right - bbox.left;

	Render::Get().draw_list_act->AddText(Vector2D((bbox.left + w * 0.5f) - sz.x * 0.5f, bbox.bottom + 1), false, false, Utils::Float3ToClr(g_Options.color_esp_c4), "Calibri", 14, name.c_str());
}
//--------------------------------------------------------------------------------
void Visuals::ThirdPerson() {
	if (!g_LocalPlayer)
		return;

	if (g_Options.misc_thirdperson && g_LocalPlayer->IsAlive())
	{
		if (!g_Input->m_fCameraInThirdPerson)
		{
			g_Input->m_fCameraInThirdPerson = true;
		}

		float dist = g_Options.misc_thirdperson_dist;

		QAngle *view = g_LocalPlayer->GetVAngles();
		trace_t tr;
		Ray_t ray;

		Vector desiredCamOffset = Vector(cos(DEG2RAD(view->yaw)) * dist,
			sin(DEG2RAD(view->yaw)) * dist,
			sin(DEG2RAD(-view->pitch)) * dist
		);

		//cast a ray from the Current camera Origin to the Desired 3rd person Camera origin
		ray.Init(g_LocalPlayer->GetEyePos(), (g_LocalPlayer->GetEyePos() - desiredCamOffset));
		CTraceFilter traceFilter;
		traceFilter.pSkip = g_LocalPlayer;
		g_EngineTrace->TraceRay(ray, MASK_SHOT, &traceFilter, &tr);

		Vector diff = g_LocalPlayer->GetEyePos() - tr.endpos;

		float distance2D = sqrt(abs(diff.x * diff.x) + abs(diff.y * diff.y));// Pythagorean

		bool horOK = distance2D > (dist - 2.0f);
		bool vertOK = (abs(diff.z) - abs(desiredCamOffset.z) < 3.0f);

		float cameraDistance;

		if (horOK && vertOK)  // If we are clear of obstacles
		{
			cameraDistance = dist; // go ahead and set the distance to the setting
		}
		else
		{
			if (vertOK) // if the Vertical Axis is OK
			{
				cameraDistance = distance2D * 0.95f;
			}
			else// otherwise we need to move closer to not go into the floor/ceiling
			{
				cameraDistance = abs(diff.z) * 0.95f;
			}
		}
		g_Input->m_fCameraInThirdPerson = true;

		g_Input->m_vecCameraOffset.z = cameraDistance;
	}
	else
	{
		g_Input->m_fCameraInThirdPerson = false;
	}
}


void Visuals::AddToDrawList() {
	for (auto i = 1; i <= g_EntityList->GetHighestEntityIndex(); ++i) {
		auto entity = C_BaseEntity::GetEntityByIndex(i);

		if (!entity)
			continue;
		

		if (i < 65) {
			auto player = Player();
			if (player.Begin((C_BasePlayer*)entity)) {
				if (g_Options.esp_skeleton && entity!=g_LocalPlayer) player.RenderSkeleton();
				if (g_Options.esp_player_snaplines) player.RenderSnapline();
				if (g_Options.esp_player_boxes)     player.RenderBox();
				if (g_Options.esp_weapon_type > 0 || g_Options.esp_show_ammo)   player.RenderWeaponName();
				if (g_Options.esp_player_names || g_Options.esp_show_flashed)     player.RenderName();
				if (g_Options.esp_player_health)    player.RenderHealth();
				if (g_Options.esp_player_armour)    player.RenderArmour();
			}
		}
		else if (g_Options.esp_dropped_weapons && entity->IsWeapon())
			RenderWeapon(static_cast<C_BaseCombatWeapon*>(entity));
		else if (g_Options.esp_dropped_weapons/* && Utils::IsDangerZone*/)
			DZItemEsp(entity);
		else if (g_Options.esp_dropped_weapons && entity->IsDefuseKit())
			RenderDefuseKit(entity);
		else if (entity->IsPlantedC4() && g_Options.esp_planted_c4)
			RenderPlantedC4(entity);
	}


	if (g_Options.esp_crosshair || (g_Options.esp_sniper_crosshair && (G.WeaponID == WEAPON_SSG08 || G.WeaponID == WEAPON_AWP || G.WeaponID == WEAPON_G3SG1 || G.WeaponID == WEAPON_SCAR20)))
		RenderCrosshair();
	if (g_Options.esp_fov)
		RenderFov();
	if (g_Options.esp_rcs)
		RenderRcs();

}

void Visuals::Player::RenderSkeleton() {
	Vector			vHitboxSkeletonArray[18][2];
	static matrix3x4_t boneMatrix[MAXSTUDIOBONES];
	
	auto GetHitBoxSkeleton = [](int nHitBoxOne, int nHitBoxTwo, C_BasePlayer* pEntity, Vector* vOut, matrix3x4_t boneMatrix[MAXSTUDIOBONES])
	{
		auto GetHitboxPosition = [](C_BasePlayer* p, int hitbox, matrix3x4_t MatrixArray[MAXSTUDIOBONES]) {

			if (hitbox >= HITBOX_MAX || hitbox < 0)
				return Vector(0, 0, 0);

			const model_t *model = p->GetModel();
			if (!model)
				return Vector(0, 0, 0);

			studiohdr_t *studioHdr = g_MdlInfo->GetStudiomodel(model);
			if (!studioHdr)
				return Vector(0, 0, 0);


			mstudiobbox_t *studioBox = studioHdr->GetHitboxSet(0)->GetHitbox(hitbox);
			if (!studioBox)
				return Vector(0, 0, 0);

			Vector min, max;

			Math::VectorTransform(studioBox->bbmin, MatrixArray[studioBox->bone], min);
			Math::VectorTransform(studioBox->bbmax, MatrixArray[studioBox->bone], max);

			Vector output = (min + max) * 0.5f;

			return output;
		};
		vOut[0] = GetHitboxPosition(pEntity,nHitBoxOne, boneMatrix);
		vOut[1] = GetHitboxPosition(pEntity,nHitBoxTwo, boneMatrix);
	};
	if (ctx.pl->SetupBones(boneMatrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, 0.0f)) {
		GetHitBoxSkeleton(HITBOX_NECK, HITBOX_UPPER_CHEST, ctx.pl, vHitboxSkeletonArray[2], boneMatrix);
		GetHitBoxSkeleton(HITBOX_UPPER_CHEST, HITBOX_CHEST, ctx.pl, vHitboxSkeletonArray[3], boneMatrix);
		GetHitBoxSkeleton(HITBOX_CHEST, HITBOX_LOWER_CHEST, ctx.pl, vHitboxSkeletonArray[4], boneMatrix);
		GetHitBoxSkeleton(HITBOX_LOWER_CHEST, HITBOX_STOMACH, ctx.pl, vHitboxSkeletonArray[5], boneMatrix);
		GetHitBoxSkeleton(HITBOX_STOMACH, HITBOX_LEFT_THIGH, ctx.pl, vHitboxSkeletonArray[6], boneMatrix);
		GetHitBoxSkeleton(HITBOX_LEFT_THIGH, HITBOX_LEFT_CALF, ctx.pl, vHitboxSkeletonArray[7], boneMatrix);
		GetHitBoxSkeleton(HITBOX_LEFT_CALF, HITBOX_LEFT_FOOT, ctx.pl, vHitboxSkeletonArray[8], boneMatrix);
		GetHitBoxSkeleton(HITBOX_STOMACH, HITBOX_RIGHT_THIGH, ctx.pl, vHitboxSkeletonArray[9], boneMatrix);
		GetHitBoxSkeleton(HITBOX_RIGHT_THIGH, HITBOX_RIGHT_CALF, ctx.pl, vHitboxSkeletonArray[10], boneMatrix);
		GetHitBoxSkeleton(HITBOX_RIGHT_CALF, HITBOX_RIGHT_FOOT, ctx.pl, vHitboxSkeletonArray[11], boneMatrix);
		GetHitBoxSkeleton(HITBOX_CHEST, HITBOX_LEFT_UPPER_ARM, ctx.pl, vHitboxSkeletonArray[12], boneMatrix);
		GetHitBoxSkeleton(HITBOX_LEFT_UPPER_ARM, HITBOX_LEFT_FOREARM, ctx.pl, vHitboxSkeletonArray[13], boneMatrix);
		GetHitBoxSkeleton(HITBOX_LEFT_FOREARM, HITBOX_LEFT_HAND, ctx.pl, vHitboxSkeletonArray[14], boneMatrix);
		GetHitBoxSkeleton(HITBOX_CHEST, HITBOX_RIGHT_UPPER_ARM, ctx.pl, vHitboxSkeletonArray[15], boneMatrix);
		GetHitBoxSkeleton(HITBOX_RIGHT_UPPER_ARM, HITBOX_RIGHT_FOREARM, ctx.pl, vHitboxSkeletonArray[16], boneMatrix);
		GetHitBoxSkeleton(HITBOX_RIGHT_FOREARM, HITBOX_RIGHT_HAND, ctx.pl, vHitboxSkeletonArray[17], boneMatrix);
		auto DrawHitBoxLine = [](Vector* vHitBoxArray, Color color)
		{
			//MUT("ESP::DrawHitBoxLine");
			Vector vHitBoxOneScreen;
			Vector vHitBoxTwoScreen;
			if (!vHitBoxArray[0].IsValid() || !vHitBoxArray[1].IsValid()) {
				return;
			}
			if (!g_DebugOverlay->ScreenPosition(vHitBoxArray[0], vHitBoxOneScreen) && !g_DebugOverlay->ScreenPosition(vHitBoxArray[1], vHitBoxTwoScreen))
			{
				Render::Get().draw_list_act->AddLine(Vector2D(vHitBoxOneScreen.x, vHitBoxOneScreen.y),
					Vector2D(vHitBoxTwoScreen.x, vHitBoxTwoScreen.y), color);
			}
			//END();
		};
		for (BYTE IndexArray = 0; IndexArray < 18; IndexArray++)
		DrawHitBoxLine(vHitboxSkeletonArray[IndexArray], ctx.clr);
	}
}

void Visuals::RenderRcs() {
	int w, h;

	g_EngineClient->GetScreenSize(w, h);

	int cx = w / 2;
	int cy = h / 2;
	auto punch = g_LocalPlayer->m_aimPunchAngle();
	int x = cx;
	int y = cy;
	int dx = (cx * 2) / 90.f;
	int dy = (cy * 2) / 90.f;
	int punch_x = (int)(x - (dx*punch.yaw));
	int punch_y = (int)(y + (dy*punch.pitch));

	Render::Get().draw_list_act->AddRectangleFilled(Vector2D(punch_x - 4, punch_y), Vector2D( 9,  1), Utils::Float3ToClr(g_Options.color_esp_crosshair_rcs));
	Render::Get().draw_list_act->AddRectangleFilled(Vector2D(punch_x, punch_y - 4), Vector2D( 1, 9), Utils::Float3ToClr(g_Options.color_esp_crosshair_rcs));
}

void Visuals::RenderFov() {
	int w, h;

	g_EngineClient->GetScreenSize(w, h);

	int cx = w / 2;
	int cy = h / 2;
	bool psile = (weapons[G.WeaponID].pSilent && g_LocalPlayer->m_iShotsFired() <= 1) ? true : false;

	//bool PSilent = (weapons[G.WeaponID].pSilent && ShootFired < 1) ? true : false;

	if (psile) {
		if (weapons[G.WeaponID].Enabled && weapons[G.WeaponID].pSilentFov)
			Render::Get().draw_list_act->AddCircle(Vector2D(cx, cy), cx * weapons[G.WeaponID].pSilentFov * 2.f / 180.f, Color(g_Options.color_esp_fov[0], g_Options.color_esp_fov[1], g_Options.color_esp_fov[2], 0.15f));
	}
	else if (weapons[G.WeaponID].Enabled && weapons[G.WeaponID].Fov)
		Render::Get().draw_list_act->AddCircle(Vector2D(cx, cy), cx * weapons[G.WeaponID].Fov * 2.f /180.f, Color(g_Options.color_esp_fov[0], g_Options.color_esp_fov[1], g_Options.color_esp_fov[2], 0.15f));
}