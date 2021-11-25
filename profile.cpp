#include <algorithm>

#include "profile.hpp"
#include "cloud.hpp"
#include "../options.hpp"
Profile::Profile() {
	this->MatchFramework = **reinterpret_cast<CMatchFramework***>(Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), "8B 0D ? ? ? ? 8B 11 FF 52 3C 8B C8 8B 10") + 0x2);
}
Profile::~Profile() {

}
void Profile::OnRecieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32_t *pcubMsgSize) {
	uint32_t MessageType = *punMsgType & 0x7FFFFFFF;
	if (MessageType == k_EMsgGCClientWelcome)
	{
		CMsgClientWelcome Message;
		try
		{
			if (!Message.ParsePartialFromArray((void*)((DWORD)pubDest + 8), *pcubMsgSize - 8))
				return;
		}
		catch (...) {
			return;
		}
		if (Message.outofdate_subscribed_caches_size() <= 0)
			return;
		CMsgSOCacheSubscribed* Cache = Message.mutable_outofdate_subscribed_caches(0);

		for (int i = 0; i < Cache->objects_size(); i++)
		{
			CMsgSOCacheSubscribed::SubscribedType* Object = Cache->mutable_objects(i);
			if (!Object->has_type_id())
				continue;
			switch (Object->type_id())
			{
			case 1: // Inventory
			{
				for (int j = 0; j < Object->object_data_size(); j++)
				{
					std::string* ObjectData = Object->mutable_object_data(j);

					CSOEconItem Item;

					if (!Item.ParseFromArray((void*)const_cast<char*>(ObjectData->data()), ObjectData->size()))
					{
						continue;
					}
					if (Item.equipped_state_size() <= 0) {
						continue;
					}

					CSOEconItemEquipped* EquippedState = Item.mutable_equipped_state(0);

					if (EquippedState->new_class() == 0 && EquippedState->new_slot() == 55)
					{
						Item.clear_equipped_state();

						ObjectData->resize(Item.ByteSize());
						Item.SerializeToArray((void*)const_cast<char*>(ObjectData->data()), Item.ByteSize());
					}




					ObjectData->resize(Item.ByteSize());
					Item.SerializeToArray((void*)const_cast<char*>(ObjectData->data()), Item.ByteSize());
				}
				auto g_MatchSessionOnlineHost = MatchFramework->GetMatchSession();
				if (g_MatchSessionOnlineHost) {
					auto g_MatchSystem = MatchFramework->GetMatchSystem();
					if (g_MatchSystem)
					{
						auto g_PlayerManager = g_MatchSystem->GetPlayerManager();
						if (g_PlayerManager) {
							auto g_PlayerLocal = g_PlayerManager->GetLocalPlayer(0);
							if (g_PlayerLocal) {
								C_LocalPlayer::xuid_high = g_PlayerLocal->GetXUIDHigh();
							}
						}
					}
				}
				if(g_Options.medals)
				ApplyMedals(Object);
				ApplySkins(Object);
			}
			break;
			}
		}

		if ((uint32_t)Message.ByteSize() <= cubDest - 8)
		{
			Message.SerializeToArray((void*)((DWORD)pubDest + 8), Message.ByteSize());

			*pcubMsgSize = Message.ByteSize() + 8;
		}
	}
	else if (MessageType == k_EMsgGCCStrike15_v2_MatchmakingGC2ClientHello)
	{
		CMsgGCCStrike15_v2_MatchmakingGC2ClientHello Message;
		try
		{
			if (!Message.ParsePartialFromArray((void*)((DWORD)pubDest + 8), *pcubMsgSize - 8)) {
				return;
			}
		}
		catch (...)
		{
			return;
		}

		if (!Message.has_ranking()) {
			return;
		}
		C_LocalPlayer::xuid_low = Message.account_id();
		if(g_Options.rank > 0)
		Message.mutable_ranking()->set_rank_id(g_Options.rank);
		if (g_Options.wins > 0)
		Message.mutable_ranking()->set_wins(g_Options.wins);
		if (g_Options.friendly > 0)
		Message.mutable_commendation()->set_cmd_friendly(g_Options.friendly);
		if (g_Options.funny > 0)
		Message.mutable_commendation()->set_cmd_leader(g_Options.funny);
		if (g_Options.mentor > 0)
		Message.mutable_commendation()->set_cmd_teaching(g_Options.mentor);
		if (g_Options.private_rank > 0)
		Message.set_player_level(g_Options.private_rank);
		if ((uint32_t)Message.ByteSize() <= cubDest - 8)
		{
			Message.SerializeToArray((void*)((DWORD)pubDest + 8), Message.ByteSize());

			*pcubMsgSize = Message.ByteSize() + 8;
		}
	}
}

bool Profile::Update() {
	CMsgClientHello Message;
	CMsgGCCStrike15_v2_MatchmakingClient2GCHello Message1;

	Message.set_client_session_need(1);
	Message.clear_socache_have_versions();

	void* ptr = malloc(Message.ByteSize() + 8);

	void* ptr1 = malloc(Message1.ByteSize() + 8);

	if (!ptr || !ptr1) {
		return false;
	}

	((uint32_t*)ptr)[0] = k_EMsgGCClientHello | ((DWORD)1 << 31);
	((uint32_t*)ptr)[1] = 0;

	((uint32_t*)ptr1)[0] = k_EMsgGCCStrike15_v2_MatchmakingClient2GCHello | ((DWORD)1 << 31);//9129
	((uint32_t*)ptr1)[1] = 0;


	Message.SerializeToArray((void*)((DWORD)ptr + 8), Message.ByteSize());
	Message1.SerializeToArray((void*)((DWORD)ptr1 + 8), Message1.ByteSize());

	bool result = g_GameCoordinator->SendMessage(k_EMsgGCClientHello | ((DWORD)1 << 31), ptr, Message.ByteSize() + 8) == k_EGCResultOK &&
		g_GameCoordinator->SendMessage(k_EMsgGCCStrike15_v2_MatchmakingClient2GCHello | ((DWORD)1 << 31), ptr1, Message1.ByteSize() + 8) == k_EGCResultOK;

	free(ptr);
	free(ptr1);

	return result;
}

bool Profile::OnSendMessage(uint32_t unMsgType, const void* pubData, uint32_t cubData) {
	static bool gotcha = false;
	uint32_t MessageType = unMsgType & 0x7FFFFFFF;

	if (MessageType == k_EMsgGCAdjustItemEquippedState)
	{
		CMsgAdjustItemEquippedState Message;

		try
		{
			if (!Message.ParsePartialFromArray((void*)((DWORD)pubData + 8), cubData - 8)) {
				return true;
			}
		}
		catch (...)
		{
			return true;
		}

		if (!Message.has_item_id() || !Message.has_new_class() || !Message.has_new_slot()) {
			return true;
		}

		//for (const auto& ItemConfig : Cloud::Get().skin_component->config[C_LocalPlayer::xuid_low].get_items())
		//{
		//	auto ItemData = ItemConfig;
		//	if (ItemData->GetEquippedState(Message.new_class()) == Message.new_slot())
		//		ItemData->ClearEquippedState(Message.new_class());
		//}

		//uint32_t ItemIndex = ((uint32_t)Message.item_id() - 20000);

		//if (ItemIndex < WEAPON_INVALID || ItemIndex > WEAPON_MAX)
		//	return true;
		//return true;

		//CSkinChangerItem* ItemData = E::SkinChanger->GetWeaponCustomSkin(ItemIndex);

		//if (!ItemData)
		//	return true;

		//ItemData->SetEquippedState(Message.new_class(), Message.new_slot());
		return true;
	}
	else if (MessageType == 2500 && !gotcha)
	{
		CMsgStoreGetUserData Message;
		try
		{
			if (!Message.ParsePartialFromArray((void*)((DWORD)pubData + 8), cubData - 8)) {
				return true;
			}
		}
		catch (...)
		{
			return true;
		}
	}
	return true;
}

void Profile::ApplyMedals(CMsgSOCacheSubscribed::SubscribedType* pInventoryCacheObject) {
	CSOEconItem Medal;

	Medal.set_account_id(g_SteamUser->GetSteamID().GetAccountID());
	Medal.set_origin(9);
	Medal.set_rarity(6);
	Medal.set_quantity(1);
	Medal.set_quality(4);
	Medal.set_level(1);

	CSOEconItemAttribute* TimeAcquiredAttribute = Medal.add_attribute();
	uint32_t TimeAcquiredAttributeValue = 0;

	TimeAcquiredAttribute->set_def_index(222);

	TimeAcquiredAttribute->set_value_bytes(&TimeAcquiredAttributeValue, 4);

	int i = 10000;

	for (uint32_t MedalIndex : medals)
	{
		if (MedalIndex > 0) {
			Medal.set_def_index(MedalIndex);
			Medal.set_inventory(i);
			Medal.set_id(i);

			pInventoryCacheObject->add_object_data(Medal.SerializeAsString());

			i++;
		}
	}
	if (wearMedal > 0) {
		//pInventoryCacheObject->add_object_data(Medal.SerializeAsString());
		Medal.set_def_index(wearMedal);
		Medal.set_inventory(i);
		Medal.set_id(i);

		CSOEconItemEquipped* EquippedState = Medal.add_equipped_state();

		EquippedState->set_new_class(0);
		EquippedState->set_new_slot(55);
	}
	
	pInventoryCacheObject->add_object_data(Medal.SerializeAsString());
}

void Profile::ApplySkins(CMsgSOCacheSubscribed::SubscribedType* pInventoryCacheObject) {
	struct WeaponSkin_t {
		int Fallback;
		int Rarity;
	};
	///*

	static std::map<int, int> slots = {
		{1,6},
		{2,3},
		{3,5},
		{4,2},
		{ 7,15},
		{ 8,17 },
		{ 9,18 },
		{ 10,14 },
		{ 11,19 },
		{ 13,14 },
		{ 14,23 },
		{ 16,15 },
		{ 17,8 },
		{ 18,14 },
		{ 19,11 },
		{ 24,10 },
		{ 25,21 },
		{ 26,12 },
		{ 27,22 },
		{ 28,24 },
		{ 29,22 },
		{ 30,5 },
		{ 32,2 },
		{ 33,9 },
		{ 34,8 },
		{ 35,20 },
		{ 36,4 },
		{ 38,19},
		{ 39,17},
		{ 40,16 },
		{ WEAPON_KNIFE_T,0},
		{ WEAPON_KNIFE,0 },
		{ 5000,41},
		{ 5001,41 },
		{ 60,15},
		{ 61,2},
		{ 63,5 },
		{ 64,6 }
	};

	static std::map<int, int> teams = {
		{ 1,-1 },
		{ 2,-1 },
		{ 3,3 },
		{ 4,2 },
		{ 7,2 },
		{ 8,3 },
		{ 9,-1 },
		{ 10,3 },
		{ 11,2 },
		{ 13,2 },
		{ 14,-1 },
		{ 16,3 },
		{ 17,2 },
		{ 18,-1 },
		{ 19,-1 },
		{23,-1},
		{ 24,-1 },
		{ 25,-1 },
		{ 26,-1 },
		{WEAPON_KNIFE_T,2},
		{WEAPON_KNIFE,3},
		{ 5000,2 },
		{ 5001,3 },
		{ 27,3 },
		{ 28,-1 },
		{ 29,2 },
		{ 30,2 },
		{ 32,3 },
		{ 33,-1 },
		{ 34,3 },
		{ 35,-1 },
		{ 36,-1 },
		{ 38,3 },
		{ 39,2 },
		{ 40,-1},
		{ 60,3 },
		{ 61,3 },
		{ 63,-1 },
		{ 64,-1 }
	};

	static std::map<int, std::vector<WeaponSkin_t> > k_allskins = {
		{
			29,{
			{ 5, 1 },
			{ 30, 2 },
			{ 41, 3 },
			{ 83, 4 },
			{ 119, 1 },
			{ 171, 1 },
			{ 204, 2 },
			{ 246, 3 },
			{ 250, 3 },
			{ 256, 6 },
			{ 323, 2 },
			{ 345, 3 },
			{ 390, 4 },
			{ 405, 4 },
			{ 434, 3 },
			{ 458, 1 },
			{ 517, 3 },
			{ 552, 3 },
			{ 596, 4 },
			{ 638, 5 },
			{ 655, 3 },
			{ 673, 3 },
			}
		},
		{
			25,{
			{ 42, 2 },
			{ 95, 1 },
			{ 96, 1 },
			{ 135, 1 },
			{ 166, 3 },
			{ 169, 2 },
			{ 205, 1 },
			{ 238, 3 },
			{ 240, 2 },
			{ 314, 4 },
			{ 320, 3 },
			{ 348, 3 },
			{ 370, 3 },
			{ 393, 5 },
			{ 407, 3 },
			{ 505, 3 },
			{ 521, 4 },
			{ 557, 4 },
			{ 616, 3 },
			{ 654, 4 },
			{ 689, 4 },
			{ 706, 3 },
		} },
		{ 516,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 411, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 418, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 617, 6 },
			{ 618, 4 },
			{ 619, 5 },
		} },
		{ 38,{
			{ 46, 1 },
			{ 70, 2 },
			{ 100, 1 },
			{ 116, 1 },
			{ 157, 2 },
			{ 165, 5 },
			{ 196, 4 },
			{ 232, 3 },
			{ 298, 1 },
			{ 312, 5 },
			{ 391, 5 },
			{ 406, 3 },
			{ 502, 3 },
			{ 518, 3 },
			{ 597, 5 },
			{ 612, 4 },
			{ 642, 3 },
			{ 685, 3 },
		} },
		{ 515,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 411, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 418, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 617, 6 },
			{ 618, 4 },
			{ 619, 5 },
		} },
		{ 61,{
			{ 25, 2 },
			{ 60, 4 },
			{ 183, 4 },
			{ 217, 3 },
			{ 221, 5 },
			{ 236, 3 },
			{ 277, 3 },
			{ 290, 4 },
			{ 313, 4 },
			{ 318, 4 },
			{ 332, 2 },
			{ 339, 5 },
			{ 364, 3 },
			{ 454, 2 },
			{ 489, 3 },
			{ 504, 6 },
			{ 540, 3 },
			{ 637, 4 },
			{ 653, 6 },
			{ 657, 3 },
			{ 705, 5 },
		} },
		{ 5031,{
			{ 10013, 6 },
			{ 10015, 6 },
			{ 10016, 6 },
			{ 10040, 6 },
			{ 10041, 6 },
			{ 10042, 6 },
			{ 10043, 6 },
			{ 10044, 6 },
		} },
		{ 512,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 411, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 621, 3 },
		} },
		{ 28,{
			{ 28, 3 },
			{ 201, 2 },
			{ 240, 2 },
			{ 285, 3 },
			{ 298, 1 },
			{ 317, 3 },
			{ 355, 3 },
			{ 369, 2 },
			{ 432, 3 },
			{ 483, 4 },
			{ 514, 4 },
			{ 610, 3 },
			{ 698, 4 },
		} },
		{ 64,{
			{ 12, 3 },
			{ 27, 1 },
			{ 522, 6 },
			{ 523, 5 },
			{ 595, 4 },
			{ 683, 5 },
			{ 701, 3 },
		} },
		{ 35,{
			{ 3, 2 },
			{ 25, 1 },
			{ 62, 5 },
			{ 99, 1 },
			{ 107, 1 },
			{ 158, 1 },
			{ 164, 3 },
			{ 166, 3 },
			{ 170, 1 },
			{ 191, 3 },
			{ 214, 4 },
			{ 225, 3 },
			{ 263, 4 },
			{ 286, 5 },
			{ 294, 2 },
			{ 299, 2 },
			{ 356, 4 },
			{ 450, 1 },
			{ 484, 3 },
			{ 537, 5 },
			{ 590, 3 },
			{ 634, 4 },
			{ 699, 4 },
		} },
		{ 7,{
			{ 14, 5 },
			{ 44, 5 },
			{ 72, 2 },
			{ 122, 2 },
			{ 170, 2 },
			{ 172, 3 },
			{ 180, 6 },
			{ 226, 4 },
			{ 282, 5 },
			{ 300, 4 },
			{ 302, 6 },
			{ 316, 6 },
			{ 340, 5 },
			{ 341, 4 },
			{ 380, 6 },
			{ 394, 5 },
			{ 422, 3 },
			{ 456, 5 },
			{ 474, 6 },
			{ 490, 5 },
			{ 506, 5 },
			{ 524, 6 },
			{ 600, 6 },
			{ 639, 6 },
			{ 656, 4 },
			{ 675, 6 },
		} },
		{ 500,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 410, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 558, 5 },
			{ 563, 4 },
			{ 568, 5 },
			{ 569, 4 },
			{ 570, 4 },
			{ 571, 4 },
			{ 572, 4 },
			{ 573, 4 },
			{ 578, 3 },
			{ 580, 3 },
		} },
		{ 16,{
			{ 8, 2 },
			{ 16, 2 },
			{ 17, 2 },
			{ 101, 2 },
			{ 155, 6 },
			{ 164, 4 },
			{ 167, 3 },
			{ 176, 3 },
			{ 187, 4 },
			{ 215, 6 },
			{ 255, 6 },
			{ 309, 7 },
			{ 336, 6 },
			{ 384, 4 },
			{ 400, 5 },
			{ 449, 5 },
			{ 471, 4 },
			{ 480, 4 },
			{ 512, 6 },
			{ 533, 6 },
			{ 588, 5 },
			{ 632, 6 },
			{ 664, 5 },
			{ 695, 6 },
		} },
		{ 63,{
			{ 12, 3 },
			{ 218, 3 },
			{ 268, 4 },
			{ 269, 5 },
			{ 270, 6 },
			{ 297, 3 },
			{ 298, 1 },
			{ 315, 3 },
			{ 322, 3 },
			{ 325, 4 },
			{ 334, 3 },
			{ 350, 4 },
			{ 366, 1 },
			{ 435, 4 },
			{ 453, 3 },
			{ 476, 5 },
			{ 543, 4 },
			{ 602, 3 },
			{ 622, 3 },
			{ 643, 5 },
			{ 687, 4 },
		} },
		{ 10,{
			{ 22, 1 },
			{ 47, 1 },
			{ 92, 2 },
			{ 154, 5 },
			{ 178, 3 },
			{ 194, 4 },
			{ 218, 3 },
			{ 244, 3 },
			{ 260, 4 },
			{ 288, 4 },
			{ 371, 4 },
			{ 429, 5 },
			{ 477, 4 },
			{ 492, 3 },
			{ 529, 4 },
			{ 604, 6 },
			{ 626, 5 },
			{ 659, 3 },
		} },
		{ 8,{
			{ 9, 5 },
			{ 10, 3 },
			{ 33, 3 },
			{ 46, 1 },
			{ 47, 1 },
			{ 73, 3 },
			{ 100, 1 },
			{ 110, 2 },
			{ 197, 3 },
			{ 280, 6 },
			{ 305, 4 },
			{ 375, 2 },
			{ 444, 1 },
			{ 455, 6 },
			{ 507, 3 },
			{ 541, 5 },
			{ 583, 4 },
			{ 601, 5 },
			{ 674, 3 },
			{ 690, 5 },
		} },
		{ 17,{
			{ 3, 2 },
			{ 17, 1 },
			{ 32, 2 },
			{ 38, 3 },
			{ 98, 3 },
			{ 101, 1 },
			{ 157, 2 },
			{ 188, 4 },
			{ 246, 3 },
			{ 284, 4 },
			{ 310, 4 },
			{ 333, 1 },
			{ 337, 4 },
			{ 343, 2 },
			{ 372, 3 },
			{ 402, 4 },
			{ 433, 6 },
			{ 498, 3 },
			{ 534, 3 },
			{ 589, 3 },
			{ 651, 4 },
			{ 665, 3 },
			{ 682, 3 },
		} },
		{ 14,{
			{ 22, 1 },
			{ 75, 2 },
			{ 202, 1 },
			{ 243, 2 },
			{ 266, 3 },
			{ 401, 3 },
			{ 452, 2 },
			{ 472, 1 },
			{ 496, 4 },
			{ 547, 3 },
			{ 648, 4 },
		} },
		{ 33,{
			{ 5, 1 },
			{ 11, 3 },
			{ 15, 2 },
			{ 28, 3 },
			{ 102, 3 },
			{ 141, 2 },
			{ 209, 1 },
			{ 213, 4 },
			{ 245, 1 },
			{ 250, 3 },
			{ 354, 3 },
			{ 365, 1 },
			{ 423, 3 },
			{ 442, 1 },
			{ 481, 5 },
			{ 500, 4 },
			{ 536, 4 },
			{ 627, 3 },
			{ 649, 3 },
			{ 696, 6 },
		} },
		{ 13,{
			{ 76, 2 },
			{ 83, 4 },
			{ 119, 1 },
			{ 192, 3 },
			{ 216, 3 },
			{ 235, 2 },
			{ 237, 2 },
			{ 241, 1 },
			{ 264, 3 },
			{ 297, 3 },
			{ 308, 3 },
			{ 379, 4 },
			{ 398, 6 },
			{ 428, 5 },
			{ 460, 3 },
			{ 478, 3 },
			{ 494, 4 },
			{ 546, 4 },
			{ 629, 3 },
			{ 647, 4 },
			{ 661, 5 },
		} },
		{ 36,{
			{ 15, 2 },
			{ 27, 1 },
			{ 34, 2 },
			{ 77, 1 },
			{ 99, 1 },
			{ 102, 3 },
			{ 162, 4 },
			{ 164, 3 },
			{ 168, 4 },
			{ 207, 2 },
			{ 219, 3 },
			{ 230, 3 },
			{ 258, 5 },
			{ 271, 5 },
			{ 295, 5 },
			{ 358, 4 },
			{ 373, 2 },
			{ 388, 5 },
			{ 404, 5 },
			{ 426, 3 },
			{ 466, 2 },
			{ 467, 1 },
			{ 501, 4 },
			{ 551, 5 },
			{ 592, 3 },
			{ 650, 3 },
			{ 668, 4 },
			{ 678, 6 },
		} },
		{ 509,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 411, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 620, 3 },
		} },
		{ 2,{
			{ 28, 3 },
			{ 43, 2 },
			{ 46, 1 },
			{ 47, 1 },
			{ 153, 4 },
			{ 190, 3 },
			{ 220, 4 },
			{ 249, 4 },
			{ 261, 4 },
			{ 276, 3 },
			{ 307, 3 },
			{ 330, 1 },
			{ 396, 4 },
			{ 447, 4 },
			{ 450, 1 },
			{ 491, 3 },
			{ 528, 3 },
			{ 544, 3 },
			{ 625, 4 },
			{ 658, 5 },
		} },
		{ 60,{
			{ 60, 4 },
			{ 77, 2 },
			{ 189, 4 },
			{ 217, 3 },
			{ 235, 3 },
			{ 254, 4 },
			{ 257, 5 },
			{ 301, 5 },
			{ 321, 5 },
			{ 326, 5 },
			{ 360, 6 },
			{ 383, 4 },
			{ 430, 6 },
			{ 440, 4 },
			{ 445, 5 },
			{ 497, 6 },
			{ 548, 6 },
			{ 587, 6 },
			{ 631, 4 },
			{ 644, 5 },
			{ 663, 3 },
			{ 681, 5 },
		} },
		{ 506,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 410, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 560, 5 },
			{ 565, 4 },
			{ 568, 5 },
			{ 569, 4 },
			{ 570, 4 },
			{ 571, 4 },
			{ 572, 4 },
			{ 575, 4 },
			{ 578, 3 },
			{ 580, 3 },
		} },
		{ 1,{
			{ 12, 3 },
			{ 17, 2 },
			{ 37, 4 },
			{ 40, 2 },
			{ 61, 5 },
			{ 90, 2 },
			{ 185, 6 },
			{ 231, 5 },
			{ 232, 4 },
			{ 237, 3 },
			{ 273, 4 },
			{ 296, 3 },
			{ 328, 4 },
			{ 347, 4 },
			{ 351, 5 },
			{ 397, 4 },
			{ 425, 3 },
			{ 468, 2 },
			{ 469, 4 },
			{ 470, 4 },
			{ 509, 3 },
			{ 527, 5 },
			{ 603, 4 },
			{ 645, 3 },
		} },
		{ 24,{
			{ 15, 2 },
			{ 17, 1 },
			{ 37, 3 },
			{ 70, 2 },
			{ 93, 1 },
			{ 169, 2 },
			{ 175, 1 },
			{ 193, 3 },
			{ 281, 3 },
			{ 333, 1 },
			{ 362, 3 },
			{ 392, 3 },
			{ 436, 4 },
			{ 441, 3 },
			{ 488, 3 },
			{ 556, 5 },
			{ 615, 3 },
			{ 652, 4 },
			{ 672, 3 },
			{ 688, 4 },
			{ 704, 4 },
		} },
		{ 27,{
			{ 32, 2 },
			{ 34, 2 },
			{ 39, 4 },
			{ 99, 1 },
			{ 100, 1 },
			{ 171, 1 },
			{ 177, 3 },
			{ 198, 3 },
			{ 291, 3 },
			{ 385, 3 },
			{ 431, 4 },
			{ 462, 3 },
			{ 473, 1 },
			{ 499, 3 },
			{ 535, 4 },
			{ 608, 4 },
			{ 633, 3 },
			{ 666, 3 },
			{ 703, 4 },
		} },
		{ 508,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 411, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 562, 5 },
			{ 567, 4 },
			{ 568, 5 },
			{ 569, 4 },
			{ 570, 4 },
			{ 571, 4 },
			{ 572, 4 },
			{ 577, 4 },
			{ 579, 3 },
			{ 581, 3 },
		} },
		{ 34,{
			{ 33, 3 },
			{ 39, 4 },
			{ 61, 4 },
			{ 100, 1 },
			{ 141, 2 },
			{ 148, 1 },
			{ 199, 1 },
			{ 262, 4 },
			{ 329, 3 },
			{ 366, 1 },
			{ 368, 3 },
			{ 386, 3 },
			{ 403, 3 },
			{ 448, 3 },
			{ 482, 4 },
			{ 549, 3 },
			{ 609, 5 },
			{ 630, 3 },
			{ 679, 4 },
			{ 697, 3 },
		} },
		{ 30,{
			{ 2, 1 },
			{ 17, 1 },
			{ 36, 3 },
			{ 159, 3 },
			{ 179, 4 },
			{ 206, 1 },
			{ 216, 3 },
			{ 235, 2 },
			{ 242, 1 },
			{ 248, 4 },
			{ 272, 4 },
			{ 289, 3 },
			{ 303, 3 },
			{ 374, 3 },
			{ 439, 2 },
			{ 459, 1 },
			{ 463, 3 },
			{ 520, 4 },
			{ 539, 3 },
			{ 555, 4 },
			{ 599, 3 },
			{ 614, 5 },
			{ 671, 3 },
			{ 684, 3 },
		} },
		{ 505,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 410, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 559, 5 },
			{ 564, 4 },
			{ 568, 5 },
			{ 569, 4 },
			{ 570, 4 },
			{ 571, 4 },
			{ 572, 4 },
			{ 574, 4 },
			{ 578, 3 },
			{ 580, 3 },
		} },
		{ 4,{
			{ 2, 2 },
			{ 3, 3 },
			{ 38, 4 },
			{ 40, 2 },
			{ 48, 4 },
			{ 159, 4 },
			{ 208, 2 },
			{ 230, 4 },
			{ 278, 3 },
			{ 293, 2 },
			{ 353, 5 },
			{ 367, 3 },
			{ 381, 4 },
			{ 399, 3 },
			{ 437, 5 },
			{ 479, 3 },
			{ 495, 3 },
			{ 532, 4 },
			{ 586, 6 },
			{ 607, 4 },
			{ 623, 3 },
			{ 680, 3 },
			{ 694, 4 },
		} },
		{ 40,{
			{ 26, 1 },
			{ 60, 3 },
			{ 96, 1 },
			{ 99, 1 },
			{ 200, 2 },
			{ 222, 6 },
			{ 233, 2 },
			{ 253, 3 },
			{ 304, 3 },
			{ 319, 3 },
			{ 361, 3 },
			{ 503, 5 },
			{ 538, 3 },
			{ 554, 4 },
			{ 624, 6 },
			{ 670, 4 },
		} },
		{ 514,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 411, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
		} },
		{ 26,{
			{ 13, 4 },
			{ 25, 1 },
			{ 70, 2 },
			{ 148, 1 },
			{ 149, 1 },
			{ 159, 3 },
			{ 164, 3 },
			{ 171, 1 },
			{ 203, 3 },
			{ 224, 3 },
			{ 236, 2 },
			{ 267, 3 },
			{ 306, 4 },
			{ 349, 4 },
			{ 376, 2 },
			{ 457, 1 },
			{ 508, 4 },
			{ 526, 3 },
			{ 542, 6 },
			{ 594, 3 },
			{ 641, 3 },
			{ 676, 5 },
			{ 692, 3 },
		} },
		{ 19,{
			{ 20, 4 },
			{ 67, 5 },
			{ 100, 1 },
			{ 111, 3 },
			{ 124, 1 },
			{ 156, 6 },
			{ 169, 2 },
			{ 175, 1 },
			{ 182, 5 },
			{ 228, 4 },
			{ 234, 2 },
			{ 244, 3 },
			{ 283, 5 },
			{ 311, 3 },
			{ 335, 3 },
			{ 342, 2 },
			{ 359, 6 },
			{ 486, 3 },
			{ 516, 5 },
			{ 593, 4 },
			{ 611, 3 },
			{ 636, 5 },
			{ 669, 4 },
		} },
		{ 32,{
			{ 21, 2 },
			{ 32, 3 },
			{ 71, 4 },
			{ 95, 2 },
			{ 104, 2 },
			{ 184, 5 },
			{ 211, 5 },
			{ 246, 4 },
			{ 275, 3 },
			{ 327, 3 },
			{ 338, 3 },
			{ 346, 2 },
			{ 357, 3 },
			{ 389, 6 },
			{ 443, 2 },
			{ 485, 4 },
			{ 515, 3 },
			{ 550, 3 },
			{ 591, 5 },
			{ 635, 3 },
			{ 667, 4 },
			{ 700, 3 },
		} },
		{ 507,{
			{ 5, 1 },
			{ 12, 3 },
			{ 38, 3 },
			{ 40, 1 },
			{ 42, 2 },
			{ 43, 2 },
			{ 44, 4 },
			{ 59, 3 },
			{ 72, 1 },
			{ 77, 1 },
			{ 98, 3 },
			{ 143, 1 },
			{ 175, 1 },
			{ 409, 5 },
			{ 410, 4 },
			{ 413, 5 },
			{ 414, 3 },
			{ 415, 5 },
			{ 416, 5 },
			{ 417, 6 },
			{ 418, 4 },
			{ 419, 4 },
			{ 420, 4 },
			{ 421, 4 },
			{ 561, 5 },
			{ 566, 4 },
			{ 568, 5 },
			{ 569, 4 },
			{ 570, 4 },
			{ 571, 4 },
			{ 572, 4 },
			{ 576, 4 },
			{ 578, 3 },
			{ 582, 3 },
		} },
		{ 39,{
			{ 28, 3 },
			{ 39, 4 },
			{ 98, 3 },
			{ 101, 1 },
			{ 136, 1 },
			{ 186, 3 },
			{ 243, 2 },
			{ 247, 3 },
			{ 287, 4 },
			{ 298, 1 },
			{ 363, 2 },
			{ 378, 2 },
			{ 487, 5 },
			{ 519, 4 },
			{ 553, 3 },
			{ 598, 3 },
			{ 613, 4 },
			{ 686, 4 },
			{ 702, 3 },
		} },
		{ 3,{
			{ 3, 2 },
			{ 44, 4 },
			{ 46, 1 },
			{ 78, 1 },
			{ 141, 2 },
			{ 151, 1 },
			{ 210, 1 },
			{ 223, 3 },
			{ 252, 3 },
			{ 254, 3 },
			{ 265, 3 },
			{ 274, 4 },
			{ 352, 5 },
			{ 377, 2 },
			{ 387, 3 },
			{ 427, 5 },
			{ 464, 4 },
			{ 510, 4 },
			{ 530, 4 },
			{ 585, 3 },
			{ 605, 3 },
			{ 646, 3 },
			{ 660, 6 },
			{ 693, 3 },
		} },
		{ 9,{
			{ 30, 3 },
			{ 51, 6 },
			{ 72, 2 },
			{ 84, 4 },
			{ 174, 5 },
			{ 181, 5 },
			{ 212, 5 },
			{ 227, 5 },
			{ 251, 4 },
			{ 259, 5 },
			{ 279, 6 },
			{ 344, 6 },
			{ 395, 6 },
			{ 424, 4 },
			{ 446, 6 },
			{ 451, 2 },
			{ 475, 6 },
			{ 525, 5 },
			{ 584, 4 },
			{ 640, 5 },
			{ 662, 6 },
			{ 691, 5 },
		} },
		{ 11,{
			{ 6, 2 },
			{ 8, 1 },
			{ 46, 1 },
			{ 72, 1 },
			{ 74, 1 },
			{ 147, 1 },
			{ 195, 3 },
			{ 229, 3 },
			{ 235, 2 },
			{ 294, 2 },
			{ 382, 3 },
			{ 438, 4 },
			{ 465, 1 },
			{ 493, 5 },
			{ 511, 5 },
			{ 545, 3 },
			{ 606, 3 },
			{ 628, 4 },
			{ 677, 3 },
		} } };
	CSteamID LocalSteamID = g_SteamUser->GetSteamID();

	int i = 20000;

	for (auto ItemConfig : Cloud::Get().skin_component->config[C_LocalPlayer::xuid_low].get_items())
	{
		auto ItemData = ItemConfig;
		if (!ItemData.enabled)continue;
		int ItemIndex = (ItemData.definition_override_index != 0) ? ItemData.definition_override_index : ItemData.definition_index;

		CSOEconItem Skin;

		Skin.set_id(20000 + ItemConfig.definition_index);
		Skin.set_account_id(LocalSteamID.GetAccountID());
		Skin.set_def_index(ItemIndex);
		Skin.set_inventory(i++);
		Skin.set_origin(24);
		Skin.set_quantity(1);
		Skin.set_level(1);
		Skin.set_style(0);
		Skin.set_flags(0);
		Skin.set_in_use(false);
		Skin.set_original_id(0);
		if (ItemData.custom_name)
			Skin.set_custom_name(ItemData.custom_name);

		if (ItemIndex >= WEAPON_BAYONET && ItemIndex <= WEAPON_KNIFE_WIDOWMAKER || (ItemIndex >= 5027 && ItemIndex <= 5034)) //|| ItemIndex == GLOVE_STUDDED_BLOODHOUND
		{
			Skin.set_rarity(6);
			Skin.set_quality(4);
		}
		else
		{
			int rarity = 0;// k_allskins[ItemData.iItemDefinitionIndex][ItemData.nFallbackPaintKit] == 0 ? 6 : k_allskins[ItemData.iItemDefinitionIndex][ItemData.nFallbackPaintKit];
			
			for (auto d : k_allskins[ItemIndex]) {
				rarity = d.Fallback == ItemData.paint_kit_index ? d.Rarity : rarity;
			}
			Skin.set_rarity(rarity);//(ItemData->GetPaintKit() != nullptr) ? ItemData->GetPaintKit()->rarity : 
			Skin.set_quality(ItemConfig.entity_quality_index);
		}

		if (teams[ItemConfig.definition_index] == -1) {
			CSOEconItemEquipped* EquippedState = Skin.add_equipped_state();

			EquippedState->set_new_class(3);
			EquippedState->set_new_slot(slots[ItemConfig.definition_index]);
			//}
			//if (ItemData->GetEquippedState(2) != 9999)
			//{
			//CSOEconItemEquipped* 
			EquippedState = Skin.add_equipped_state();

			EquippedState->set_new_class(2);
			EquippedState->set_new_slot(slots[ItemConfig.definition_index]);
		}
		else {
			CSOEconItemEquipped* EquippedState = Skin.add_equipped_state();

			EquippedState->set_new_class(teams[ItemConfig.definition_index]);
			EquippedState->set_new_slot(slots[ItemConfig.definition_index]);
		}
		CSOEconItemAttribute* PaintKitAttribute = Skin.add_attribute();

		float PaintKitAttributeValue = ItemData.paint_kit_index;

		PaintKitAttribute->set_def_index(6);

		PaintKitAttribute->set_value_bytes(&PaintKitAttributeValue, 4);

		// Paint Seed
		CSOEconItemAttribute* SeedAttribute = Skin.add_attribute();
		float SeedAttributeValue = (float)(ItemData.seed);

		SeedAttribute->set_def_index(7);

		SeedAttribute->set_value_bytes(&SeedAttributeValue, 4);

		// Paint Wear
		CSOEconItemAttribute* WearAttribute = Skin.add_attribute();
		float WearAttributeValue = ItemData.wear;

		WearAttribute->set_def_index(8);

		WearAttribute->set_value_bytes(&WearAttributeValue, 4);

		// StatTrak
		if (ItemData.stat_trak > -1)
		{
			// Counter Type
			CSOEconItemAttribute* StatTrakAttribute = Skin.add_attribute();
			uint32_t StatTrakAttributeValue = 0;

			StatTrakAttribute->set_def_index(81);

			StatTrakAttribute->set_value_bytes(&StatTrakAttributeValue, 4);

			// Counter Value
			CSOEconItemAttribute* StatTrakCountAttribute = Skin.add_attribute();
			uint32_t StatTrakCountAttributeValue = ItemData.stat_trak;

			StatTrakCountAttribute->set_def_index(80);

			StatTrakCountAttribute->set_value_bytes(&StatTrakCountAttributeValue, 4);

			// Quality
			Skin.set_quality(9);
		}

		// Stickers
		for (int j = 0; j < 5; j++)
		{
			sticker_setting StickerData = ItemData.stickers[j];


			// Sticker Kit
			CSOEconItemAttribute* StickerKitAttribute = Skin.add_attribute(); 
			uint32_t StickerKitAttributeValue = StickerData.kit;

			StickerKitAttribute->set_def_index(113 + 4 * j);

			StickerKitAttribute->set_value_bytes(&StickerKitAttributeValue, 4);

			// Sticker Wear
			CSOEconItemAttribute* StickerWearAttribute = Skin.add_attribute();
			float StickerWearAttributeValue = StickerData.wear;

			StickerWearAttribute->set_def_index(114 + 4 * j);

			StickerWearAttribute->set_value_bytes(&StickerWearAttributeValue, 4);

			// Sticker Scale
			CSOEconItemAttribute* StickerScaleAttribute = Skin.add_attribute();
			float StickerScaleAttributeValue = StickerData.scale;

			StickerScaleAttribute->set_def_index(115 + 4 * j);

			StickerScaleAttribute->set_value_bytes(&StickerScaleAttributeValue, 4);

			// Sticker Rotation
			CSOEconItemAttribute* StickerRotationAttribute = Skin.add_attribute();
			float StickerRotationAttributeValue = StickerData.rotation;

			StickerRotationAttribute->set_def_index(116 + 4 * j);

			StickerRotationAttribute->set_value_bytes(&StickerRotationAttributeValue, 4);
		}

		pInventoryCacheObject->add_object_data(Skin.SerializeAsString());

		//i++;
	}
}