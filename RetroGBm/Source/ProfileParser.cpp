#include "ProfileParser.h"
#include "json.hpp"

#include <RetroGBm/Logger.h>

#include <fstream>

// JSON - Must be first
void to_json(nlohmann::json& j, const ProfileCheats& p)
{
	j = nlohmann::json
	{
		{"name", p.name},
		{"code", p.code}
	};
}

void to_json(nlohmann::json& j, const ProfileGameData& p)
{
	j = nlohmann::json
	{
		{"checksum", p.checksum},
		{"fileName", p.filename},
		{"lastPlayed", p.lastPlayed},
		{"totalPlayTimeMinutes", p.totalPlayTimeMinutes},
		{"cheats", p.cheats}
	};
}

void to_json(nlohmann::json& j, const ProfileOptions& p)
{
	j = nlohmann::json
	{
		{"romDirectories", p.rom_directories},
		{"key_a", p.key_a},
		{"key_b", p.key_b},
		{"key_start", p.key_start},
		{"key_select", p.key_select},
		{"key_up", p.key_up},
		{"key_down", p.key_down},
		{"key_left", p.key_left},
		{"key_right", p.key_right},
	};
}

ProfileData ParseProfile(const std::filesystem::path& path)
{
	ProfileData profile;

	if (!std::filesystem::exists("profile.json"))
	{
		Logger::Warning("profile.json does not exist");
		return profile;
	}

	std::ifstream file("profile.json");
	if (!file)
	{
		Logger::Error("Unable to open profile.json");
		return profile;
	}

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	nlohmann::json doc;
	try
	{
		doc = nlohmann::json::parse(content);
	}
	catch (const std::exception& e)
	{
		Logger::Error("Failed to parse profile.json: " + std::string(e.what()));
		return profile;
	}

	// Parse gameData
	if (doc.contains("gameData") && doc["gameData"].is_array())
	{
		for (const auto& game : doc["gameData"])
		{
			ProfileGameData profileData;

			if (game.contains("cheats") && game["cheats"].is_array())
			{
				for (const auto& cheat : game["cheats"]) 
				{
					ProfileCheats pc;
					if (cheat.contains("name")) pc.name = cheat["name"].get<std::string>();
					if (cheat.contains("code")) pc.code = cheat["code"].get<std::string>();
					profileData.cheats.push_back(pc);
				}
			}

			if (game.contains("checksum")) profileData.checksum = game["checksum"].get<std::string>();
			if (game.contains("fileName")) profileData.filename = game["fileName"].get<std::string>();
			if (game.contains("lastPlayed")) profileData.lastPlayed = game["lastPlayed"].get<std::string>();
			if (game.contains("totalPlayTimeMinutes")) profileData.totalPlayTimeMinutes = game["totalPlayTimeMinutes"].get<int>();

			profile.gameData.push_back(profileData);
		}
	}

	// Options
	if (doc.contains("options") && doc["options"].is_object())
	{
		const auto& optionsJson = doc["options"];
		if (optionsJson.contains("romDirectories")) profile.options.rom_directories = optionsJson["romDirectories"].get<std::string>();
		if (optionsJson.contains("key_a")) profile.options.key_a = optionsJson["key_a"].get<int>();
		if (optionsJson.contains("key_b")) profile.options.key_b = optionsJson["key_b"].get<int>();
		if (optionsJson.contains("key_start")) profile.options.key_start = optionsJson["key_start"].get<int>();
		if (optionsJson.contains("key_select")) profile.options.key_select = optionsJson["key_select"].get<int>();
		if (optionsJson.contains("key_up")) profile.options.key_up = optionsJson["key_up"].get<int>();
		if (optionsJson.contains("key_down")) profile.options.key_down = optionsJson["key_down"].get<int>();
		if (optionsJson.contains("key_left")) profile.options.key_left = optionsJson["key_left"].get<int>();
		if (optionsJson.contains("key_right")) profile.options.key_right = optionsJson["key_right"].get<int>();
	}

	return profile;
}

void SaveProfile(const std::filesystem::path& path, const ProfileData& data)
{
	nlohmann::json j;
	j["options"] = data.options;
	j["gameData"] = data.gameData;

	std::string json = j.dump(2);

	std::ofstream file("profile.json", std::ios_base::trunc);
	file << json;
}
