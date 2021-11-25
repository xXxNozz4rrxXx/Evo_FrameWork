#pragma once
#include "../valve_sdk/sdk.hpp"

class recv_prop_hook
{
public:
	recv_prop_hook(RecvProp* prop, const RecvVarProxyFn proxy_fn) :
		m_property(prop),
		m_original_proxy_fn(prop->m_ProxyFn)
	{
		set_proxy_function(proxy_fn);
	}

	~recv_prop_hook()
	{
		m_property->m_ProxyFn = m_original_proxy_fn;
	}

	auto get_original_function() const -> RecvVarProxyFn
	{
		return m_original_proxy_fn;
	}

	auto set_proxy_function(const RecvVarProxyFn proxy_fn) const -> void
	{
		m_property->m_ProxyFn = proxy_fn;
	}

private:
	RecvProp* m_property;
	RecvVarProxyFn m_original_proxy_fn;
};
