#pragma once                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         #pragma once
#include <Windows.h>
#include <memory>
#include <iostream>
#include <map>
#include <winnt.h>
//#include <detours.h> 

//#pragma comment(lib,"detours.lib")

//using namespace std;

#define EXCEPTIONFLAG_SINGLESTEP 0x100
//#define DETOUR


namespace detail
{
	class protect_guard
	{
	public:
		protect_guard(void* base, size_t len, std::uint32_t flags)
		{
			_base = base;
			_length = len;
			if (!VirtualProtect(base, len, flags, (PDWORD)&_old))
				throw std::runtime_error("Failed to protect region.");
		}
		~protect_guard()
		{
			VirtualProtect(_base, _length, _old, (PDWORD)&_old);
		}

	private:
		void*         _base;
		size_t        _length;
		std::uint32_t _old;
	};
}

class vfunc_hook
{
public:
	vfunc_hook();
	vfunc_hook(void* base);
	~vfunc_hook();

	bool setup(void* class_base = nullptr, std::string module = "");

	uint32_t create_tramp_to_addr(uint32_t addr);

	template<typename T>
	void hook_index(int index, T fun)
	{
		//assert(index >= 0 && index <= (int)vftbl_len);
		auto hk = reinterpret_cast<std::uintptr_t>(fun);
		//assert(tr);
		new_vftbl[index + 1] = hk;
	}
	void unhook_index(int index)
	{
		new_vftbl[index] = old_vftbl[index];
	}
	void unhook_all()
	{
		try {
			if (old_vftbl != nullptr) {
				auto guard = detail::protect_guard{ class_base, sizeof(std::uintptr_t), PAGE_READWRITE };
				*(std::uintptr_t**)class_base = old_vftbl;
				old_vftbl = nullptr;
			}
		}
		catch (...) {
		}
	}

	template<typename T>
	T GetOriginalFunc(int index)
	{
		return (T)old_vftbl[index];
	}

private:
	static inline std::size_t estimate_vftbl_length(std::uintptr_t* vftbl_start);

	void*           class_base;
	std::size_t     vftbl_len;
	std::uintptr_t* new_vftbl;
	std::uintptr_t* old_vftbl;
	std::uint32_t base_addr; // module base address
};

class table_hook
{
public:
	constexpr table_hook() :
		m_new_vmt{ nullptr },
		m_old_vmt{ nullptr } {}

	~table_hook()
	{
		if (m_new_vmt)
			delete[](m_new_vmt - 1);
	}

protected:
	auto is_code_ptr(void* ptr) -> bool
	{
		constexpr const DWORD protect_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

		MEMORY_BASIC_INFORMATION out;
		VirtualQuery(ptr, &out, sizeof out);

		return out.Type
			&& !(out.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			&& out.Protect & protect_flags;
	}
	auto initialize(void** original_table) -> void
	{
		m_old_vmt = original_table;

		size_t size = 0;
		while (m_old_vmt[size] && is_code_ptr(m_old_vmt[size]))
			++size;

		m_new_vmt = (new void*[size + 1]) + 1;
		//std::copy(m_old_vmt - 1, m_old_vmt + size, m_new_vmt - 1);
		memcpy(m_new_vmt - 1, m_old_vmt - 1, sizeof(void*) * (size + 1));
	}

	void leak_table()
	{
		m_new_vmt = nullptr;
	}

	auto hook_instance(void* inst) const -> void
	{
		auto& vtbl = *reinterpret_cast<void***>(inst);
		vtbl = m_new_vmt;
	}

	auto unhook_instance(void* inst) const -> void
	{
		auto& vtbl = *reinterpret_cast<void***>(inst);
		vtbl = m_old_vmt;
	}

	auto initialize_and_hook_instance(void* inst) -> bool
	{
		auto& vtbl = *reinterpret_cast<void***>(inst);
		auto initialized = false;
		if (!m_old_vmt)
		{
			initialized = true;
			initialize(vtbl);
		}
		hook_instance(inst);
		return initialized;
	}

	template <typename Fn>
	auto hook_function(Fn hooked_fn, const std::size_t index) -> Fn
	{
		m_new_vmt[index] = (void*)(hooked_fn);
		return (Fn)(m_old_vmt[index]);
	}

	template<typename T>
	auto apply_hook(std::size_t idx) -> void
	{
		T::m_original = hook_function(&T::hooked, idx);
	}

	template <typename Fn = uintptr_t>
	auto get_original_function(const int index) -> Fn
	{
		return (Fn)(m_old_vmt[index]);
	}

private:
	void** m_new_vmt = nullptr;
	void** m_old_vmt = nullptr;
};

class vmt_multi_hook : table_hook
{
public:
	constexpr vmt_multi_hook() {}

	~vmt_multi_hook()
	{
		leak_table();
	}

	using table_hook::apply_hook;
	using table_hook::get_original_function;
	using table_hook::hook_function;
	using table_hook::hook_instance;
	using table_hook::unhook_instance;
	using table_hook::initialize;
	using table_hook::initialize_and_hook_instance;
};