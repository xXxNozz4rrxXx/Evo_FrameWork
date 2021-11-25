#pragma once

#include "../singleton.hpp"
#include "../helpers/kit_parser.hpp"
#include "../render.hpp"
#include "../helpers/math.hpp"
#include "../valve_sdk/csgostructs.hpp"
#include <map>
#include <vector>
#include <string_view>
#include <memory>
#include "../helpers/websocket_component.hpp"


class WebSocket;
struct game_data::paint_kit;
class Skinchanger;

struct weapon_info
{
	constexpr weapon_info(const char* model, const char* icon = nullptr) :
		model(model),
		icon(icon)
	{}

	const char* model;
	const char* icon;
};
struct weapon_name
{
	constexpr weapon_name(const int definition_index, const char* name) :
		definition_index(definition_index),
		name(name)
	{}

	int definition_index = 0;
	const char* name = nullptr;
};
struct quality_name
{
	constexpr quality_name(const int index, const char* name) :
		index(index),
		name(name)
	{}

	int index = 0;
	const char* name = nullptr;
};
#define VALYE_SYNCER
//#ifdef VALYE_SYNCER

template<typename Container, typename T1, typename T2, typename TC>
class value_syncer
{
	using container_type = typename Container::value_type;

	const Container& container;
	T1& key;
	T2& value;
	const TC container_type::* member;

public:
	value_syncer(const Container& container, T1& key, T2& value, const TC container_type::* member)
		: container{ container }
		, key{ key }
		, value{ value }
		, member{ member }
	{}

	auto key_to_value() const -> void
	{
		key = std::clamp(key, T1(0), T1(container.size() - 1));
		value = container.at(key).*member;
	}

	auto value_to_key() const -> void
	{
		auto it = std::find_if(std::begin(container), std::end(container), [this](const container_type& x)
		{
			return value == x.*member;
		});

		// Originally I wanted this to work with maps too, but fuck that
		if (it != std::end(container))
			key = it - std::begin(container);
		else
			key = T1(0);
	}
};

enum class sync_type
{
	VALUE_TO_KEY,
	KEY_TO_VALUE
};

template<sync_type Type, typename Container, typename T1, typename T2, typename TC>
static auto do_sync(const Container& container, T1& key, T2& value, TC Container::value_type::* member) -> void
{
	auto syncer = value_syncer<Container, T1, T2, TC>{ container, key, value, member };
	if constexpr (Type == sync_type::VALUE_TO_KEY)
		syncer.value_to_key();
	else
		syncer.key_to_value();
}
//#endif
struct sticker_setting
{
#ifdef VALYE_SYNCER
	template<sync_type Type>
	void update()
	{
		do_sync<Type>(game_data::sticker_kits, kit_vector_index, kit, &game_data::paint_kit::id);
	}
#endif
	int kit = 0;
	int kit_vector_index = 0;
	float wear = (std::numeric_limits<float>::min)();
	float scale = 1.f;
	float rotation = 0.f;
};
struct item_setting
{
#ifdef VALYE_SYNCER
	template<sync_type Type>
	void update()
	{
		do_sync<Type>(
			Cloud::Get().skin_component->k_weapon_names,
			definition_vector_index,
			definition_index,
			&weapon_name::definition_index
			);

		do_sync<Type>(
			Cloud::Get().skin_component->k_quality_names,
			entity_quality_vector_index,
			entity_quality_index,
			&quality_name::index
			);

		const std::vector<game_data::paint_kit>* kit_names;
		const std::vector<weapon_name>* defindex_names;

		if (definition_index == GLOVE_T_SIDE || definition_index == GLOVE_T_SIDE)
		{
			kit_names = &game_data::glove_kits;
			defindex_names = &Cloud::Get().skin_component->k_glove_names;
		}
		else
		{
			kit_names = &game_data::skin_kits;
			defindex_names = &Cloud::Get().skin_component->k_knife_names;
		}

		do_sync<Type>(
			*kit_names,
			paint_kit_vector_index,
			paint_kit_index,
			&game_data::paint_kit::id
			);

		do_sync<Type>(
			*defindex_names,
			definition_override_vector_index,
			definition_override_index,
			&weapon_name::definition_index
			);

		for (auto& sticker : stickers)
			sticker.update<Type>();
	}
#endif
	char name[32] = "Default";
	bool enabled = false;
	int definition_vector_index = 0;
	int definition_index = 1;
	int entity_quality_vector_index = 0;
	int entity_quality_index = 0;
	int paint_kit_vector_index = 0;
	int paint_kit_index = 0;
	int definition_override_vector_index = 0;
	int definition_override_index = 0;
	int seed = 0;
	int stat_trak = 0;
	bool bUpdate = false;
	float wear = (std::numeric_limits<float>::min)();
	char custom_name[32] = "";
	std::array<sticker_setting, 5> stickers;
};
class config {
public:
	config()
	{
		m_items.reserve(70);
		m_items.push_back(item_setting());
	}

	auto get_by_definition_index(const int definition_index) -> item_setting*
	{
		auto it = std::find_if(m_items.begin(), m_items.end(), [definition_index](const item_setting& e)
		{
			return e.enabled && e.definition_index == definition_index;
		});

		return it == m_items.end() ? nullptr : &*it;
	}

	auto get_items() -> std::vector<item_setting>&
	{
		return m_items;
	}

	auto get_icon_override_map() -> std::unordered_map<std::string_view, std::string_view>&
	{
		return m_icon_overrides;
	}

	auto get_icon_override(const std::string_view original) const -> const char*
	{
		return m_icon_overrides.count(original) ? m_icon_overrides.at(original).data() : nullptr;
	}
private:
	std::vector<item_setting> m_items;
	std::unordered_map<std::string_view, std::string_view> m_icon_overrides;
};

enum class EStickerAttributeType
{
	Index,
	Wear,
	Scale,
	Rotation
};

class Skinchanger {
	friend class Cloud;
	std::unique_ptr<websocket_component> websocket;   
	auto erase_override_if_exists_by_index(const int definition_index, int idx) -> void;
	auto is_knife(const int i) -> bool
	{
		return (i >= ItemDefinitionIndex::WEAPON_BAYONET && i < ItemDefinitionIndex::GLOVE_STUDDED_BLOODHOUND) || i == ItemDefinitionIndex::WEAPON_KNIFE_T || i == ItemDefinitionIndex::WEAPON_KNIFE;
	}
	auto apply_config_on_attributable_item(C_BaseAttributableItem* item, item_setting* config,const unsigned xuid_low) -> void;
	auto get_wearable_create_fn()->CreateClientClassFn;
	auto make_glove(int entry, int serial)->C_BaseAttributableItem*;
	auto Updateplayer(C_BasePlayer* player) -> void;
	auto apply_sticker_changer(C_BaseAttributableItem* item) -> void;
public:
	Skinchanger();
	~Skinchanger();
	static std::map<unsigned int, config> config;
	const weapon_info* get_weapon_info(int defindex);
	auto HandleNewEvent(IGameEvent* pEvent) -> void;
	auto Update() -> void;
	const std::vector<weapon_name> k_knife_names = {
	{ 0, "Default" },
{ WEAPON_BAYONET, "Bayonet" },
{ WEAPON_KNIFE_FLIP, "Flip Knife" },
{ WEAPON_KNIFE_GUT, "Gut Knife" },
{ WEAPON_KNIFE_KARAMBIT, "Karambit" },
{ WEAPON_KNIFE_M9_BAYONET, "M9 Bayonet" },
{ WEAPON_KNIFE_TACTICAL, "Huntsman Knife" },
{ WEAPON_KNIFE_FALCHION, "Falchion Knife" },
{ WEAPON_KNIFE_SURVIVAL_BOWIE, "Bowie Knife" },
{ WEAPON_KNIFE_BUTTERFLY, "Butterfly Knife" },
{ WEAPON_KNIFE_PUSH, "Shadow Daggers" }
	};;
	const std::vector<weapon_name> k_glove_names = {
	{ 0, "Default" },
{ GLOVE_STUDDED_BLOODHOUND, "Bloodhound" },
{ GLOVE_T_SIDE, "Default (Terrorists)" },
{ GLOVE_CT_SIDE, "Default (Counter-Terrorists)" },
{ GLOVE_SPORTY, "Sporty" },
{ GLOVE_SLICK, "Slick" },
{ GLOVE_LEATHER_WRAP, "Handwrap" },
{ GLOVE_MOTORCYCLE, "Motorcycle" },
{ GLOVE_SPECIALIST, "Specialist" },
{ GLOVE_HYDRA, "Hydra" }
	};;
	const std::vector<weapon_name> k_weapon_names = {
	{ WEAPON_KNIFE, "Knife" },
{ GLOVE_T_SIDE, "Glove" },
{ 7, "AK-47" },
{ 8, "AUG" },
{ 9, "AWP" },
{ 63, "CZ75 Auto" },
{ 1, "Desert Eagle" },
{ 2, "Dual Berettas" },
{ 10, "FAMAS" },
{ 3, "Five-SeveN" },
{ 11, "G3SG1" },
{ 13, "Galil AR" },
{ 4, "Glock-18" },
{ 14, "M249" },
{ 60, "M4A1-S" },
{ 16, "M4A4" },
{ 17, "MAC-10" },
{ 27, "MAG-7" },
{ 33, "MP7" },
{ 34, "MP9" },
{ 28, "Negev" },
{ 35, "Nova" },
{ 32, "P2000" },
{ 36, "P250" },
{ 19, "P90" },
{ 23, "MP5-SD"},
{ 26, "PP-Bizon" },
{ 64, "R8 Revolver" },
{ 29, "Sawed-Off" },
{ 38, "SCAR-20" },
{ 40, "SSG 08" },
{ 39, "SG 553" },
{ 30, "Tec-9" },
{ 24, "UMP-45" },
{ 61, "USP-S" },
{ 25, "XM1014" },
	};;
	const std::vector<quality_name> k_quality_names = {
	{ 0, "Default" },
	{ 1, "Genuine" },
	{ 2, "Vintage" },
	{ 3, "Unusual" },
	{ 5, "Community" },
	{ 6, "Developer" },
	{ 7, "Self-Made" },
	{ 8, "Customized" },
	{ 9, "Strange" },
	{ 10, "Completed" },
	{ 12, "Tournament" }
	};
};