#include "ProfileParser.h"
#include "simdjson.h"
#include "json.hpp"

#include <RetroGBm/Logger.h>

#include <fstream>

using namespace simdjson;

std::vector<ProfileData> ParseProfile(const std::filesystem::path& path)
{
	if (!std::filesystem::exists("profile.json"))
	{
		Logger::Warning("profile.json does not exist");
		return std::vector<ProfileData>();
	}

	// Load profile.json
	simdjson_result<padded_string> json_result = padded_string::load("profile.json");
	if (json_result.error() != error_code::SUCCESS)
	{
		return std::vector<ProfileData>();
	}

	const padded_string& json = json_result.value();

	ondemand::parser parser;
	ondemand::document doc = parser.iterate(json);

	// Parse array
	simdjson_result<fallback::ondemand::value> gameData = doc.find_field("gameData");
	if (gameData.error() != error_code::SUCCESS)
	{
		Logger::Error("Unable to parse JSON 'gameData'");
		return std::vector<ProfileData>();
	}

	std::vector<ProfileData> profileDataList;
	for (auto game : gameData.get_array())
	{
		ProfileData profileData;

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

	return profileDataList;
}

void to_json(nlohmann::json& j, const ProfileData& p)
{
	j = nlohmann::json
	{
		{"checksum", p.checksum},
		{"fileName", p.filename},
		{"lastPlayed", p.lastPlayed},
		{"totalPlayTimeMinutes", p.totalPlayTimeMinutes}
	};
}

void SaveProfile(const std::filesystem::path& path, const std::vector<ProfileData>& data)
{
	nlohmann::json j; // thanks to the `to_json` function
	// to_json(j, data);

	j["gameData"] = data;

	std::string json = j.dump(2);

	std::ofstream file("profile.json", std::ios_base::trunc);
	file << json;
}