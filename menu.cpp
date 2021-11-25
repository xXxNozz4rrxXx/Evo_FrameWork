#include "Menu.hpp"
#define NOMINMAX
#include <Windows.h>
#include <chrono>
#include "features/profile.hpp"
#include "valve_sdk/csgostructs.hpp"
#include "helpers/input.hpp"
#include "options.hpp"
#include "ui.hpp"
#include "Config.h"
#include "droid.hpp"
#include "features\skins.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "imgui/directx9/imgui_impl_dx9.h"
#include "helpers\kit_parser.hpp"
//#include "helpers\item_definitions.hpp"
#include "features\cloud.hpp"
#include <ctime>

//#include "..\inventory.hpp"

// =========================================================
// 
// These are the tabs on the sidebar
// 
// =========================================================
static char* sidebar_tabs[] = {
	"Aimbot",
	"Visuals",
	"Misc",
	"SkinChanger",
	"Profile Changer",
	"Colors"
};
static char *rus_s[] = {
	u8"АИМ",
	u8"ВХ",
	u8"ДРУГОЕ",
	u8"СКИНЫ",
	u8"ПРОФИЛЬ",
	u8"ЦВЕТА"
};

static ConVar* cl_mouseenable = nullptr;
constexpr static float get_sidebar_item_width() { return 150.0f; }
constexpr static float get_sidebar_item_height() { return  70.0f; }

namespace ImGuiEx
{
	bool identical(const char* buf, const char* item) {
		size_t buf_size = strlen(buf);
		size_t item_size = strlen(item);
		//Check if the item length is shorter or equal --> exclude
		if (buf_size >= item_size) return false;
		for (int i = 0; i < strlen(buf); ++i)
			// set the current pos if matching or return the pos if not
			if (buf[i] != item[i]) return false;
		// Complete match
		// and the item size is greater --> include
		return true;
	}

	int propose(ImGuiTextEditCallbackData* data) {
		//We don't want to "preselect" anything
		if (strlen(data->Buf) == 0) return 0;

		//Get our items back
		std::vector<game_data::paint_kit>& items = static_cast<std::pair<std::vector<game_data::paint_kit>&,int&>*> (data->UserData)->first;
		size_t length = static_cast<std::pair<std::vector<game_data::paint_kit>&, int&>*> (data->UserData)->first.size();

		//We need to give the user a chance to remove wrong input
		//We use SFML Keycodes here, because the Imgui Keys aren't working the way I thought they do...
		if (InputSys::Get().IsKeyDown(VK_BACK)) { //TODO: Replace with imgui key
			//We delete the last char automatically, since it is what the user wants to delete, but only if there is something (selected/marked/hovered)
			//FIXME: This worked fine, when not used as helper function
			if (data->SelectionEnd != data->SelectionStart)
				if (data->BufTextLen > 0) //...and the buffer isn't empty			
					if (data->CursorPos > 0) //...and the cursor not at pos 0
						data->DeleteChars(data->CursorPos - 1, 1);
			return 0;
		}
		if (InputSys::Get().IsKeyDown(VK_DELETE)) return 0; //TODO: Replace with imgui key



		for (int i = 0; i < length; i++) {
			if (identical(data->Buf, items[i].name.c_str())) {
				const int cursor = data->CursorPos;
				//Insert the first match
				static_cast<std::pair<std::vector<game_data::paint_kit>&, int&>*> (data->UserData)->second = i;
				data->DeleteChars(0, data->BufTextLen);
				data->InsertChars(0, items[i].name.c_str());
				//Reset the cursor position
				data->CursorPos = cursor;
				//Select the text, so the user can simply go on writing
				data->SelectionStart = cursor;
				data->SelectionEnd = data->BufTextLen;
				break;
			}
		}
		return 0;
	}

	bool TextInputComboBox(const char* id, std::string& buffer, size_t maxInputSize, std::vector<game_data::paint_kit>& items,int& idx, size_t item_len, short showMaxItems) {
		//Check if both strings matches
		if (showMaxItems == 0)
			showMaxItems = item_len;

		ImGui::PushID(id);
		std::pair<std::vector<game_data::paint_kit>&,int&> pass(items,idx); //We need to pass the array length as well
		buffer.resize(maxInputSize);
		bool ret = ImGui::InputText("##in", &buffer[0], maxInputSize, ImGuiInputTextFlags_::ImGuiInputTextFlags_CallbackAlways, propose, static_cast<void*>(&pass));


		ImGui::OpenPopupOnItemClick("combobox"); //Enable right-click
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();

		ImGui::SameLine(0, 0);
		if (ImGui::ArrowButton("##openCombo", ImGuiDir_Down)) {
			ImGui::OpenPopup("combobox");
		}
		ImGui::OpenPopupOnItemClick("combobox"); //Enable right-click

		pos.y += size.y;
		size.x += ImGui::GetItemRectSize().x;
		size.y += 5 + (size.y * showMaxItems);
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::BeginPopup("combobox", ImGuiWindowFlags_::ImGuiWindowFlags_NoMove)) {

			ImGui::Text("Select one item or type");
			ImGui::Separator();
			for (int i = 0; i < item_len; i++)
				if (ImGui::Selectable(items[i].name.c_str())) {
					buffer =  items[i].name;
					idx = i;
				}

			ImGui::EndPopup();
		}
		ImGui::PopID();

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::Text(id);

		return ret;
	}
    inline bool ColorEdit3(const char* label, Color* v, bool show_alpha = true)
    {
        auto clr = ImVec4{
            v->r() / 255.0f,
            v->g() / 255.0f,
            v->b() / 255.0f,
            v->a() / 255.0f
        };

        if(ImGui::ColorEdit3(label, &clr.x, show_alpha)) {
            v->SetColor(clr.x, clr.y, clr.z, clr.w);
            return true;
        }
        return false;
    }
    inline bool ColorEdit3(const char* label, Color* v)
    {
        return ColorEdit3(label, v, false);
    }
	// ImGui ListBox lambda binder
	static bool ListBox(const char* label, int* current_item, std::function<const char*(int)> lambda, int items_count, int height_in_items)
	{
		return ImGui::ListBox(label, current_item, [](void* data, int idx, const char** out_text)
		{
			*out_text = (*reinterpret_cast<std::function<const char*(int)>*>(data))(idx);
			return true;
		}, &lambda, items_count, height_in_items);
	}
}

template<size_t N>
void render_tabs(char* (&names)[N], int& activetab, float w, float h, bool sameline)
{
    bool values[N] = { false };

    values[activetab] = true;

    for(auto i = 0; i < N; ++i) {
        if(ImGui::ToggleButton(names[i], &values[i], ImVec2{ w, h })) {
            activetab = i;
        }
        if(sameline && i < N - 1)
            ImGui::SameLine();
    }
}

ImVec2 get_sidebar_size()
{
    constexpr float padding = 10.0f;
    constexpr auto size_w = padding * 2.0f + get_sidebar_item_width();
    constexpr auto size_h = padding * 2.0f + (sizeof(sidebar_tabs) / sizeof(char*)) * get_sidebar_item_height();

    return ImVec2{ size_w, ImMax(325.0f, size_h) };
}

int get_fps()
{
    using namespace std::chrono;
    static int count = 0;
    static auto last = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    static int fps = 0;

    count++;

    if(duration_cast<milliseconds>(now - last).count() > 1000) {
        fps = count;
        count = 0;
        last = now;
    }

    return fps;
}


void Menu::DrawRadar()
{
	auto visibleColor = g_Options.radar_vis_en;
	auto visible = Utils::Float3ToClr(g_Options.radar_vis_en);//Color(int(visibleColor[0] * 255.0f), int(visibleColor[1] * 255.0f), int(visibleColor[2] * 255.0f), int(visibleColor[3] * 255.0f));

	auto TvisibleColor = g_Options.radar_vis_al;
	auto Tvisible = Utils::Float3ToClr(g_Options.radar_vis_al);//Color(int(TvisibleColor[0] * 255.0f), int(TvisibleColor[1] * 255.0f), int(TvisibleColor[2] * 255.0f), int(TvisibleColor[3] * 255.0f));

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec2 oldPadding = style.WindowPadding;
	float oldAlpha = style.Colors[ImGuiCol_WindowBg].w;
	style.WindowPadding = ImVec2(0, 0);
	style.Colors[ImGuiCol_WindowBg].w = (float)g_Options.rad_alpha / 255.0f;
	if (ImGui::Begin(g_Options.Russian ? (u8"Радар") : (" Radar"), &_visible, ImVec2(200, 200), (float)g_Options.rad_alpha / 255.0f), ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)
	{
		ImVec2 siz = ImGui::GetWindowSize();
		ImVec2 pos = ImGui::GetWindowPos();

		ImDrawList* windowDrawList = ImGui::GetWindowDrawList();
		windowDrawList->AddLine(ImVec2(pos.x + (siz.x / 2), pos.y + 0), ImVec2(pos.x + (siz.x / 2), pos.y + siz.y), ImColor(64, 72, 95), 1.5f);
		windowDrawList->AddLine(ImVec2(pos.x + 0, pos.y + (siz.y / 2)), ImVec2(pos.x + siz.x, pos.y + (siz.y / 2)), ImColor(64, 72, 95), 1.5f);
		static auto GetU32 = [](Color _color)
		{
			return ((_color[3] & 0xff) << 24) + ((_color[2] & 0xff) << 16) + ((_color[1] & 0xff) << 8)
				+ (_color[0] & 0xff);
		};
		static auto RotatePoint =[](Vector EntityPos, Vector LocalPlayerPos, int posX, int posY, int sizeX, int sizeY, float angle, float zoom)
		{
			float r_1, r_2;
			float x_1, y_1;

			r_1 = -(EntityPos.y - LocalPlayerPos.y);
			r_2 = EntityPos.x - LocalPlayerPos.x;
			float Yaw = angle - 90.0f;

			float yawToRadian = Yaw * (float)(3.14159265358979323846f / 180.0F);
			x_1 = (float)(r_2 * (float)cos((double)(yawToRadian)) - r_1 * sin((double)(yawToRadian))) / 20;
			y_1 = (float)(r_2 * (float)sin((double)(yawToRadian)) + r_1 * cos((double)(yawToRadian))) / 20;


			x_1 *= zoom;
			y_1 *= zoom;

			int sizX = sizeX / 2;
			int sizY = sizeY / 2;

			x_1 += sizX;
			y_1 += sizY;

			if (x_1 < 5)
				x_1 = 5;

			if (x_1 > sizeX - 5)
				x_1 = sizeX - 5;

			if (y_1 < 5)
				y_1 = 5;

			if (y_1 > sizeY - 5)
				y_1 = sizeY - 5;


			x_1 += posX;
			y_1 += posY;


			return Vector2D(x_1, y_1);
		};
		if (g_EngineClient->IsInGame() && g_EngineClient->IsConnected())
		{
			auto pLocalEntity = g_LocalPlayer;
			if (pLocalEntity)
			{
				Vector LocalPos = pLocalEntity->GetEyePos();
				QAngle ang;
				g_EngineClient->GetViewAngles(ang);
				for (auto& player : Cloud::Get().players->players) {
					if (!player.is_alive)
						continue;
					bool bIsEnemy = pLocalEntity->m_iTeamNum() != player.team;

					if (g_Options.rad_enemy_only && !bIsEnemy)
						continue;

					bool viewCheck = false;
					Vector2D EntityPos = RotatePoint(Vector(player.x,player.y,0), LocalPos, pos.x, pos.y, siz.x, siz.y, ang.yaw, g_Options.rad_zoom);

					//ImU32 clr = (bIsEnemy ? (isVisibled ? Color::LightGreen() : Color::Blue()) : Color::White()).GetU32();
					ImU32 clr = GetU32(bIsEnemy ? (visible) : Tvisible );

					int s = 4;
					windowDrawList->AddCircleFilled(ImVec2(EntityPos.x, EntityPos.y), s, clr);
				}
			}
		}
	}
	ImGui::End();
	style.WindowPadding = oldPadding;
	style.Colors[ImGuiCol_WindowBg].w = oldAlpha;
}

void RenderEspTab()
{
	static char* esp_tab_names[] = { "ESP", "Glow", "Chams" }, *rus[] = { u8"ВХ",u8"ГЛОУ",u8"ЧАМСЫ" };
    static int   active_esp_tab = 0;

    bool placeholder_true = true;

    auto& style = ImGui::GetStyle();
    float group_w = ImGui::GetCurrentWindow()->Size.x - style.WindowPadding.x * 2;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    {
        render_tabs(g_Options.Russian ? rus :esp_tab_names, active_esp_tab, group_w / _countof(esp_tab_names), 25.0f, true);
    }

    ImGui::PopStyleVar();
    ImGui::BeginGroupBox("##body_content");
    {
        if(active_esp_tab == 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ style.WindowPadding.x, style.ItemSpacing.y });
            ImGui::Columns(3, nullptr, false);
            ImGui::SetColumnOffset(1, group_w / 3.0f);
            ImGui::SetColumnOffset(2, 2 * group_w / 3.0f);
            ImGui::SetColumnOffset(3, group_w);

            ImGui::Checkbox(g_Options.Russian ? u8"Включить" : "Enabled", &g_Options.esp_enabled);

			ImGui::Combo(g_Options.Russian ? u8"Стиль квадратов" : "Box Style", &g_Options.Style, g_Options.Russian ? u8"Никакой\0Толстый\0Обычный\0Обычный обведенный\0Углы\0Углы с рамкой\0\0" : "None\0Fat\0Default\0Default outline\0Corner\0Corner outline\0\0", -1);
			ImGui::Checkbox(g_Options.Russian ? u8"Проверка на команду" : "Team check", &g_Options.esp_enemies_only);
			ImGui::Checkbox(g_Options.Russian ? u8"Квадраты" : "Box esp", &g_Options.esp_player_boxes);
            ImGui::Checkbox(g_Options.Russian ? u8"Имена" : "Name esp", &g_Options.esp_player_names);
            ImGui::Checkbox(g_Options.Russian ? u8"Жизни" : "Health esp", &g_Options.esp_player_health);
            ImGui::Checkbox(g_Options.Russian ? u8"Броня" : "Armour esp", &g_Options.esp_player_armour);
			ImGui::Combo(g_Options.Russian ? u8"Оружие" : "Weapon esp", &g_Options.esp_weapon_type, g_Options.Russian ? u8"Откл\0Текст\0Иконки\0\0" : "Off\0Text\0Icon\0\0", -1);
			ImGui::Checkbox(g_Options.Russian ? u8"Патроны" : "Ammo esp", &g_Options.esp_show_ammo);
			ImGui::Checkbox(g_Options.Russian ? u8"Только видимые" : "Visible only", &g_Options.esp_visible_only);
			ImGui::Checkbox(g_Options.Russian ? u8"Индикатор ослепления" : "Flashed indicator", &g_Options.esp_show_flashed);
            ImGui::NextColumn();
			ImGui::Checkbox(g_Options.Russian ? u8"Линии" : "Snaplines", &g_Options.esp_player_snaplines);
			ImGui::Checkbox(g_Options.Russian ? u8"Шаги" : "Sound esp", &g_Options.esp_sound);
            ImGui::Checkbox(g_Options.Russian ? u8"Прицел" : "Crosshair", &g_Options.esp_crosshair);
			ImGui::Checkbox(g_Options.Russian ? u8"Прицел для снайперских винтовок" : "Sniper crosshair", &g_Options.esp_sniper_crosshair);
			ImGui::Checkbox(g_Options.Russian ? u8"Показ отдачи" : "Recoil crosshair", &g_Options.esp_rcs);
			ImGui::Checkbox(g_Options.Russian ? u8"Траектория гранат" : "Grenade prediction", &g_Options.misc_nade_pred);
			ImGui::Checkbox(g_Options.Russian ? u8"Отключить ослепление" : "NoFlash", &g_Options.misc_noflash);
			if(g_Options.misc_noflash)
				ImGui::SliderFloat(g_Options.Russian ? u8"Плотность ослепления" : "Noflash alpha", &g_Options.misc_noflash_val, 0, 255);
			ImGui::Checkbox(g_Options.Russian ? u8"Область обнаружения аимботом" : "FOV", &g_Options.esp_fov);
            ImGui::Checkbox(g_Options.Russian ? u8"Оружие на земле" : "Dropped Weapons", &g_Options.esp_dropped_weapons);
            ImGui::Checkbox(g_Options.Russian ? u8"Набор обезвреживания" : "Defuse Kit", &g_Options.esp_defuse_kit);
            ImGui::Checkbox(g_Options.Russian ? u8"Заложенная бомба" : "Planted C4", &g_Options.esp_planted_c4);

            ImGui::NextColumn();
			ImGui::Combo(g_Options.Russian ? u8"Стиль полосок" : "Bar Style", &g_Options.bar_type, g_Options.Russian ? u8"Обычные\0С разделителями\0\0" : "Simple\0Separators\0\0", -1);
			ImGui::Checkbox(g_Options.Russian ? u8"Скелет" : "Skeleton", &g_Options.esp_skeleton);
            ImGui::Columns(1, nullptr, false);
            ImGui::PopStyleVar();
        } else if(active_esp_tab == 1) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ style.WindowPadding.x, style.ItemSpacing.y });
            ImGui::Columns(3, nullptr, false);
            ImGui::SetColumnOffset(1, group_w / 3.0f);
            ImGui::SetColumnOffset(2, 2 * group_w / 3.0f);
            ImGui::SetColumnOffset(3, group_w);

            ImGui::Checkbox(g_Options.Russian ? u8"Включить" : "Enabled", &g_Options.glow_enabled);
            ImGui::Checkbox(g_Options.Russian ? u8"Проверка на команду" : "Team check", &g_Options.glow_enemies_only);
			ImGui::Checkbox(g_Options.Russian ? u8"Только видимые" : "Visible only", &g_Options.glow_visible_only);
            ImGui::Checkbox(g_Options.Russian ? u8"Игроки" : "Players", &g_Options.glow_players);
            ImGui::Checkbox(g_Options.Russian ? u8"Курицы" : "Chickens", &g_Options.glow_chickens);
            ImGui::Checkbox(g_Options.Russian ? u8"Носитель бомбы" : "C4 Carrier", &g_Options.glow_c4_carrier);
            ImGui::Checkbox(g_Options.Russian ? u8"Заложенная бомба" : "Planted C4", &g_Options.glow_planted_c4);
            ImGui::Checkbox(g_Options.Russian ? u8"Набор обезвреживания" : "Defuse Kits", &g_Options.glow_defuse_kits);
            ImGui::Checkbox(g_Options.Russian ? u8"Оружие" : "Weapons", &g_Options.glow_weapons);

            ImGui::NextColumn();

            ImGui::PushItemWidth(100);
            
            ImGui::PopItemWidth();

            ImGui::NextColumn();

            ImGui::Columns(1, nullptr, false);
            ImGui::PopStyleVar();
        } else if(active_esp_tab == 2) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ style.WindowPadding.x, style.ItemSpacing.y });
            ImGui::Columns(3, nullptr, false);
            ImGui::SetColumnOffset(1, group_w / 3.0f);
            ImGui::SetColumnOffset(2, 2 * group_w / 2.9f);
            ImGui::SetColumnOffset(3, group_w);

            ImGui::BeginGroupBox("Players");
            {
				ImGui::Combo(g_Options.Russian ? u8"Тип чамсов" : "Type", &g_Options.chams_player_type, g_Options.Russian ? u8"Выкл\0Близкие\0Дальние\0\0" : "Off\0Short\0Far\0\0", -1); //ImGui::SameLine();
                ImGui::Checkbox(g_Options.Russian ? u8"Проверка на команду" : "Team Check", &g_Options.chams_player_enemies_only);
				ImGui::Checkbox(g_Options.Russian ? u8"Только видимые" : "Visible Only", &g_Options.chams_visible_only);
				ImGui::Combo(g_Options.Russian ? u8"Стиль" : "Style", &g_Options.chams_player_style, g_Options.Russian ? u8"Плоские\0Текстурированые\0Металлические\0\0" : "Flat\0Default\0Metalic\0\0", -1);
				
                
                ImGui::PushItemWidth(110);
                
                ImGui::PopItemWidth();
            }
            ImGui::EndGroupBox();

            ImGui::NextColumn();

            ImGui::BeginGroupBox("Arms");
            {
				ImGui::Text(g_Options.Russian ? u8"Чамсы рук: " : "Chams arms: ");
                ImGui::Checkbox(g_Options.Russian ? u8"Включить" : "Enabled", &g_Options.chams_arms_enabled);
                ImGui::Checkbox(g_Options.Russian ? u8"Сетчатые" : "Wireframe", &g_Options.chams_arms_wireframe);
                ImGui::Checkbox(g_Options.Russian ? u8"Плоские" : "Flat", &g_Options.chams_arms_flat);
                ImGui::Checkbox(g_Options.Russian ? u8"За стеной" : "Ignore-Z", &g_Options.chams_arms_ignorez);
                ImGui::Checkbox(g_Options.Russian ? u8"Стекло" : "Glass", &g_Options.chams_arms_glass);
                ImGui::PushItemWidth(110);
              
                ImGui::PopItemWidth();
            }
            ImGui::EndGroupBox();

            ImGui::Columns(1, nullptr, false);
            ImGui::PopStyleVar();
        }
    }
    ImGui::EndGroupBox();
}

const char* GetWeaponNameById(int id)
{
	switch (id)
	{
	case 1:
		return "deagle";
	case 2:
		return "elite";
	case 3:
		return "fiveseven";
	case 4:
		return "glock";
	case 7:
		return "ak47";
	case 8:
		return "aug";
	case 9:
		return "awp";
	case 10:
		return "famas";
	case 11:
		return "g3sg1";
	case 13:
		return "galilar";
	case 14:
		return "m249";
	case 60:
		return "m4a1_silencer";
	case 16:
		return "m4a1";
	case 17:
		return "mac10";
	case 19:
		return "p90";
	case 24:
		return "ump45";
	case 25:
		return "xm1014";
	case 26:
		return "bizon";
	case 27:
		return "mag7";
	case 28:
		return "negev";
	case 29:
		return "sawedoff";
	case 30:
		return "tec9";
	case 32:
		return "hkp2000";
	case 33:
		return "mp7";
	case 34:
		return "mp9";
	case 35:
		return "nova";
	case 36:
		return "p250";
	case 38:
		return "scar20";
	case 39:
		return "sg556";
	case 40:
		return "ssg08";
	case 61:
		return "usp_silencer";
	case 63:
		return "cz75a";
	case 64:
		return "revolver";
	case 508:
		return "knife_m9_bayonet";
	case 500:
		return "bayonet";
	case 505:
		return "knife_flip";
	case 506:
		return "knife_gut";
	case 507:
		return "knife_karambit";
	case 509:
		return "knife_tactical";
	case 512:
		return "knife_falchion";
	case 514:
		return "knife_survival_bowie";
	case 515:
		return "knife_butterfly";
	case 516:
		return "knife_push";

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
}

void RenderSkinsTab() {
	{

		auto& entries = Cloud::Get().skin_component->config[C_LocalPlayer::xuid_low].get_items();

		static auto selected_id = 0;

		ImGui::Columns(2, nullptr, false);

		// Config selection
		{
			ImGui::PushItemWidth(-1);

			char element_name[64];

			ImGui::ListBox("##config", &selected_id, [&element_name, &entries](int idx)
			{
				sprintf_s(element_name, "%s (%s)", entries.at(idx).name, Cloud::Get().skin_component->k_weapon_names.at(entries.at(idx).definition_vector_index).name);
				return element_name;
			}, entries.size(), 11);

			const auto button_size = ImVec2(ImGui::GetColumnWidth() / 2 - 12.5f, 31);

			if (ImGui::Button(g_Options.Russian ? u8"Добавить" : "Add", button_size))
			{
				entries.push_back(item_setting());
				selected_id = entries.size() - 1;
			}
			ImGui::SameLine();

			if (ImGui::Button(g_Options.Russian ? u8"Удалить" : "Remove", button_size) && entries.size() > 1)
				entries.erase(entries.begin() + selected_id);

			ImGui::PopItemWidth();
		}

		ImGui::NextColumn();

		selected_id = selected_id < int(entries.size()) ? selected_id : entries.size() - 1;

		auto& selected_entry = entries[selected_id];

		{
			ImGui::BeginChild("1111",ImVec2(0,250));
			{
			// Name
			ImGui::InputText(g_Options.Russian ? u8"Имя" : "Name", selected_entry.name, 32);

			// Item to change skins for
			ImGui::Combo(g_Options.Russian ? u8"Предмет" : "Item", &selected_entry.definition_vector_index, [](void* data, int idx, const char** out_text)
			{
				*out_text = Cloud::Get().skin_component->k_weapon_names[idx].name;
				return true;
			}, nullptr, Cloud::Get().skin_component->k_weapon_names.size(), 5);

			// Enabled
			ImGui::Checkbox(g_Options.Russian ? u8"Включить" : "Enabled", &selected_entry.enabled);

			// Pattern Seed
			ImGui::InputInt(g_Options.Russian ? u8"Паттерн" : "Seed", &selected_entry.seed);

			// Custom StatTrak number
			ImGui::InputInt(g_Options.Russian ? u8"Счётчик убийств" : "StatTrak", &selected_entry.stat_trak);

			// Wear Float
			ImGui::SliderFloat(g_Options.Russian ? u8"Потрёпанность" : "Wear", &selected_entry.wear, FLT_MIN, 1.f, "%.10f", 5);

			// Paint kit
			if (selected_entry.definition_index != GLOVE_T_SIDE)
			{
				static  std::string buffer;
				ImGuiEx::TextInputComboBox(g_Options.Russian ? u8"Скин" : "Paint Kit", buffer,32, game_data::skin_kits, selected_entry.paint_kit_vector_index, game_data::skin_kits.size(), 10);
				
			}
			else
			{
				static  std::string buffer;
				//ImGui::Combo(g_Options.Russian ? u8"Скин" : "Paint Kit", &selected_entry.paint_kit_vector_index, [](void* data, int idx, const char** out_text)
				ImGuiEx::TextInputComboBox(g_Options.Russian ? u8"Скин" : "Paint Kit", buffer, 32, game_data::glove_kits, selected_entry.paint_kit_vector_index, game_data::glove_kits.size(), 10);
			}

			// Quality
			ImGui::Combo(g_Options.Russian ? u8"Рарность" : "Quality", &selected_entry.entity_quality_vector_index, [](void* data, int idx, const char** out_text)
			{
				*out_text = Cloud::Get().skin_component->k_quality_names[idx].name;
				return true;
			}, nullptr, Cloud::Get().skin_component->k_quality_names.size(), 5);

			// Yes we do it twice to decide knifes
			selected_entry.update<sync_type::KEY_TO_VALUE>();

			// Item defindex override
			if (selected_entry.definition_index == WEAPON_KNIFE)
			{
				ImGui::Combo(g_Options.Russian ? u8"Нож" : "Knife", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
				{
					*out_text = Cloud::Get().skin_component->k_knife_names.at(idx).name;
					return true;
				}, nullptr, Cloud::Get().skin_component->k_knife_names.size(), 5);
			}
			else if (selected_entry.definition_index == GLOVE_T_SIDE)
			{
				ImGui::Combo(g_Options.Russian ? u8"Перчатки" : "Glove", &selected_entry.definition_override_vector_index, [](void* data, int idx, const char** out_text)
				{
					*out_text = Cloud::Get().skin_component->k_glove_names.at(idx).name;
					return true;
				}, nullptr, Cloud::Get().skin_component->k_glove_names.size(), 5);
			}
			else
			{
				// We don't want to override weapons other than knives or gloves
				static auto unused_value = 0;
				selected_entry.definition_override_vector_index = 0;
				ImGui::Combo("Unavailable", &unused_value, g_Options.Russian ? u8"Для ножей или перчаток" : "For knives or gloves\0");
			}

			selected_entry.update<sync_type::KEY_TO_VALUE>();

			// Custom Name tag
			ImGui::InputText(g_Options.Russian ? u8"Имя оружия" : "Name Tag", selected_entry.custom_name, 32);
		}
		ImGui::EndChild();
		}

		ImGui::NextColumn();

		ImGui::Columns(1, nullptr, false);

		ImGui::Separator();

		{
			ImGui::Columns(2, nullptr, false);

			ImGui::PushID("sticker");

			static auto selected_sticker_slot = 0;

			auto& selected_sticker = selected_entry.stickers[selected_sticker_slot];

			ImGui::PushItemWidth(-1);

			char element_name[64];

			ImGui::ListBox("", &selected_sticker_slot, [&selected_entry, &element_name](int idx)
			{
				auto kit_vector_index = selected_entry.stickers[idx].kit_vector_index;
				sprintf_s(element_name, "#%d (%s)", idx + 1, game_data::sticker_kits.at(kit_vector_index).name.c_str());
				return element_name;
			}, 5, 5);
			ImGui::PopItemWidth();

			ImGui::NextColumn();
			static std::string buffer;
			ImGuiEx::TextInputComboBox(g_Options.Russian ? u8"Стикер" : "Sticker Kit", buffer, 32, game_data::sticker_kits, selected_sticker.kit_vector_index, game_data::sticker_kits.size(), 10);

			ImGui::SliderFloat(g_Options.Russian ? u8"Потёртость" : "Wear", &selected_sticker.wear, FLT_MIN, 1.f, "%.10f", 5);

			ImGui::SliderFloat(g_Options.Russian ? u8"Размер" : "Scale", &selected_sticker.scale, 0.1f, 5.f, "%.3f");

			ImGui::SliderFloat(g_Options.Russian ? u8"Поворот" : "Rotation", &selected_sticker.rotation, 0.f, 360.f);

			ImGui::NextColumn();

			ImGui::PopID();
		}

		ImGui::Columns(1, nullptr, false);

		ImGui::Separator();

		ImGui::Columns(3, nullptr, false);

		ImGui::PushItemWidth(-1);

		// Lower buttons for modifying items and saving
		{
			const auto button_size = ImVec2(ImGui::GetColumnWidth() - 1, 20);

			if (ImGui::Button(g_Options.Russian ? u8"Применить" : "Update", button_size)) {
				Utils::ForceFullUpdate();
				Profile::Get().Update();
			}
				//(*g_client_state)->ForceFullUpdate();
				//g_EngineClient->ClientCmd_Unrestricted("record x;stop"); //this will be changed at a later date.		
			ImGui::NextColumn();
		}

		ImGui::PopItemWidth();

	}
}

void RenderMiscTab()
{
    bool placeholder_true = true;

    auto& style = ImGui::GetStyle();
    float group_w = ImGui::GetCurrentWindow()->Size.x - style.WindowPadding.x * 2;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::ToggleButton(g_Options.Russian ? u8"ДРУГОЕ": "MISC" , &placeholder_true, ImVec2{ group_w, 25.0f });
    ImGui::PopStyleVar();

    ImGui::BeginGroupBox("##body_content");
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ style.WindowPadding.x, style.ItemSpacing.y });
        ImGui::Columns(3, nullptr, false);
        ImGui::SetColumnOffset(1, group_w / 3.0f);
        ImGui::SetColumnOffset(2, 2 * group_w / 3.0f);
        ImGui::SetColumnOffset(3, group_w);

        ImGui::Checkbox(g_Options.Russian ? u8"Авто распрыжка" : "Bunny hop", &g_Options.misc_bhop);
		ImGui::Checkbox(g_Options.Russian ? u8"Авто стрейф" : "Auto strafe", &g_Options.misc_autostrafe);

		ImGui::Checkbox(g_Options.Russian ? u8"3-е лицо" : "Third Person", &g_Options.misc_thirdperson);
		if(g_Options.misc_thirdperson)
			ImGui::SliderFloat(g_Options.Russian ? u8"Дистанция" : "Distance", &g_Options.misc_thirdperson_dist, 0.f, 150.f);
        ImGui::Checkbox(g_Options.Russian ? u8"Без рук" : "No hands", &g_Options.misc_no_hands);
		ImGui::Checkbox(g_Options.Russian ? u8"Показ рангов" : "Rank reveal", &g_Options.misc_showranks);
		ImGui::Checkbox(g_Options.Russian ? u8"Водный знак" : "Watermark##hc", &g_Options.misc_watermark);
		ImGui::Checkbox(g_Options.Russian ? u8"Авто принятие игры" : "Auto accept", &g_Options.autoaccept);
		ImGui::Checkbox(g_Options.Russian ? u8"Список наблюдателей" : "Spectator list", &g_Options.misc_speclist);
		if(g_Options.misc_speclist)
			ImGui::Checkbox(g_Options.Russian ? u8"Только за мной" : "Only me", &g_Options.misc_speclist_you);
		ImGui::Checkbox(g_Options.Russian ? u8"Ножевой бот" : "Knife bot", &g_Options.misc_kb_enabled);
		if (g_Options.misc_kb_enabled) {
			ImGui::Checkbox(g_Options.Russian ? u8"Автоподбор атак" : "Auto knife bot", &g_Options.misc_auto_kb);
			ImGui::Checkbox(g_Options.Russian ? u8"Атаковать спиной" : "Attack 360", &g_Options.misc_full_kb);
		}
        //ImGui::PushItemWidth(-1.0f);
		ImGui::NextColumn();
		ImGui::Checkbox(g_Options.Russian ? u8"Отдаление" : "FOV changer", &g_Options.fov_changer);
        ImGui::SliderInt(g_Options.Russian ? u8"Отдаление модели рук" : "viewmodel_fov:", &g_Options.viewmodel_fov, 68, 120);
		ImGui::SliderFloat(g_Options.Russian ? u8"Отдаление камеры" : "camera fov:", &g_Options.ov_fov, -180, 180);
		ImGui::Text("Postprocessing:");
        ImGui::SliderFloat(g_Options.Russian ? u8"Красный" : "Red", &g_Options.mat_ambient_light_r, 0, 1);
        ImGui::SliderFloat(g_Options.Russian ? u8"Зелёный" : "Green", &g_Options.mat_ambient_light_g, 0, 1);
        ImGui::SliderFloat(g_Options.Russian ? u8"Голубой" : "Blue", &g_Options.mat_ambient_light_b, 0, 1);
		//ImGui::Checkbox(g_Options.Russian ? u8"Облачный радар" : "Cloud radar", &g_Options.Cloud_radar);
		//ImGui::Checkbox(g_Options.Russian ? u8"Облачные скины" : "Cloud skins", &g_Options.Cloud_skins);
		ImGui::Checkbox(g_Options.Russian ? u8"Радар" : "Radar", &g_Options.radar);
		if (g_Options.radar) {
			ImGui::Checkbox(g_Options.Russian ? u8"Рисовать только врагов" : "Enemy Only", &g_Options.rad_enemy_only);
			ImGui::SliderFloat(g_Options.Russian ? u8"Непрозрачность радара" : "Radar alpha", &g_Options.rad_alpha, 0, 255);
			ImGui::SliderFloat(g_Options.Russian ? u8"Увеличение радара" : "Radar zoom", &g_Options.rad_zoom, 0, 4);
		}
        //ImGui::PopItemWidth();

        ImGui::Columns(1, nullptr, false);
        ImGui::PopStyleVar();
    }
    ImGui::EndGroupBox();
}

const char* keyNames[] =
{
	"",
	"Mouse 1",
	"Mouse 2",
	"Cancel",
	"Middle Mouse",
	"Mouse 4",
	"Mouse 5",
	"",
	"Backspace",
	"Tab",
	"",
	"",
	"Clear",
	"Enter",
	"",
	"",
	"Shift",
	"Control",
	"Alt",
	"Pause",
	"Caps",
	"",
	"",
	"",
	"",
	"",
	"",
	"Escape",
	"",
	"",
	"",
	"",
	"Space",
	"Page Up",
	"Page Down",
	"End",
	"Home",
	"Left",
	"Up",
	"Right",
	"Down",
	"",
	"",
	"",
	"Print",
	"Insert",
	"Delete",
	"",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"",
	"",
	"",
	"",
	"",
	"Numpad 0",
	"Numpad 1",
	"Numpad 2",
	"Numpad 3",
	"Numpad 4",
	"Numpad 5",
	"Numpad 6",
	"Numpad 7",
	"Numpad 8",
	"Numpad 9",
	"Multiply",
	"Add",
	"",
	"Subtract",
	"Decimal",
	"Divide",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",

};

void RenderAimTab()
{
    auto& style = ImGui::GetStyle();
    float group_w = ImGui::GetCurrentWindow()->Size.x - style.WindowPadding.x * 2;

    bool placeholder_true = true;
	static int active_sidebar_tab_aim = 0;
	static char* sidebar_tabs_aim[] = {
	"AIMBOT",
	"TRIGGERBOT"
	}, *rus[] = { u8"AИМБОТ",u8"TPИГГЕРБОТ" };
	render_tabs(g_Options.Russian ? rus : sidebar_tabs_aim, active_sidebar_tab_aim, group_w / _countof(sidebar_tabs_aim), 25.0f, true);
	if (active_sidebar_tab_aim == 0)
	{
		ImGui::BeginChild("AimChild1", ImVec2(340, 0), true);
		{

			{
				ImGui::TextColored(ImColor(25, 176, 215), "Global AimBot");
				ImGui::Checkbox(g_Options.Russian ? u8"Включить" : "Enabled", &Legitbot.Enabled);
				ImGui::Combo("Key##1", &Legitbot.key, keyNames, ARRAYSIZE(keyNames));
				ImGui::Checkbox(g_Options.Russian ? u8"Огонь по своим" : "Friendly Fire", &Legitbot.Deathmatch);
				ImGui::Checkbox("Backtrack", &Legitbot.Backtrack);
				ImGui::Checkbox(g_Options.Russian ? u8"Проверка на дым" : "Smoke Check", &Legitbot.SmokeCheck);


				ImGui::Checkbox(g_Options.Russian ? u8"Проверка на прыжок" : "Jump Check", &Legitbot.JumpCheck);
				ImGui::Checkbox(g_Options.Russian ? u8"Проверка на ослепление" : "Flash Check", &Legitbot.FlashCheck);
				//	ImGui::Checkbox(("Draw Fov"), &Legitbot.DrawFov);
				//ImGui::Checkbox(("Auto Fire"), &Legitbot.AutoFire);
				ImGui::Checkbox(g_Options.Russian ? u8"Авто пистолет" : "Auto Pistol", &Legitbot.AutoPistol);
				ImGui::TextColored(ImColor(25, 176, 215), g_Options.Russian ? u8"Тип доводки" : "Smooth type");
				ImGui::Combo("", &Legitbot.AimType, g_Options.Russian ? u8"Обычный\0Лучше\0Криволинейный\0\0":"Default\0Better\0Curve\0\0", -1);
				ImGui::Checkbox(g_Options.Russian ? u8"Задержка после убийства" : "Kill Delay", &Legitbot.KillDelay);
				ImGui::SliderFloat("##0", &Legitbot.KillDelayTime, 0, 2, "%.2f", 1.0F);
				//ImGui::Checkbox("Legit AA", &Legitbot.LegitAA);
				//ImGui::SliderFloat(g_Options.Russian ? u8"Угол legit AA" : "Legit AA Offset", &Legitbot.LegitAA_Offset, -180, 180, "%.2f", 1.0F);


			}

		}ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("AimChild", ImVec2(390 + 55, 0), true);
		{

			{
				int cw = G.WeaponID;
				if (!cw == 0)
				{

					ImGui::TextColored(ImColor(25, 176, 215), "AimBot");
					ImGui::Checkbox(g_Options.Russian ? u8"Включить для данного оружия" : "Enable For Current Weapon", &weapons[cw].Enabled);
					ImGui::Checkbox(g_Options.Russian ? u8"Умный аим" : "Nearest", &weapons[cw].Nearest);
					if (!weapons[cw].Nearest)
					{
						ImGui::Combo(g_Options.Russian ? u8"Часть тела" : "Hitbox", &weapons[cw].Bone, g_Options.Russian ? u8"Голова\0Шея\0Ключицы\0Живот\0Грудь\0Тело\0Верхний желудок\0Нижний желудок\0\0": u8"Head\0\rNeck\0\rLower Neck\0Pelvis\0Chest\0Lower Chest\0Stomach\0Lower stomach\0\0", -1);
					}

					ImGui::SliderFloat(g_Options.Russian ? u8"Радиус" : "FOV", &weapons[cw].Fov, 0, 25);
					ImGui::SliderFloat(g_Options.Russian ? u8"Плавность" : "Smooth", &weapons[cw].Smooth, 0, 200);



					if (cw != WEAPON_AWP || cw != WEAPON_SSG08)
					{
						ImGui::SliderInt(g_Options.Russian ? u8"Контроль отдачи Х" : "RCS X", &weapons[cw].RcsX, 0, 200, "%.0f %");
						ImGui::SliderInt(g_Options.Russian ? u8"Контроль отдачи У" : "RCS Y", &weapons[cw].RcsY, 0, 200, "%.0f %");
					}

					if (cw == WEAPON_AWP || cw == WEAPON_SSG08)
					{
						if (cw == WEAPON_AWP)
						{
							ImGui::Checkbox(g_Options.Russian ? u8"Бытсрое прицеливание" : "Fast-Zoom", &Legitbot.FastZoom[1]);
						}
						else
						{
							ImGui::Checkbox(g_Options.Russian ? u8"Быстрое прицеливание" : "Fast-Zoom", &Legitbot.FastZoom[0]);
						}
					}

					ImGui::SliderInt(g_Options.Russian ? u8"Стартовая пуля" : "Start bullet", &weapons[cw].StartBullet, 1, 30);
					ImGui::SliderInt(g_Options.Russian ? u8"Конечная пуля" : "End bullet", &weapons[cw].EndBullet, 1, 30);

					ImGui::TextColored(ImColor(25, 176, 215), "pSilentAim");
					ImGui::Checkbox(g_Options.Russian ? u8"ПСайлент" : "pSilent", &weapons[cw].pSilent);
					ImGui::SliderInt(g_Options.Russian ? u8"Шанс попадания" : "Percentage", &weapons[cw].pSilentPercentage, 0, 100);
					ImGui::SliderFloat(g_Options.Russian ? u8"Радиус псала" : "FOV##0", &weapons[cw].pSilentFov, 0, 25);
				}
				else
					ImGui::TextColored(ImColor(25, 176, 215), g_Options.Russian ? u8"Возьмите оружие" : "Take weapon");
			}


		}ImGui::EndChild();
	}
	else if (active_sidebar_tab_aim == 1)
	{
		ImGui::BeginChild("AimChild1", ImVec2(340, 0), true);
		{

			{
				ImGui::Checkbox(g_Options.Russian ? u8"Включить" : u8"Activated", &Triggerbot.Enabled);
				ImGui::Combo("Key##2", &Triggerbot.Key, keyNames, ARRAYSIZE(keyNames));
				ImGui::Checkbox(g_Options.Russian ? u8"Стрельба по всем" : u8"Deathmatch", &Triggerbot.Deathmatch);
				ImGui::Checkbox(g_Options.Russian ? u8"Проверка на дым" : u8"SmokeCheck", &Triggerbot.SmokeCheck);
				ImGui::SliderFloat(g_Options.Russian ? u8"Задержка" : u8"Delay", &Triggerbot.Delay, 0, 1, "%.1f %");

			}

		}ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("AimChild", ImVec2(390 + 55, 0), true);
		{

			{
				int cw = G.WeaponID;
				if (!cw == 0)
				{
					ImGui::Checkbox(g_Options.Russian ? u8"Включить" : u8"Activated", &weapons[cw].TriggerEnabled);
					
						//	GUI::Text(u8"Êîñòè");
						ImGui::Selectable(g_Options.Russian ? u8"Голова" : u8"Head", &weapons[cw].TriggerHitboxHead);
						ImGui::Selectable(g_Options.Russian ? u8"Шея" : u8"Neck", &weapons[cw].TriggerHitboxChest);
						ImGui::Selectable(g_Options.Russian ? u8"Туловиже-верх" : u8"Breast", &weapons[cw].TriggerHitboxStomach);
						ImGui::Selectable(g_Options.Russian ? u8"Туловиже-низ" : u8"Belly", &weapons[cw].TriggerHitboxArms);
						ImGui::Selectable(g_Options.Russian ? u8"Ноги" : u8"Legs", &weapons[cw].TriggerHitboxLegs);
						ImGui::SliderInt(g_Options.Russian ? u8"Шанс" : u8"Chance", &weapons[cw].TriggerHitChance, 0.f, 100.f);

					
				}
				else
					ImGui::TextColored(ImColor(25, 176, 215), g_Options.Russian ? u8"Возьмите оружие" : "Take weapon");
			}


		}ImGui::EndChild();
	}
}

void RenderColorsTab()
{
	ImGui::Text(g_Options.Russian ? u8"Bx" : "Esp");
	ImGui::ColorEdit3(g_Options.Russian ? u8"Команда видимая" : "Team Visible", g_Options.color_esp_ally_visible);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Противники видимые" : "Enemies Visible", g_Options.color_esp_enemy_visible);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Команда невидимая" : "Team InVisible", g_Options.color_esp_ally_occluded);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Противники невидимые" : "Enemies InVisible", g_Options.color_esp_enemy_occluded);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Прицел" : "Crosshair", g_Options.color_esp_crosshair);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Выброшенное оружие" : "Dropped Weapons", g_Options.color_esp_weapons);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Набор обезвержеивания" : "Defuse Kit", g_Options.color_esp_defuse);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Поставленная бомба" : "Planted C4", g_Options.color_esp_c4);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Зона обнаружения аимбота" : "FOV", g_Options.color_esp_fov);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Шум" : "Sound esp", g_Options.color_esp_sound);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Траектория гранат" : "Nade prediction", g_Options.color_nade_pred);
	ImGui::Text(g_Options.Russian ? u8"Глоу" : "Glow"); 
	ImGui::ColorEdit3(g_Options.Russian ? u8"Союзники" : "Team", g_Options.color_glow_ally);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Союзники за стеной" : "Team invisible", g_Options.color_glow_ally_invis);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Враги" : "Enemy", g_Options.color_glow_enemy);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Враги за стеной" : "Enemy invisible", g_Options.color_glow_enemy_invis);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Курицы" : "Chickens", g_Options.color_glow_chickens);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Носитель бомбы" : "C4 Carrier", g_Options.color_glow_c4_carrier);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Бомба" : "Planted C4", g_Options.color_glow_planted_c4);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Набор сапёра" : "Defuse Kits", g_Options.color_glow_defuse);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Оружие" : "Weapons", g_Options.color_glow_weapons);
	ImGui::Text(g_Options.Russian ? u8"Чамсы" : "Chams");
	ImGui::ColorEdit3(g_Options.Russian ? u8"Союзники (видимые)" : "Team (Visible)", g_Options.color_chams_player_ally_visible);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Союзники (невидимые)" : "Team (InVisible)", g_Options.color_chams_player_ally_occluded);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Противники (видимый)" : "Enemy (Visible)", g_Options.color_chams_player_enemy_visible);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Противники (невидимый)" : "Enemy (InVisible)", g_Options.color_chams_player_enemy_occluded);
	ImGui::Text(g_Options.Russian ? u8"Чамсы на руки" : "Hands Chams");
	ImGui::ColorEdit3(g_Options.Russian ? u8"Цвет (видимый)" : "Color (Visible)", g_Options.color_chams_arms_visible);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Цвет (невидимый)" : "Color (InVisible)", g_Options.color_chams_arms_occluded);
	ImGui::Text(g_Options.Russian ? u8"Другое" : "Other");
	ImGui::ColorEdit3(g_Options.Russian ? u8"Цвет водяного знака" : "Color watermark", g_Options.color_watermark);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Цвет союзников на радаре" : "Color radar ally", g_Options.radar_vis_al);
	ImGui::ColorEdit3(g_Options.Russian ? u8"Цвет врагов на радаре" : "Color radar enemy", g_Options.radar_vis_en);
}

void RenderProfTab()
{
	if(ImGui::InputInt(g_Options.Russian ? u8"Ранг" : "Rank",&g_Options.rank))
		Profile::Get().Update();
	if (ImGui::InputInt(g_Options.Russian ? u8"Победы" : "Wins", &g_Options.wins))
		Profile::Get().Update();
	if (ImGui::InputInt(g_Options.Russian ? u8"Дружелюбный" : "Friendly", &g_Options.friendly))
		Profile::Get().Update();
	if (ImGui::InputInt(g_Options.Russian ? u8"Весёлый" : "Funny", &g_Options.funny))
		Profile::Get().Update();
	if (ImGui::InputInt(g_Options.Russian ? u8"Учитель" : "Mentor", &g_Options.mentor))
		Profile::Get().Update();

	if (ImGui::InputInt(g_Options.Russian ? u8"Приватный ранг" : "Private rank", &g_Options.private_rank))
		Profile::Get().Update();
	ImGui::NextColumn();
	ImGui::Checkbox(g_Options.Russian ? (u8"Включить изменение медалей") : "Enable Medal Changer", &g_Options.medals);
	if (g_Options.medals) {
		static int medal_id = 0;
		static int selected_entry = -1;
		ImGui::InputInt(g_Options.Russian ? (u8"id медали") : "Medal ID", &medal_id);


		if (ImGui::Button(g_Options.Russian ? (u8"Добавить") : "Add") && medal_id != 0) {
			Profile::Get().medals.insert(Profile::Get().medals.end(), medal_id);
			medal_id = 0;
			selected_entry = -1;
		}

		ImGui::ListBoxHeader(g_Options.Russian ? (u8"Медали") : "Medals");
		for (int m = 0; m < Profile::Get().medals.size(); m++) {
			if (ImGui::Selectable(std::to_string(Profile::Get().medals[m]).c_str())) {
				selected_entry = m;
				//Profile::Get().medals.erase(Profile::Get().medals.begin() + m);
			}
		}
		ImGui::ListBoxFooter();

		if (ImGui::Button(g_Options.Russian ? (u8"Применить") : "Apply##Medals")) {
			Profile::Get().Update();
		}
		if (selected_entry != -1) {
			if (ImGui::Button(g_Options.Russian ? (u8"Надеть") : "Equip")) {
				Profile::Get().wearMedal = Profile::Get().medals[selected_entry];
			}ImGui::SameLine();
			if (ImGui::Button(g_Options.Russian ? (u8"Удалить") : "Remove")) {
				Profile::Get().medals.erase(Profile::Get().medals.begin() + selected_entry);
			}
		}
	}
}

void Menu::Initialize()
{
    _visible = true;

    cl_mouseenable = g_CVar->FindVar("cl_mouseenable");
//	if (std::string(hash).compare(md5(CLoader::Get().getHWID())) != 0)
//		CLoader::debugger_detected = true;
    CreateStyle();
}

void Menu::Shutdown()
{
    ImGui_ImplDX9_Shutdown();
    cl_mouseenable->SetValue(true);
}

void Menu::OnDeviceLost()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
}

void Menu::OnDeviceReset()
{
    ImGui_ImplDX9_CreateDeviceObjects();
}

void Menu::Render()
{
	//if (Cloud::lastdispatch - g_GlobalVars->curtime < -5000)
	//	CLoader::debugger_detected = true;
	//if(CLoader::debugger_detected)
	//	delete g_EngineClient;
	ImGui::GetIO().MouseDrawCursor = _visible;
	if (g_Options.misc_speclist) {
		if (ImGui::Begin(("Spectator List"), &g_Options.misc_speclist, ImVec2(0, 0), 0.7F, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar ))
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			if (Render::Get().specs > 0) Render::Get().spectators += "\n";
			ImVec2 siz = ImGui::CalcTextSize(Render::Get().spectators.c_str());
			ImGui::SetWindowSize(ImVec2(200,  20 + siz.y));
			ImGui::Text((Render::Get().spectators.c_str()));
		}
		ImGui::End();
	}
	if(g_Options.radar)
		DrawRadar();
    if(!_visible)
        return;

    const auto sidebar_size = get_sidebar_size();
    static int active_sidebar_tab = 0;

    //ImGui::PushStyle(_style);

    ImGui::SetNextWindowPos(ImVec2{ 0, 0 }, ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ImVec2{ 1000, 0 }, ImGuiSetCond_Once);

	ImVec2 siz;
	ImVec2 pos;

	//ImGui::SetNextWindowPos(ImVec2(100, 100));

	if (ImGui::Begin("Ev0lution Framework - CS:GO",
		&_visible,
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize )) {

		siz = ImGui::GetWindowSize();
		pos = ImGui::GetWindowPos();
		//auto& style = ImGui::GetStyle();
      
        {
            ImGui::BeginGroupBox("##sidebar", sidebar_size);
            {
				//ImGui::GetCurrentWindow()->Flags &= ~ImGuiWindowFlags_ShowBorders;
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                render_tabs(g_Options.Russian ? rus_s : sidebar_tabs , active_sidebar_tab, get_sidebar_item_width(), get_sidebar_item_height() - 11, false);

				ImGui::PopStyleVar();
				
				ImGui::Spacing();
				ImGui::Combo("", &g_Options.Conf_ini, g_Options.Russian ? u8"Легит\0Полурейдж\0Рейдж\0\0":"Legit\0SameRage\0Rage\0\0", -1);
				if (ImGui::Button(g_Options.Russian ? u8"Сохранить" : "Save", ImVec2(get_sidebar_item_width(), 20)))
					Config->Save();
				if (ImGui::Button(g_Options.Russian ? u8"Загрузить" : "Load", ImVec2(get_sidebar_item_width(), 20)))
					Config->Load();
            }
            ImGui::EndGroupBox();
        }

        ImGui::SameLine();

        // Make the body the same vertical size as the sidebar
        // except for the width, which we will set to auto
        auto size = ImVec2{ 0.0f, sidebar_size.y };

		ImGui::BeginGroupBox("##body", size);
        if(active_sidebar_tab == 0) {
            RenderAimTab();
        } else if(active_sidebar_tab == 1) {
			RenderEspTab();
        } else if(active_sidebar_tab == 2) {
            RenderMiscTab();
		}
		else if (active_sidebar_tab == 3) {
			RenderSkinsTab();
		}
		else if (active_sidebar_tab == 4) {
			RenderProfTab();
		} else if (active_sidebar_tab == 5) {
			RenderColorsTab();
		}
		ImGui::EndGroupBox();

        ImGui::End();
    }
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + siz.y));
	if (ImGui::Begin(("CSGOSimple##1337"), &_visible, ImVec2(siz.x, 25), 1.0F, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::Checkbox("Russian", &g_Options.Russian);
		ImGui::SameLine(siz.x - ImGui::CalcTextSize(("Russian")).x - ImGui::CalcTextSize(("ch4z#0001")).x);
		ImGui::Text(("         ch4z#0001"));
	}
	ImGui::End();
}

void Menu::Show()
{
    _visible = true;
    cl_mouseenable->SetValue(false);
}

void Menu::Hide()
{
    _visible = false;
    cl_mouseenable->SetValue(true);
}

void Menu::Toggle()
{
	cl_mouseenable->SetValue(_visible);
    _visible = !_visible;
}

void Menu::CreateStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 1.0f;
	style.WindowPadding = ImVec2(8, 8);
	style.WindowMinSize = ImVec2(32, 32);
	style.WindowRounding = 16.0f;
	style.FramePadding = ImVec2(4, 3);
	style.FrameRounding = 4.0f;
	style.ItemSpacing = ImVec2(8, 4);
	style.ItemInnerSpacing = ImVec2(4, 4);
	style.TouchExtraPadding = ImVec2(0, 0);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 3.0f;
	style.ScrollbarSize = 2.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 2.0f;
	style.GrabRounding = 4.0f;
	style.DisplayWindowPadding = ImVec2(22, 22);
	style.DisplaySafeAreaPadding = ImVec2(4, 4);
	style.AntiAliasedLines = true;
	style.WindowTitleAlign = ImVec2(0.5, 0.5);

	style.Colors[ImGuiCol_Text] = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.11f, 0.28f, 0.92f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.10f, 0.39f, 0.71f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.11f, 0.28f, 0.66f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.13f, 0.13f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.15f, 0.31f, 0.83f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.17f, 0.14f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.17f, 0.14f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.02f, 0.88f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.12f, 0.64f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.11f, 0.54f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.07f, 0.24f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.13f, 0.66f, 0.76f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.19f, 0.09f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.13f, 0.66f, 0.84f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.17f, 0.14f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.14f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.13f, 0.66f, 1.00f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.16f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.34f, 0.81f, 0.19f, 0.00f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.71f, 0.05f, 0.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.65f, 0.16f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.31f, 0.77f, 0.07f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.37f, 0.71f, 0.06f, 0.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	ImGui::GetStyle() = style;
}

