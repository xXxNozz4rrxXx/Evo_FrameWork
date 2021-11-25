#pragma once
#include "Config.h"
#include "../options.hpp"
#include "../helpers/math.hpp"
#include "../helpers/utils.hpp"
#include "../menu.hpp"
#include "../helpers/input.hpp"

template< typename T >
class ConfigItem
{
	std::string category, name;
	T* value;
public:
	ConfigItem( std::string category, std::string name, T* value )
	{
		this->category = category;
		this->name = name;
		this->value = value;
	}
};

template< typename T >
class ConfigValue
{
public:
	ConfigValue( std::string category_, std::string name_, T* value_ )
	{
		category = category_;
		name = name_;
		value = value_;
	}

	std::string category, name;
	T* value;
};

class CConfig
{
protected:
	std::vector< ConfigValue< int >* > ints;
	std::vector< ConfigValue< char >* > chars;
	std::vector< ConfigValue< bool >* > bools;
	std::vector< ConfigValue< float >* > floats;
private:
	void SetupValue( int&, int, std::string, std::string );
	void SetupValue(char * value, char * def, std::string category, std::string name);
	void SetupValueColor(const int, int, std::string, std::string);
	void SetupValue( bool&, bool, std::string, std::string );
	void SetupValue( float&, float, std::string, std::string );
	void SaveSkins();
	void LoadSkins();
public:
	CConfig()
	{
		Setup();
	}

	void Setup();

	void Save();
	void Load();
};

extern CConfig* Config;
