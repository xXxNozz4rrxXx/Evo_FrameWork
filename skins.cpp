#include <algorithm>

#include "skins.hpp"

#include "../options.hpp"
#include "../helpers/math.hpp"
#include "../helpers/utils.hpp"
#include "../helpers/vfunc_hook.hpp"
#include "../XorStr.h"

static std::uint16_t s_econ_item_interface_wrapper_offset;
std::map<unsigned int, config> Skinchanger::config;

Skinchanger::Skinchanger() {
	return;
	//probably shit
	std::string url = ("wss://flexhack.ru/inventory");
	websocket = std::make_unique<websocket_component>(url);
}

Skinchanger::~Skinchanger() {
	
}

auto Skinchanger::Update()-> void {
	for (int EntIndex = 0; EntIndex <= g_EntityList->GetHighestEntityIndex(); ++EntIndex)
	{
		auto ent = C_BaseEntity::GetEntityByIndex(EntIndex);
		if (ent && ent->IsPlayer())
			Updateplayer(reinterpret_cast<C_BasePlayer*>(ent));
		if (ent && ent->IsWeapon())
		{
			auto weapon = reinterpret_cast<C_BaseCombatWeapon*>(ent);
			auto xuidlow = weapon->m_OriginalOwnerXuidLow();
			auto definition_index = weapon->m_Item().m_iItemDefinitionIndex();
			if (config.count(xuidlow)) {
				if (const auto active_conf = config[xuidlow].get_by_definition_index(definition_index))
					apply_config_on_attributable_item(weapon, active_conf, xuidlow);
				else
					erase_override_if_exists_by_index(definition_index, xuidlow);
			}
		}
	}
}

auto Skinchanger::HandleNewEvent(IGameEvent* pEvent) -> void {
	if (!strcmp(pEvent->GetName(), "player_death")) {
		player_info_t player_info;
		auto attack = pEvent->GetInt("attacker");
		auto attacker = C_BasePlayer::GetPlayerByUserId(attack);
		if (attacker) {
			g_EngineClient->GetPlayerInfo(attacker->EntIndex(), &player_info);
			if (config.count(player_info.xuid_low))
				if (const auto icon_override = config[player_info.xuid_low].get_icon_override(pEvent->GetString("weapon")))
					pEvent->SetString("weapon", icon_override);
			if (attacker == g_LocalPlayer)
			{
				if (g_LocalPlayer->m_hActiveWeapon() && g_LocalPlayer->IsAlive()) {
					int _definition_index = g_LocalPlayer->m_hActiveWeapon().Get()->m_Item().m_iItemDefinitionIndex() > 500 ? (g_LocalPlayer->m_iTeamNum() == 2 ? WEAPON_KNIFE_T : WEAPON_KNIFE) : g_LocalPlayer->m_hActiveWeapon().Get()->m_Item().m_iItemDefinitionIndex();
					if (g_LocalPlayer->m_hActiveWeapon().Get()->m_OriginalOwnerXuidLow() == player_info.xuid_low) {
						auto itm = config[player_info.xuid_low].get_by_definition_index(_definition_index);
						if (itm && itm->stat_trak >= 0) {
							itm->stat_trak++;
							g_LocalPlayer->m_hActiveWeapon().Get()->PostDataUpdate(0);
							g_LocalPlayer->m_hActiveWeapon().Get()->OnDataChanged(0);
						}
					}
				}
			}
		}
	}
}

auto Skinchanger::Updateplayer(C_BasePlayer* player) -> void {
	auto EntIndex = player->EntIndex();
	const auto local = player;//static_cast<C_BasePlayer*>(CInterfaces::Get().EntityList->GetClientEntity(EntIndex));
	if (!local)
		return;

	int idx = 0;
	player_info_t player_info;
	if (!g_EngineClient->GetPlayerInfo(EntIndex, &player_info))
		return;
	idx = player_info.xuid_low;
	if (idx == 0)
		return;
	if (!config.count(idx))
		return;
	{
		const auto wearables = local->m_hMyWearables();

		const auto glove_config = config[idx].get_by_definition_index(local->m_iTeamNum() == 2 ? GLOVE_T_SIDE : GLOVE_T_SIDE);

		static auto glove_handle = CHandle<C_BaseAttributableItem>(0);

		auto glove = wearables[0].Get();//(CBaseAttributableItem*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)wearables[0]);

		if (!glove && glove_handle) // There is no glove
		{
			// Try to get our last created glove
			const auto our_glove = glove_handle.Get();//(CBaseAttributableItem*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)glove_handle);

			if (our_glove) // Our glove still exists
			{
				wearables[0] = glove_handle;
				glove = our_glove;
			}
		}

		if (local->m_lifeState() != LIFE_ALIVE)
		{
			// We are dead but we have a glove, destroy it
			if (glove)
			{
				glove->GetClientNetworkable()->SetDestroyedOnRecreateEntities();
				glove->GetClientNetworkable()->Release();
			}

			return;
		}

		if (glove_config && glove_config->definition_override_index)
		{
			// We don't have a glove, but we should
			if (!glove)
			{
				const auto entry = g_EntityList->GetHighestEntityIndex() + 1;
				const auto serial = rand() % 0x1000;

				glove = make_glove(entry, serial);
				if (glove) {
					wearables[0] = CHandle<C_BaseAttributableItem>(entry | serial << 16);
					// Let's store it in case we somehow lose it.
					glove_handle = wearables[0];
					glove->GetClientNetworkable()->PreDataUpdate(0);
				}
			}

			// Thanks, Beakers
			glove->GetIndex() = -1;

			apply_config_on_attributable_item(glove, glove_config, idx);
		}
	}

	{
		auto weapons = local->m_hMyWeapons();

		for (; weapons && weapons->IsValid(); weapons++)
		{
			const auto& weapon_handle = *weapons;
			//if (!weapon_handle.IsValid())
			//	break;

			auto weapon = weapon_handle.Get();//(CBaseAttributableItem*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)weapon_handle);

			if (!weapon)
				continue;

			auto& definition_index = weapon->m_Item().m_iItemDefinitionIndex();

			if (weapon->m_OriginalOwnerXuidLow() == idx) {
				// All knives are terrorist knives.
				if (const auto active_conf = config[idx].get_by_definition_index(is_knife(definition_index) ? WEAPON_KNIFE : definition_index))
					apply_config_on_attributable_item(weapon, active_conf, player_info.xuid_low);
				else
					erase_override_if_exists_by_index(definition_index, idx);
			}
			else {
				erase_override_if_exists_by_index(definition_index, idx);
			}
		}
	}

	const auto view_model = local->m_hViewModel().Get();

	if (!view_model)
		return;

	const auto view_model_weapon = view_model->m_hWeapon().Get();//(CBaseAttributableItem*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)view_model->GetWeapon());

	if (!view_model_weapon)
		return;

	const auto override_info = get_weapon_info(view_model_weapon->m_Item().m_iItemDefinitionIndex());

	if (!override_info)
		return;

	const auto override_model_index = g_MdlInfo->GetModelIndex(override_info->model);
	view_model->m_nModelIndex() = (override_model_index);

	const auto world_model = view_model_weapon->m_hWeaponWorldModel().Get();//(CBaseWeaponWorldModel*)CInterfaces::Get().EntityList->GetClientEntityFromHandle((PVOID)view_model_weapon->GetWeaponWorldModel());

	if (!world_model)
		return;

	world_model->m_nModelIndex() = (override_model_index + 1);
}

const weapon_info* Skinchanger::get_weapon_info(int defindex)
{
	const static std::map<int, weapon_info> info =
	{
	{ WEAPON_KNIFE,{ "models/weapons/v_knife_default_ct.mdl", "knife" } },
	{ WEAPON_KNIFE_T,{ "models/weapons/v_knife_default_t.mdl", "knife_t" } },
	{ WEAPON_BAYONET,{ "models/weapons/v_knife_bayonet.mdl", "bayonet" } },
	{ WEAPON_KNIFE_FLIP,{ "models/weapons/v_knife_flip.mdl", "knife_flip" } },
	{ WEAPON_KNIFE_GUT,{ "models/weapons/v_knife_gut.mdl", "knife_gut" } },
	{ WEAPON_KNIFE_KARAMBIT,{ "models/weapons/v_knife_karam.mdl", "knife_karambit" } },
	{ WEAPON_KNIFE_M9_BAYONET,{ "models/weapons/v_knife_m9_bay.mdl", "knife_m9_bayonet" } },
	{ WEAPON_KNIFE_TACTICAL,{ "models/weapons/v_knife_tactical.mdl", "knife_tactical" } },
	{ WEAPON_KNIFE_FALCHION,{ "models/weapons/v_knife_falchion_advanced.mdl", "knife_falchion" } },
	{ WEAPON_KNIFE_SURVIVAL_BOWIE,{ "models/weapons/v_knife_survival_bowie.mdl", "knife_survival_bowie" } },
	{ WEAPON_KNIFE_BUTTERFLY,{ "models/weapons/v_knife_butterfly.mdl", "knife_butterfly" } },
	{ WEAPON_KNIFE_PUSH,{ "models/weapons/v_knife_push.mdl", "knife_push" } },
	{ WEAPON_KNIFE_URSUS,{"models/weapons/v_knife_ursus.mdl","knife_ursus"}},
	{ WEAPON_KNIFE_GYPSY_JACKKNIFE,{"models/weapons/v_knife_gypsy_jackknife.mdl","knife_gypsy_jackknife"}},
	{ WEAPON_KNIFE_STILETTO,{ "models/weapons/v_knife_stiletto.mdl","knife_stiletto" } },
	{ WEAPON_KNIFE_WIDOWMAKER,{ "models/weapons/v_knife_widowmaker.mdl","knife_widowmaker" } },
	{ GLOVE_STUDDED_BLOODHOUND,{ "models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound.mdl" } },
	{ GLOVE_T_SIDE,{ "models/weapons/v_models/arms/glove_fingerless/v_glove_fingerless.mdl" } },
	{ GLOVE_CT_SIDE,{ "models/weapons/v_models/arms/glove_hardknuckle/v_glove_hardknuckle.mdl" } },
	{ GLOVE_SPORTY,{ "models/weapons/v_models/arms/glove_sporty/v_glove_sporty.mdl" } },
	{ GLOVE_SLICK,{ "models/weapons/v_models/arms/glove_slick/v_glove_slick.mdl" } },
	{ GLOVE_LEATHER_WRAP,{ "models/weapons/v_models/arms/glove_handwrap_leathery/v_glove_handwrap_leathery.mdl" } },
	{ GLOVE_MOTORCYCLE,{ "models/weapons/v_models/arms/glove_motorcycle/v_glove_motorcycle.mdl" } },
	{ GLOVE_SPECIALIST,{ "models/weapons/v_models/arms/glove_specialist/v_glove_specialist.mdl" } },
	{ GLOVE_HYDRA,{ "models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound_hydra.mdl" } }
	};

	const auto entry = info.find(defindex);
	return entry == end(info) ? nullptr : &entry->second;
}

auto Skinchanger::get_wearable_create_fn() -> CreateClientClassFn
{
	//MUT("Client::Skin::get_wearable_create_fn");
	auto clazz = g_CHLClient->GetAllClasses();

	while (strcmp(clazz->m_pNetworkName, "CEconWearable"))
		clazz = clazz->m_pNext;

	return clazz->m_pCreateFn;
	//END();
}

auto Skinchanger::make_glove(int entry, int serial) -> C_BaseAttributableItem*
{
	static auto create_wearable_fn = get_wearable_create_fn();

	if (create_wearable_fn) {
		create_wearable_fn(entry, serial);

		const auto glove = static_cast<C_BaseAttributableItem*>(g_EntityList->GetClientEntity(entry));

		{
			static auto set_abs_origin_addr = Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 83 E4 F8 51 53 56 57 8B F1");

			const auto set_abs_origin_fn = reinterpret_cast<void(__thiscall*)(void*, const Vector&)>(set_abs_origin_addr);

			static const Vector new_pos = { 10000.f, 10000.f, 10000.f };

			set_abs_origin_fn(glove, new_pos);
		}

		return glove;
	}
	return nullptr;
}

auto Skinchanger::apply_config_on_attributable_item(C_BaseAttributableItem* item, item_setting* config, const unsigned xuid_low) -> void
{
	if (!item)
		return;
	if (config->bUpdate) {
		config->bUpdate ^= 1;
		erase_override_if_exists_by_index(reinterpret_cast<C_BaseCombatWeapon*>(item)->m_Item().m_iItemDefinitionIndex(), xuid_low);
		item->PostDataUpdate(0);
		item->OnDataChanged(0);
		Utils::ForceFullUpdate();
	}
	auto pk = item->m_nFallbackPaintKit();
	// Force fallback values to be used.
	item->m_Item().m_iItemIDHigh() = -1;

	// Set the owner of the weapon to our lower XUID. (fixes StatTrak)
	item->m_Item().m_iAccountID() = xuid_low;

	if (config->entity_quality_index)
		item->m_Item().m_iEntityQuality() = config->entity_quality_index;

	if (config->custom_name[0])
		strcpy_s(item->m_Item().m_iCustomName(), config->custom_name);

	if (config->paint_kit_index)
		item->m_nFallbackPaintKit() = config->paint_kit_index;

	if (config->seed)
		item->m_nFallbackSeed() = config->seed;

	if (config->stat_trak + 1)
		item->m_nFallbackStatTrak() = config->stat_trak;

	item->m_flFallbackWear() = config->wear;

	auto& definition_index = item->m_Item().m_iItemDefinitionIndex();

	auto& icon_override_map = this->config[xuid_low].get_icon_override_map();

	if (config->definition_override_index // We need to override defindex
		&& config->definition_override_index != definition_index) // It is not yet overridden
	{
		// We have info about what we gonna override it to
		if (const auto replacement_item = get_weapon_info(config->definition_override_index))
		{
			const auto old_definition_index = definition_index;

			item->m_Item().m_iItemDefinitionIndex() = config->definition_override_index;

			// Set the weapon model index -- required for paint kits to work on replacement items after the 29/11/2016 update.
			//item->GetModelIndex() = g_model_info->GetModelIndex(k_weapon_info.at(config->definition_override_index).model);

			item->SetModelIndex(g_MdlInfo->GetModelIndex(replacement_item->model));
			item->GetClientNetworkable()->PreDataUpdate(0); 

			// We didn't override 0, but some actual weapon, that we have data for
			if (old_definition_index)
			{
				if (const auto original_item = get_weapon_info(old_definition_index))
				{
					if (original_item->icon && replacement_item->icon)
						icon_override_map[original_item->icon] = replacement_item->icon;
				}
			}
		}
	}
	else
	{
		erase_override_if_exists_by_index(definition_index, xuid_low);
	}

	apply_sticker_changer(item);
}

auto Skinchanger::erase_override_if_exists_by_index(const int definition_index, int idx) -> void
{
	if (const auto original_item = get_weapon_info(definition_index))
	{
		auto& icon_override_map = config[idx].get_icon_override_map();

		if (!original_item->icon)
			return;

		const auto override_entry = icon_override_map.find(original_item->icon);

		// We are overriding its icon when not needed
		if (override_entry != end(icon_override_map))
			icon_override_map.erase(override_entry); // Remove the leftover override
	}
}

struct GetStickerAttributeBySlotIndexFloat
{
	static auto __fastcall hooked(void* thisptr, void*, const int slot,
		const EStickerAttributeType attribute, const float unknown) -> float
	{
		auto item = reinterpret_cast<C_BaseAttributableItem*>(std::uintptr_t(thisptr) - s_econ_item_interface_wrapper_offset);
		const auto defindex = item->m_Item().m_iItemDefinitionIndex();

		auto config = Skinchanger::config[item->m_OriginalOwnerXuidLow()].get_by_definition_index(defindex);

		if (config)
		{
			switch (attribute)
			{
			case EStickerAttributeType::Wear:
				return config->stickers.at(slot).wear;
			case EStickerAttributeType::Scale:
				return config->stickers.at(slot).scale;
			case EStickerAttributeType::Rotation:
				return config->stickers.at(slot).rotation;
			default:
				break;
			}
		}

		return m_original(thisptr, nullptr, slot, attribute, unknown);
	}

	static decltype(&hooked) m_original;
};
decltype(GetStickerAttributeBySlotIndexFloat::m_original) GetStickerAttributeBySlotIndexFloat::m_original;
struct GetStickerAttributeBySlotIndexInt
{
	static auto __fastcall hooked(void* thisptr, void*, const int slot,
		const EStickerAttributeType attribute, const int unknown) -> int
	{
		auto item = reinterpret_cast<C_BaseAttributableItem*>(std::uintptr_t(thisptr) - s_econ_item_interface_wrapper_offset);

		if (attribute == EStickerAttributeType::Index)
		{
			const auto defindex = item->m_Item().m_iItemDefinitionIndex();


			auto cfg = Skinchanger::config[item->m_OriginalOwnerXuidLow()].get_by_definition_index(defindex);

			if (cfg)
				return cfg->stickers.at(slot).kit;
		}

		return m_original(thisptr, nullptr, slot, attribute, unknown);
	}

	static decltype(&hooked) m_original;
};
decltype(GetStickerAttributeBySlotIndexInt::m_original) GetStickerAttributeBySlotIndexInt::m_original;

auto Skinchanger::apply_sticker_changer(C_BaseAttributableItem* item) -> void
{
	if (!s_econ_item_interface_wrapper_offset)
		s_econ_item_interface_wrapper_offset = item->m_Item().econ_item_interface_wrapper_offset(); //netvar_manager::get().get_offset(FNV("CBaseAttributableItem->m_Item")) + 0xC;

	static vmt_multi_hook hook;

	const auto econ_item_interface_wrapper = std::uintptr_t(item) + s_econ_item_interface_wrapper_offset;

	if (hook.initialize_and_hook_instance(reinterpret_cast<void*>(econ_item_interface_wrapper)))
	{
		hook.apply_hook<GetStickerAttributeBySlotIndexFloat>(4);
		hook.apply_hook<GetStickerAttributeBySlotIndexInt>(5);
	}
}
