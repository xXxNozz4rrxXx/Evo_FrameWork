#include "Parser.h"

namespace valve_parser
{
	Object::Object(Document* doc) : Node(doc) {}

	Object* Object::ToObject()
	{
		return this;
	}

	std::shared_ptr<Node> Object::GetKeyByName(char* name)
	{
		for (auto& child : children)
		{
			if (child->ToKeyValue())
			{
				if (Util::StrEqu(child->ToKeyValue()->Key, name))
					return child;
			}
		}

		return 0;
	}

	bool Object::Parse()
	{
		std::shared_ptr<Node> n;

		while (*_doc->p)
		{
			//check for object close
			auto string_begin = Str::ParseTextExpectedTag(_doc->p, STRING, true);
			if (!string_begin)
			{
				auto obj_close = Str::ParseTextExpectedTag(_doc->p, OBJECT_CLOSE, true);
				if (obj_close)
				{
					_doc->p = obj_close + 1;
					return true;
				}
				else
					return false;
			}

			if (!_doc->identify(n))
				return false;

			if (n->ToKeyValue())
			{
				this->children.push_back(n);
			}

			if (n->ToObject())
			{
				this->children.push_back(n);
				Object* obj = n->ToObject();
				if (!obj->Parse())
					return false;
			}
		}
		return false;
	}

	bool Node::Parse()
	{
		std::shared_ptr<Node> n;

		while (*_doc->p)
		{
			if (!_doc->identify(n))
			{
				if (!Str::EndReached(_doc->p, OBJECT_OPEN) &&
					!Str::EndReached(_doc->p, OBJECT_CLOSE) &&
					!Str::EndReached(_doc->p, STRING))
					return true;
				else
					return false;
			}

			if (n->ToKeyValue())
			{
				this->children.push_back(n);
			}

			if (n->ToObject())
			{
				this->children.push_back(n);
				Object* obj = n->ToObject();
				if (!obj->Parse())
					return false;
			}
		}
		return false;
	}
	int ParseSkins()
	{
		valve_parser::Document doc;
		auto r = doc.Load(".\\csgo\\scripts\\items\\items_game.txt", valve_parser::ENCODING::UTF8);
		if (!r)
			return 1;

		valve_parser::Document english;
		r = english.Load(".\\csgo\\resource\\csgo_english.txt", valve_parser::ENCODING::UTF16_LE);
		if (!r)
			return 2;

		auto weaponSkinCombo = doc.BreadthFirstSearch("weapon_icons");
		if (!weaponSkinCombo || !weaponSkinCombo->ToObject())
			return 3;

		auto skinDataVec = doc.BreadthFirstSearchMultiple("paint_kits");
		if (!skinDataVec.size())
			return 4;

		auto PaintKitNames = english.BreadthFirstSearch("Tokens");
		if (!PaintKitNames || !PaintKitNames->ToObject())
			return 5;

		//std::unordered_map<std::string, std::set<std::string>> G::weaponSkins;
		//std::unordered_map<std::string, skinInfo> G::skinMap;
		//std::unordered_map<std::string, std::string> G::skinNames;

		std::vector<std::string> weaponNames = {
			"deagle",
			"elite",
			"fiveseven",
			"glock",
			"ak47",
			"aug",
			"awp",
			"famas",
			"g3sg1",
			"galilar",
			"m249",
			"m4a1_silencer", //needs to be before m4a1 else silencer doesnt get filtered out :D
			"m4a1",
			"mac10",
			"p90",
			"ump45",
			"xm1014",
			"bizon",
			"mag7",
			"negev",
			"sawedoff",
			"tec9",
			"hkp2000",
			"mp7",
			"mp9",
			"nova",
			"p250",
			"scar20",
			"sg556",
			"ssg08",
			"usp_silencer",
			"cz75a",
			"revolver",
			"knife_m9_bayonet", //needs to be before bayonet else knife_m9_bayonet doesnt get filtered out :D
			"bayonet",
			"knife_flip",
			"knife_gut",
			"knife_karambit",
			"knife_tactical",
			"knife_falchion",
			"knife_survival_bowie",
			"knife_butterfly",
			"knife_push",
			"knife_ursus",
			"knife_gypsy_jackknife",
			"knife_stiletto",
			"knife_widowmaker"

		};

		//populate G::weaponSkins
		for (auto child : weaponSkinCombo->children)
		{
			if (child->ToObject())
			{
				for (auto weapon : weaponNames)
				{
					auto skinName = child->ToObject()->GetKeyByName("icon_path")->ToKeyValue()->Value.toString();
					auto pos = skinName.find(weapon);
					//filter out the skinname
					if (pos != std::string::npos)
					{
						auto pos2 = skinName.find_last_of('_');
						G.weaponSkins[weapon].insert(
							skinName.substr(pos + weapon.length() + 1,
								pos2 - pos - weapon.length() - 1)
						);
						break;
					}
				}
			}
		}

		//populate skinData
		for (auto skinData : skinDataVec)
		{
			if (skinData->ToObject())
			{
				for (auto skin : skinData->children)
				{
					if (skin->ToObject())
					{
						skinInfo si;
						si.paintkit = skin->ToObject()->name.toInt();

						auto skinName = skin->ToObject()->GetKeyByName("name")->ToKeyValue()->Value.toString();
						auto tagNode = skin->ToObject()->GetKeyByName("description_tag");
						if (tagNode)
						{
							std::string tag = tagNode->ToKeyValue()->Value.toString();
							tag = tag.substr(1, std::string::npos); //skip #
							std::transform(tag.begin(), tag.end(), tag.begin(), towlower);
							si.tagName = tag;
						}

						auto keyVal = skin->ToObject()->GetKeyByName("seed");
						if (keyVal != nullptr)
							si.seed = keyVal->ToKeyValue()->Value.toInt();

						G.skinMap[skinName] = si;
					}
				}
			}
		}

		//populate G::skinNames
		for (auto child : PaintKitNames->children)
		{
			if (child->ToKeyValue())
			{
				std::string key = child->ToKeyValue()->Key.toString();
				std::transform(key.begin(), key.end(), key.begin(), towlower);
				if (key.find("paintkit") != std::string::npos &&
					key.find("tag") != std::string::npos)
				{
					G.skinNames[key] = child->ToKeyValue()->Value.toString();
				}
			}
		}
		return 0;
	}
}

