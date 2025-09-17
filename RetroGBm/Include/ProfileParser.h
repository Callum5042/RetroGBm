#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct ProfileOptions
{
	std::string rom_directories = "RomData";
};

struct ProfileCheats
{
	std::string name;
	std::string code;
};

struct ProfileGameData
{
	std::string checksum;
	std::string filename;
	std::string lastPlayed;
	int totalPlayTimeMinutes = 0;
	std::vector<ProfileCheats> cheats;
};

struct ProfileData
{
	ProfileOptions options;
	std::vector<ProfileGameData> gameData;
};

ProfileData ParseProfile(const std::filesystem::path& path);

void SaveProfile(const std::filesystem::path& path, const ProfileData& data);