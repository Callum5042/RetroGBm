#include "ProfileParser.h"
#include "simdjson.h"
#include "json.hpp"

#include <RetroGBm/Logger.h>

#include <fstream>

using namespace simdjson;

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
	};
}

// TODO: This whole simdjson needs to be replaced with nlohmann::json for consistency and ease of use
ProfileData ParseProfile(const std::filesystem::path& path)
{
	ProfileData profile;

	if (!std::filesystem::exists("profile.json"))
	{
		Logger::Warning("profile.json does not exist");
		return profile;
	}

	// Load profile.json
	simdjson_result<padded_string> json_result = padded_string::load("profile.json");
	if (json_result.error() != error_code::SUCCESS)
	{
		return profile;
	}

	const padded_string& json = json_result.value();

	ondemand::parser parser;
	ondemand::document doc = parser.iterate(json);

	// Parse array
	simdjson_result<fallback::ondemand::value> gameData = doc.find_field("gameData");
	if (gameData.error() != error_code::SUCCESS)
	{
		Logger::Error("Unable to parse JSON 'gameData'");
		return profile;
	}

	std::vector<ProfileGameData> profileDataList;
	for (auto game : gameData.get_array())
	{
		ProfileGameData profileData;

		// cheats
		auto cheatsJson = game.find_field("cheats");
		if (cheatsJson.error() == error_code::SUCCESS)
		{
			auto cheatsArray = cheatsJson.get_array();

			for (auto cheatJson : cheatsArray)
			{
				auto codeField = cheatJson.find_field("code");
				auto nameField = cheatJson.find_field("name");

				std::string name, code;

				if (nameField.error() == simdjson::SUCCESS && nameField.type() == simdjson::ondemand::json_type::string)
				{
					name = std::string(nameField.get_string().value());
				}

				if (codeField.error() == simdjson::SUCCESS && codeField.type() == simdjson::ondemand::json_type::string)
				{
					code = std::string(codeField.get_string().value());
				}

				profileData.cheats.push_back({ name, code });
			}
		}

		// checksum
		auto checksumJson = game.find_field("checksum");
		if (checksumJson.error() == error_code::SUCCESS)
		{
			auto& checksum = checksumJson.value();
			if (checksum.is_string())
			{
				profileData.checksum = checksum.get_string().value();
			}
		}
		else
		{
			Logger::Error("Unable to parse JSON 'checksum'");
		}

		// fileName
		auto fileNameJson = game.find_field("fileName");
		if (fileNameJson.error() == error_code::SUCCESS)
		{
			auto& fileName = fileNameJson.value();
			if (fileName.is_string())
			{
				profileData.filename = fileName.get_string().value();
			}
		}
		else
		{
			Logger::Error("Unable to parse JSON 'fileName'");
		}

		// lastPlayed
		auto lastPlayedJson = game.find_field("lastPlayed");
		if (lastPlayedJson.error() == error_code::SUCCESS)
		{
			auto& lastPlayed = lastPlayedJson.value();
			if (lastPlayed.is_string())
			{
				profileData.lastPlayed = lastPlayed.get_string().value();
			}
		}

		// totalPlayTimeMinutes
		auto totalPlayTimeMinutesJson = game.find_field("totalPlayTimeMinutes");
		if (totalPlayTimeMinutesJson.error() == error_code::SUCCESS)
		{
			auto& totalPlayTimeMinutes = totalPlayTimeMinutesJson.value();
			if (totalPlayTimeMinutes.is_integer())
			{
				profileData.totalPlayTimeMinutes = static_cast<int>(totalPlayTimeMinutes.get_int64().value());
			}
		}
		else
		{
			Logger::Error("Unable to parse JSON 'totalPlayTimeMinutes'");
		}

		profileDataList.push_back(profileData);
	}

	// Options
	ProfileOptions options;

	simdjson_result<fallback::ondemand::value> optionsJson = doc.find_field("options");
	if (optionsJson.error() == error_code::SUCCESS)
	{
		// Options rom directories
		auto romDirectoriesJson = optionsJson.find_field("romDirectories");
		if (romDirectoriesJson.error() == error_code::SUCCESS)
		{
			auto& romDirectories = romDirectoriesJson.value();
			if (romDirectories.is_string())
			{
				options.rom_directories = romDirectories.get_string().value();
			}
		}
		else
		{
			Logger::Error("Unable to parse JSON 'romDirectories'");
		}
	}

	// Return 
	profile.options = options;
	profile.gameData = profileDataList;
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