#pragma once

#include <string>
#include <vector>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct ProfileOptions
{
	std::string rom_directories = "RomData";

	int key_a = 'X';
	int key_b = 'Z';
	int key_start = VK_RETURN;
	int key_select = VK_TAB;
	int key_up = VK_UP;
	int key_down = VK_DOWN;
	int key_left = VK_LEFT;
	int key_right = VK_RIGHT;
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