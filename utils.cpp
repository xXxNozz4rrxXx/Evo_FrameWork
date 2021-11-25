#include "Utils.hpp"

#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "../valve_sdk/csgostructs.hpp"
#include "Math.hpp"

using std::string;
using std::wstring;


HANDLE _out = NULL, _old_out = NULL;
HANDLE _err = NULL, _old_err = NULL;
HANDLE _in = NULL, _old_in = NULL;

namespace Utils {
	unsigned int FindInDataMap(datamap_t *pMap, const char *name) {
		while (pMap) {
			for (int i = 0; i<pMap->dataNumFields; i++) {
				if (pMap->dataDesc[i].fieldName == NULL)
					continue;

				if (strcmp(name, pMap->dataDesc[i].fieldName) == 0)
					return pMap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL];

				if (pMap->dataDesc[i].fieldType == FIELD_EMBEDDED) {
					if (pMap->dataDesc[i].td) {
						unsigned int offset;

						if ((offset = FindInDataMap(pMap->dataDesc[i].td, name)) != 0)
							return offset;
					}
				}
			}
			pMap = pMap->baseMap;
		}

		return 0;
	}
    /*
     * @brief Create console
     *
     * Create and attach a console window to the current process
     */
    void AttachConsole()
    {
        _old_out = GetStdHandle(STD_OUTPUT_HANDLE);
        _old_err = GetStdHandle(STD_ERROR_HANDLE);
        _old_in  = GetStdHandle(STD_INPUT_HANDLE);

        ::AllocConsole() && ::AttachConsole(GetCurrentProcessId());

        _out     = GetStdHandle(STD_OUTPUT_HANDLE);
        _err     = GetStdHandle(STD_ERROR_HANDLE);
        _in      = GetStdHandle(STD_INPUT_HANDLE);

        SetConsoleMode(_out,
            ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

        SetConsoleMode(_in,
            ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS |
            ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
    }

    /*
     * @brief Detach console
     *
     * Detach and destroy the attached console
     */
    void DetachConsole()
    {
        if(_out && _err && _in) {
            FreeConsole();

            if(_old_out)
                SetStdHandle(STD_OUTPUT_HANDLE, _old_out);
            if(_old_err)
                SetStdHandle(STD_ERROR_HANDLE, _old_err);
            if(_old_in)
                SetStdHandle(STD_INPUT_HANDLE, _old_in);
        }
    }

    /*
     * @brief Print to console
     *
     * Replacement to printf that works with the newly created console
     */
    bool ConsolePrint(const char* fmt, ...)
    {
        if(!_out) 
            return false;

        char buf[1024];
        va_list va;

        va_start(va, fmt);
        _vsnprintf_s(buf, 1024, fmt, va);
        va_end(va);

        return !!WriteConsoleA(_out, buf, static_cast<DWORD>(strlen(buf)), nullptr, nullptr);
    }

    /*
     * @brief Blocks execution until a key is pressed on the console window
     *
     */
    char ConsoleReadKey()
    {
        if(!_in)
            return false;

        auto key = char{ 0 };
        auto keysread = DWORD{ 0 };

        ReadConsoleA(_in, &key, 1, &keysread, nullptr);

        return key;
    }


    /*
     * @brief Wait for all the given modules to be loaded
     *
     * @param timeout How long to wait
     * @param modules List of modules to wait for
     *
     * @returns See WaitForSingleObject return values.
     */
    int WaitForModules(std::int32_t timeout, const std::initializer_list<std::wstring>& modules)
    {
        bool signaled[32] = { 0 };
        bool success = false;

        std::uint32_t totalSlept = 0;

        if(timeout == 0) {
            for(auto& mod : modules) {
                if(GetModuleHandleW(std::data(mod)) == NULL)
                    return WAIT_TIMEOUT;
            }
            return WAIT_OBJECT_0;
        }

        if(timeout < 0)
            timeout = INT32_MAX;

        while(true) {
            for(auto i = 0u; i < modules.size(); ++i) {
                auto& module = *(modules.begin() + i);
                if(!signaled[i] && GetModuleHandleW(std::data(module)) != NULL) {
                    signaled[i] = true;

                    //
                    // Checks if all modules are signaled
                    //
                    bool done = true;
                    for(auto j = 0u; j < modules.size(); ++j) {
                        if(!signaled[j]) {
                            done = false;
                            break;
                        }
                    }
                    if(done) {
                        success = true;
                        goto exit;
                    }
                }
            }
            if(totalSlept > std::uint32_t(timeout)) {
                break;
            }
            Sleep(10);
            totalSlept += 10;
        }

    exit:
        return success ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
    }

    /*
     * @brief Scan for a given byte pattern on a module
     *
     * @param module    Base of the module to search
     * @param signature IDA-style byte array pattern
     *
     * @returns Address of the first occurence
     */

	void utf8toWStr(wstring& dest, const string& src) {
		dest.clear();
		wchar_t w = 0;
		int bytes = 0;
		wchar_t err = L'?';
		for (size_t i = 0; i < src.size(); i++) {
			unsigned char c = (unsigned char)src[i];
			if (c <= 0x7f) {//first byte 
				if (bytes) {
					dest.push_back(err);
					bytes = 0;
				}
				dest.push_back((wchar_t)c);
			}
			else if (c <= 0xbf) {//second/third/etc byte 
				if (bytes) {
					w = ((w << 6) | (c & 0x3f));
					bytes--;
					if (bytes == 0)
						dest.push_back(w);
				}
				else
					dest.push_back(err);
			}
			else if (c <= 0xdf) {//2byte sequence start 
				bytes = 1;
				w = c & 0x1f;
			}
			else if (c <= 0xef) {//3byte sequence start 
				bytes = 2;
				w = c & 0x0f;
			}
			else if (c <= 0xf7) {//3byte sequence start 
				bytes = 3;
				w = c & 0x07;
			}
			else {
				dest.push_back(err);
				bytes = 0;
			}
		}
		if (bytes)
			dest.push_back(err);
	}

	wstring utf8toWStr(const string& str) {
		wstring result;
		utf8toWStr(result, str);
		return result;
	}


    std::uint8_t* PatternScan(void* module, const char* signature)
    {
		static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);

			for (auto current = start; current < end; ++current) {
				if (*current == '?') {
					++current;
					if (*current == '?')
						++current;
					bytes.push_back(-1);
				}
				else {
					bytes.push_back(strtoul(current, &current, 16));
				}
			}
			return bytes;
		};

		auto dosHeader = (PIMAGE_DOS_HEADER)module;
		auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

		auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		auto patternBytes = pattern_to_byte(signature);
		auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

		auto s = patternBytes.size();
		auto d = patternBytes.data();

		for (auto i = 0ul; i < sizeOfImage - s; ++i) {
			bool found = true;
			for (auto j = 0ul; j < s; ++j) {
				if (scanBytes[i + j] != d[j] && d[j] != -1) {
					found = false;
					break;
				}
			}
			if (found) {
				return &scanBytes[i];
			}
		}
		return nullptr;
	}

	std::uint8_t * ZeroScan(void * module, int32_t cnt)
	{
		std::string test;
		for (uint32_t i = 0; i < (cnt + 1) * sizeof(uintptr_t); i++)
			test += "00 ";
		return PatternScan(module, test.c_str());
	}
    

    /*
     * @brief Set player clantag
     *
     * @param tag New clantag
     */
    void SetClantag(const char* tag)
    {
        static auto fnClantagChanged = (int(__fastcall*)(const char*, const char*))PatternScan(GetModuleHandleW(L"engine.dll"), "53 56 57 8B DA 8B F9 FF 15");

        fnClantagChanged(tag, tag);
    }

    /*
     * @brief Set player name
     *
     * @param name New name
     */
    void SetName(const char* name)
    {
        static auto nameConvar = g_CVar->FindVar("name");
        nameConvar->m_fnChangeCallbacks.m_Size = 0;

        // Fix so we can change names how many times we want
        // This code will only run once because of `static`
        static auto do_once = (nameConvar->SetValue("\nญญญ"), true);

        nameConvar->SetValue(name);
    }

    /*
     * @brief Reveal the ranks of all players on the server
     *
     */
	void RankRevealAll()
	{
		using ServerRankRevealAll = char(__cdecl*)(int*);

		static uint8_t* fnServerRankRevealAll = PatternScan(GetModuleHandleA("client_panorama.dll"), "55 8B EC 8B 0D ? ? ? ? 85 C9 75 ? A1 ? ? ? ? 68 ? ? ? ? 8B 08 8B 01 FF 50 ? 85 C0 74 ? 8B C8 E8 ? ? ? ? 8B C8 EB ? 33 C9 89 0D ? ? ? ? 8B 45 ? FF 70 ? E8 ? ? ? ? B0 ? 5D");

		if (fnServerRankRevealAll) {
			int v[3] = { 0,0,0 };

			reinterpret_cast<ServerRankRevealAll>(fnServerRankRevealAll)(v);
		}
	}
	bool LineGoesThroughSmoke(Vector vStartPos, Vector vEndPos)
	{

		typedef bool(__cdecl* _LineGoesThroughSmoke) (Vector, Vector);
		static _LineGoesThroughSmoke LineGoesThroughSmokeFn = 0;

		if (!LineGoesThroughSmokeFn)
			LineGoesThroughSmokeFn = (_LineGoesThroughSmoke)Utils::PatternScan(GetModuleHandleW(L"client_panorama.dll"), ("55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0"));

		if (LineGoesThroughSmokeFn)
			return LineGoesThroughSmokeFn(vStartPos, vEndPos);
		return false;
	}

	
	bool IsWeaponPistol(int weaponid)
	{
		if (weaponid == WEAPON_DEAGLE || weaponid == WEAPON_ELITE || weaponid == WEAPON_FIVESEVEN || weaponid == WEAPON_GLOCK || weaponid == WEAPON_HKP2000 || weaponid == WEAPON_P250 || weaponid == WEAPON_TEC9 || weaponid == WEAPON_USP_SILENCER)
			return true;

		return false;
	}
	bool IsWeaponKnife(int weaponid)
	{
		switch (weaponid)
		{
		case WEAPON_KNIFE:
		case WEAPON_KNIFE_T:
		case WEAPON_BAYONET:
		case WEAPON_KNIFE_BUTTERFLY:
		case WEAPON_KNIFE_FALCHION:
		case WEAPON_KNIFE_FLIP:
		case WEAPON_KNIFE_GUT:
		case WEAPON_KNIFE_KARAMBIT:
		case WEAPON_KNIFE_M9_BAYONET:
		case WEAPON_KNIFE_PUSH:
		case WEAPON_KNIFE_SURVIVAL_BOWIE:
		case WEAPON_KNIFE_TACTICAL:
			return true; break;
		default:return false; break;
		}
	}

	bool IsWeaponDefaultKnife(int weaponid)
	{
		if (weaponid == WEAPON_KNIFE || weaponid == WEAPON_KNIFE_T)
			return true;

		return false;
	}

	bool IsWeaponGrenade(int weaponid)
	{
		if (weaponid == WEAPON_FLASHBANG || weaponid == WEAPON_HEGRENADE || weaponid == WEAPON_SMOKEGRENADE || weaponid == WEAPON_MOLOTOV || weaponid == WEAPON_INCGRENADE || weaponid == WEAPON_DECOY)
			return true;

		return false;
	}

	bool IsWeaponBomb(int weaponid)
	{
		if (weaponid == WEAPON_C4)
			return true;

		return false;
	}

	bool IsWeaponTaser(int weaponid)
	{
		if (weaponid == WEAPON_TASER)
			return true;

		return false;
	}
	bool IsNonAimWeapon(int weaponid)
	{
		if (IsWeaponKnife(weaponid) || IsWeaponGrenade(weaponid) || IsWeaponBomb(weaponid) || IsWeaponTaser(weaponid))
			return true;

		return false;
	}
	bool IsVisibleVector(C_BasePlayer *pEnt, C_BasePlayer *pLocal, Vector bone)
	{

		if (!pEnt || !pLocal)
			return false;

		Ray_t ray;
		trace_t tr;
		ray.Init(pLocal->GetEyePos(), bone);
		CTraceFilter traceFilter;
		traceFilter.pSkip = pLocal;
		g_EngineTrace->TraceRay(ray, MASK_SHOT, &traceFilter, &tr);

		bool visible = (tr.hit_entity == pEnt || tr.fraction >= 0.96f);
		return visible;
	}
	Vector CalculateAngle(const Vector& in, Vector out)
	{
		double delta[3] = { (in[0] - out[0]), (in[1] - out[1]), (in[2] - out[2]) };
		double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
		Vector ret = Vector();
		ret.x = (float)(asinf(delta[2] / hyp) * 57.295779513082f);
		ret.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
		ret.z = 0.0f;

		if (delta[0] >= 0.0)
			ret.y += 180.0f;
		return ret;
		/*float pitch = U::Deg2Rad(in.x);
		float yaw = U::Deg2Rad(in.y);
		float temp = cos(pitch);
		Vector3 ret = Vector3();

		ret.x = -temp * -cos(yaw);
		ret.y = sin(yaw) * temp;
		ret.z = -sin(pitch);
		return ret;*/
	}

	void AngleNormalize(QAngle& v)
	{
		for (auto i = 0; i < 3; i++)
		{
			if (v[i] < -180.0f) v[i] += 360.0f;
			if (v[i] > 180.0f) v[i] -= 360.0f;
		}
	}

	void ClampAngles(QAngle& v)
	{
	if (v.pitch > 89.0f)
       v.pitch = 89.0f;
    if (v.pitch < -89.0f)
        v.pitch = -89.0f;
 
    if (v.yaw > 180.0f)
        v.yaw = 180.0f;
    if (v.yaw < -180.0f)
        v.yaw = -180.0f;
 
    v.roll = 0.0f;
	}

	void SinCos(float x, float* s, float* c)
	{
		__asm
		{
			fld dword ptr[x]
			fsincos
			mov edx, dword ptr[c]
			mov eax, dword ptr[s]
			fstp dword ptr[edx]
			fstp dword ptr[eax]
		}
	}

	void SinCos(float x, float &s, float &c)
	{
		__asm
		{
			fld dword ptr[x]
			fsincos
			mov edx, dword ptr[c]
			mov eax, dword ptr[s]
			fstp dword ptr[edx]
			fstp dword ptr[eax]
		}
	}
#define M_PI           3.14159265358979323846f
	float Rad2Deg(float x)
	{
		return (x * (180.0f / M_PI));
	}

	float Deg2Rad(float x)
	{
		return (x * (M_PI / 180.0f));
	}
	typedef float(*RandomFloat_t)(float, float);
	float vstRandomFloat(float min, float max) {
		static RandomFloat_t m_RandomFloat;
		if (!m_RandomFloat)
			m_RandomFloat = (RandomFloat_t)GetProcAddress(GetModuleHandle(L"vstdlib.dll"), "RandomFloat");

		return m_RandomFloat(min, max);
	}

	typedef void(*RandomSeed_t)(UINT);
	void vstRandomSeed(UINT seed) {
		static RandomSeed_t m_RandomSeed;
		if (!m_RandomSeed)
			m_RandomSeed = (RandomSeed_t)GetProcAddress(GetModuleHandle(L"vstdlib.dll"), "RandomSeed");

		m_RandomSeed(seed);
	}

	void AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up)
	{
		float sr, sp, sy, cr, cp, cy;

		SinCos(Deg2Rad(angles.x), &sp, &cp);
		SinCos(Deg2Rad(angles.y), &sy, &cy);
		SinCos(Deg2Rad(angles.z), &sr, &cr);

		if (forward)
		{
			forward->x = cp * cy;
			forward->y = cp * sy;
			forward->z = -sp;
		}

		if (right)
		{
			right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
			right->y = (-1 * sr * sp * sy + -1 * cr * cy);
			right->z = -1 * sr * cp;
		}

		if (up)
		{
			up->x = (cr * sp * cy + -sr * -sy);
			up->y = (cr * sp * sy + -sr * cy);
			up->z = cr * cp;
		}
	}

	void VectorAngles2(const Vector &vecForward, Vector &vecAngles)
	{
		Vector vecView;
		if (vecForward[1] == 0.f && vecForward[0] == 0.f)
		{
			vecView[0] = 0.f;
			vecView[1] = 0.f;
		}
		else
		{
			vecView[1] = atan2(vecForward[1], vecForward[0]) * 180.f / M_PI;

			if (vecView[1] < 0.f)
				vecView[1] += 360.f;

			vecView[2] = sqrt(vecForward[0] * vecForward[0] + vecForward[1] * vecForward[1]);

			vecView[0] = atan2(vecForward[2], vecView[2]) * 180.f / M_PI;
		}

		vecAngles[0] = -vecView[0];
		vecAngles[1] = vecView[1];
		vecAngles[2] = 0.f;
	}
#define M_PI		3.14159265358979323846
#define M_PI_F		((float)(M_PI))
	bool HitChance_Run(C_BasePlayer* pLocalPlayer, QAngle angAngle, float flChance)
	{
		const float TRACE_COUNT = 40;

		auto pLocalWeapon = pLocalPlayer->m_hActiveWeapon();
		if (!pLocalWeapon)
			return false;
		auto pWeaponData = pLocalWeapon->GetCSWeaponData();
		if (!pWeaponData)
			return false;

		Vector forward, right, up;
		Vector neu = pLocalPlayer->GetEyePos();
		AngleVectors(Vector(angAngle.pitch, angAngle.yaw, angAngle.roll), &forward, &right, &up);

		int cHits = 0;
		int cNeededHits = static_cast<int>(TRACE_COUNT * (flChance / 100.f));
		pLocalWeapon->UpdateAccuracyPenalty();
		for (int i = 0; i < TRACE_COUNT; i++)
		{
			vstRandomSeed((i & 0xFFF) + 1);

			float flSpread = pLocalWeapon->GetSpread();
			float flPenalty = pLocalWeapon->GetInaccuracy();

			float a = vstRandomFloat(0.f, M_PI_F * 2.f);
			float b = vstRandomFloat(0.f, flSpread);
			float c = vstRandomFloat(0.f, M_PI_F * 2.f);
			float d = vstRandomFloat(0.f, flPenalty);

			Vector viewSpread;
			viewSpread[0] = (cos(a) * b) + (cos(c) * d);
			viewSpread[1] = (sin(a) * b) + (sin(c) * d);

			Vector viewSpreadForward = (forward + viewSpread.x * right + viewSpread.y * up);

			Vector viewAngles;
			VectorAngles2(viewSpreadForward, viewAngles);

			Vector rem;
			AngleVectors(viewAngles, &rem, NULL, NULL);
			rem = neu + (rem * pWeaponData->flRange);

			trace_t tr;
			Ray_t ray;

			CTraceFilter filter;
			filter.pSkip = pLocalPlayer;


			ray.Init(neu, rem); // (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
			g_EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

			if (tr.hit_entity && tr.hit_entity->GetClientClass()->m_ClassID == CCSPlayer)
				cHits++;

			if (static_cast<int>((static_cast<float>(cHits) / TRACE_COUNT) * 100.f) >= flChance)
				return true;
		}

		return false;
	}

	template<class T>
	static T* FindHudElement(const char* name)
	{
		static auto pThis = *reinterpret_cast<DWORD**>(PatternScan(GetModuleHandleW(L"client_panorama.dll"), "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08") + 1);

		static auto find_hud_element = reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
		return (T*)find_hud_element(pThis, name);
	}

	struct hud_weapons_t {
		std::int32_t* get_weapon_count() {
			return reinterpret_cast<std::int32_t*>(std::uintptr_t(this) + 0x80);
		}
	};

	void ForceFullUpdate()
	{
		if (g_LocalPlayer && g_EngineClient->IsInGame()) {
			static auto clear_fn = reinterpret_cast<int(__thiscall*)(void*, int)>(PatternScan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 2C 89 5D FC"));
			auto dwHudWeaponSelection = FindHudElement<uintptr_t*>("CCSGO_HudWeaponSelection");
			if (dwHudWeaponSelection && clear_fn) {
				auto pHudWeapons = (int*)(uintptr_t(dwHudWeaponSelection) - 0x20);
				if (pHudWeapons && (void*)(uintptr_t(dwHudWeaponSelection) - 0xA0)) {
					for (auto i = 0; i < *pHudWeapons; i++) i = clear_fn((void*)(uintptr_t(dwHudWeaponSelection) - 0xA0), i);
					*pHudWeapons = 0;
				}
			}
			typedef void(*ForceUpdate) (void);
			static ForceUpdate FullUpdate = (ForceUpdate)PatternScan(GetModuleHandleW(L"engine.dll"), "A1 ? ? ? ? B9 ? ? ? ? 56 FF 50 14 8B 34 85");
			if (FullUpdate)
				FullUpdate();
			else
				FullUpdate = (ForceUpdate)PatternScan(GetModuleHandleW(L"engine.dll"), "A1 ? ? ? ? B9 ? ? ? ? 56 FF 50 14 8B 34 85");
		}
	}
	Color Float3ToClr(float col[3])
	{
		if (col[0] <=1 && col[1] <=1 && col[2] <= 1)
			return Color(col[0] * 255, col[1] * 255, col[2] * 255);
		else
		return Color(col[0], col[1], col[2] );
	}
}