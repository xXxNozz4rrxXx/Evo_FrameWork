#include "vfunc_hook.hpp"
#include "tlhelp32.h"
#include <psapi.h>
#include <vector>
#include "utils.hpp"

DWORD GetModuleSize(char* module) {
	HANDLE hSnap;
	std::string strModule = module;
	std::wstring shitModule = std::wstring(strModule.begin(), strModule.end());
	MODULEENTRY32 xModule;
	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	xModule.dwSize = sizeof(MODULEENTRY32);
	if (Module32First(hSnap, &xModule)) {
		while (Module32Next(hSnap, &xModule)) {
			if (!wcscmp(xModule.szModule, shitModule.c_str())) {
				CloseHandle(hSnap);
				return (DWORD)xModule.modBaseSize;
			}
		}
	}
	CloseHandle(hSnap);
	return 0;
}

__forceinline DWORD GetModuleHandleArraySize(HANDLE hProcess) {
	DWORD szNeeded;
	auto res = K32EnumProcessModules(hProcess, nullptr, 0, &szNeeded);
	return szNeeded;
}

uint32_t GetModuleByAddress(uint32_t addr) {
	//std::map<std::string, uint64_t> res;
	auto szNeeded = GetModuleHandleArraySize(GetCurrentProcess());
	uint32_t module_ret = 0;
	std::vector<uint32_t> modHandles;
	modHandles.resize(szNeeded);
	//K32EnumProcessModules(hProcess, (HMODULE*)modHandles.data(), szNeeded, &szNeeded);

	if (!K32EnumProcessModules(GetCurrentProcess(), (HMODULE*)modHandles.data(), szNeeded, &szNeeded))
		return 0x00000000;

	for (int i = 0; i != (szNeeded / sizeof(HMODULE)); i++) {
		char fn[1024];
		if (K32GetModuleBaseNameA(GetCurrentProcess(), (HMODULE)modHandles[i], fn, sizeof(fn))) {
			//res[fn] = modHandles[i];
			auto _sz = GetModuleSize(fn);
			if ((_sz + modHandles[i]) > addr && modHandles[i] <= addr)
				return modHandles[i];
		}
	}

	return 0x00000000;
}

vfunc_hook::vfunc_hook()
	: class_base(nullptr), vftbl_len(0), new_vftbl(nullptr), old_vftbl(nullptr), base_addr(0)
{
}
vfunc_hook::vfunc_hook(void* base)
	: class_base(base), vftbl_len(0), new_vftbl(nullptr), old_vftbl(nullptr), base_addr(0)
{
}
vfunc_hook::~vfunc_hook()
{
	unhook_all();

	delete[] new_vftbl;
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

inline std::uint8_t* ZeroScan(void* module, int32_t cnt) {
	std::string test;
	for (uint32_t i = 0; i < (cnt + 1) * sizeof(uintptr_t); i++)
		test += "00 ";
	return PatternScan(module, test.c_str());
}

uint32_t vfunc_hook::create_tramp_to_addr(uint32_t addr) {
	return (uint32_t)addr;
}

bool vfunc_hook::setup(void* base /*= nullptr*/, std::string module)
{
	if (base != nullptr)
		class_base = base;
	if (class_base == nullptr)
		return false;

	old_vftbl = *(std::uintptr_t**)class_base;
	vftbl_len = estimate_vftbl_length(old_vftbl);

	if (vftbl_len == 0)
		return false;

	//new_vftbl = new std::uintptr_t[vftbl_len + 1]();

	uint32_t mod_addr;

	if (module == "")
		mod_addr = GetModuleByAddress((uint32_t)class_base);
	else
		mod_addr = (uint32_t)GetModuleHandleA(module.c_str());

	//assert(mod_addr);

	auto ptr = ZeroScan((uint8_t*)mod_addr, vftbl_len);
	//assert(ptr);
	DWORD old;
	VirtualProtect(ptr, (vftbl_len + 1) * sizeof(std::uintptr_t), 0x40, &old);



	*(void**)ptr = (uint8_t*)new uintptr_t[vftbl_len + 1]; // meme

	new_vftbl = (uintptr_t*)ptr;
	base_addr = mod_addr;

	std::memset(new_vftbl, NULL, (vftbl_len + 1) * sizeof(std::uintptr_t));
	std::memcpy(&new_vftbl[1], old_vftbl, vftbl_len * sizeof(std::uintptr_t));

	try {
		auto guard = detail::protect_guard{ class_base, sizeof(std::uintptr_t), PAGE_READWRITE };
		new_vftbl[0] = old_vftbl[-1];
		*(std::uintptr_t**)class_base = &new_vftbl[1];
	}
	catch (...) {
		delete[] new_vftbl;
		return false;
	}

	return true;
}
std::size_t vfunc_hook::estimate_vftbl_length(std::uintptr_t* vftbl_start)
{
	auto len = std::size_t{};

	while (vftbl_start[len] >= 0x00010000 &&
		vftbl_start[len] < 0xFFF00000 &&
		len < 512 /* Hard coded value. Can cause problems, beware.*/) {
		len++;
	}

	return len;
}
