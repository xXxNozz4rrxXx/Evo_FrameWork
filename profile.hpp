#pragma once

#include "../singleton.hpp"

#include "../valve_sdk/csgostructs.hpp"
#include "../valve_sdk/google/protobuf/base_gcmessages.pb.h"
#include "../valve_sdk/google/protobuf/cstrike15_gcmessages.pb.h"
#include "../valve_sdk/google/protobuf/gcsdk_gcmessages.pb.h"
#include "../valve_sdk/google/protobuf/gcsystemmsgs.pb.h"
#include "../valve_sdk/google/protobuf/econ_gcmessages.pb.h"

class CMsgSOCacheSubscribed;
class CMatchEventsSubscription;
class CMatchSessionOnlineHost;
class PlayerLocal
{
public:
	int GetXUIDLow()
	{
		//MUT("GetXUIDLowPlayerLocal");
	//	END();
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x8);
	}

	int GetXUIDHigh()
	{
		//MUT("GetXUIDHighPlayerLocal");
		//END();
		return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0xC);
	}

	const char* GetName()
	{
		//MUT("GetNamePlayerLocal");
		typedef const char*(__thiscall* tPaster)(void*);
		//END();
		return CallVFunction<tPaster>(this, 2)(this);
	}
};
class PlayerManager
{
public:
	PlayerLocal* GetLocalPlayer(int un)
	{
		//MUT("GetLocalPlayerPlayerManager");
		typedef PlayerLocal*(__thiscall* tTo)(void*, int);
		//END();
		return CallVFunction<tTo>(this, 1)(this, un);
	}
};
class CMatchSystem
{
public:
	PlayerManager* GetPlayerManager()
	{
		//MUT("GetPlayerManagerMatchSystem");
		typedef PlayerManager*(__thiscall* tKapis)(void*);
		//END();
		return CallVFunction<tKapis>(this, 0)(this);
	}
};
class CMatchFramework
{
public:
	CMatchEventsSubscription* GetEventsSubscription()
	{
		typedef CMatchEventsSubscription*(__thiscall* tGetEventsSubscription)(void*);
		//END();
		return CallVFunction<tGetEventsSubscription>(this, 11)(this);
	}

	CMatchSessionOnlineHost* GetMatchSession()
	{
		//MUT("GetMatchSessionMatchFramework");
		typedef CMatchSessionOnlineHost*(__thiscall* tGetMatchSession)(void*);
		//END();
		return CallVFunction<tGetMatchSession>(this, 13)(this);
	}

	CMatchSystem* GetMatchSystem()
	{
		//MUT("GetMatchSystemMatchFramework");
		typedef CMatchSystem*(__thiscall* tGetMatchSystem)(void*);
		//END();
		return CallVFunction<tGetMatchSystem>(this, 15)(this);
	}

};
class CMatchSessionOnlineHost
{
public:
	KeyValues* GetSessionSettings()
	{
		//MUT("GetSessionSettingsCMatchSessionOnlineHost");
		typedef KeyValues*(__thiscall* tGetSessionSettings)(void*);
		//END();
		return CallVFunction<tGetSessionSettings>(this, 1)(this);
	}
	void UpdateSessionSettings(KeyValues* packet)
	{
		//MUT("UpdateSessionSettingsCMatchSessionOnlineHost");
		typedef void(__thiscall* tUpdateSessionSettings)(void*, KeyValues*);
		CallVFunction<tUpdateSessionSettings>(this, 2)(this, packet);
		//END();
	}
	void Command(KeyValues* packet)
	{
		//MUT("CommandCMatchSessionOnlineHost");
		typedef void(__thiscall* tUpdateSessionSettings)(void*, KeyValues*);
		CallVFunction<tUpdateSessionSettings>(this, 3)(this, packet);
		//END();
	}
};

class Profile : public Singleton<Profile>
{
	friend class Singleton<Profile>;
	Profile();
	~Profile();
	CMatchFramework*	MatchFramework = nullptr;
	void ApplyMedals(CMsgSOCacheSubscribed::SubscribedType* pInventoryCacheObject);
	void ApplySkins(CMsgSOCacheSubscribed::SubscribedType* pInventoryCacheObject);
public:
	std::vector<int> medals;
	int wearMedal = -1;
	bool Update();
	void OnRecieveMessage(uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32_t *pcubMsgSize);
	bool OnSendMessage(uint32_t unMsgType, const void* pubData, uint32_t cubData);
};
