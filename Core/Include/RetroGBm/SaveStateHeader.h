#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct SaveStateHeader
{
	char identifier[8] = { 'R', 'E', 'T', 'R', 'O', 'G', 'B', 'M' };
	int version = 1;

	int64_t date_created = 0;
	int64_t date_modified = 0;
	double time_played = 0;

	char reserved[28] = {};
};

#pragma pack(pop)

static_assert(sizeof(SaveStateHeader) == 64);