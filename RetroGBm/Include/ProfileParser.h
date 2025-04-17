#pragma once

#include <string>
#include <vector>
#include <filesystem>

struct ProfileData
{
	std::string checksum;
	std::string filename;
	std::string lastPlayed;
	int totalPlayTimeMinutes = 0;
};

std::vector<ProfileData> ParseProfile(const std::filesystem::path& path);