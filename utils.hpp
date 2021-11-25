#pragma once

#define NOMINMAX
#include <Windows.h>
#include <string>
#include <initializer_list>
#include "../valve_sdk/sdk.hpp"
#include "../datamap.hpp"

namespace Utils {
	unsigned int FindInDataMap(datamap_t * pMap, const char * name);
    /*
     * @brief Create console
     *
     * Create and attach a console window to the current process
     */
	void AttachConsole();

    /*
     * @brief Detach console
     *
     * Detach and destroy the attached console
     */
    void DetachConsole();

    /*
     * @brief Print to console
     *
     * Replacement to printf that works with the newly created console
     */
    bool ConsolePrint(const char* fmt, ...);
    
    /*
     * @brief Blocks execution until a key is pressed on the console window
     *
     */
    char ConsoleReadKey();

    /*
     * @brief Wait for all the given modules to be loaded
     *
     * @param timeout How long to wait
     * @param modules List of modules to wait for
     *
     * @returns See WaitForSingleObject return values.
     */
    int WaitForModules(std::int32_t timeout, const std::initializer_list<std::wstring>& modules);

    /*
     * @brief Scan for a given byte pattern on a module
     *
     * @param module    Base of the module to search
     * @param signature IDA-style byte array pattern
     *
     * @returns Address of the first occurence
     */
    std::uint8_t* PatternScan(void* module, const char* signature);
	std::uint8_t* ZeroScan(void* module, int32_t cnt);
    /*
     * @brief Set player clantag
     *
     * @param tag New clantag
     */
    void SetClantag(const char* tag);
	void utf8toWStr(std::wstring& dest, const std::string& src);
	std::wstring utf8toWStr(const std::string& str);
    /*
     * @brief Set player name
     *
     * @param name New name
     */
    void SetName(const char* name);

    /*
     * @brief Reveal the ranks of all players on the server
     *
     */
    void RankRevealAll();
	bool LineGoesThroughSmoke(Vector vStartPos, Vector vEndPos);
	bool IsWeaponPistol(int weaponid);
	bool IsWeaponKnife(int weaponid);
	bool IsWeaponDefaultKnife(int weaponid);
	bool IsWeaponGrenade(int weaponid);
	bool IsWeaponBomb(int weaponid);
	bool IsWeaponTaser(int weaponid);
	bool IsNonAimWeapon(int weaponid);
	bool IsVisibleVector(C_BasePlayer *pEnt, C_BasePlayer *pLocal, Vector bone);
	void AngleNormalize(QAngle& v);
	Vector CalculateAngle(const Vector& in, Vector out);
	void ClampAngles(QAngle& v);
	bool HitChance_Run(C_BasePlayer* pLocalPlayer, QAngle angAngle, float flChance);
	void ForceFullUpdate();
	Color Float3ToClr(float col[3]);
	std::wstring utf8toWStr(const std::string& str);
	void utf8toWStr(std::wstring& dest, const std::string& src);
}
