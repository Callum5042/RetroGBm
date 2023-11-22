#include "Utilities/Utilities.h"
#include <Windows.h>
#include <vector>

std::string Utilities::ConvertToString(const std::wstring& str)
{
	size_t size = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);

	std::vector<char> buffer(size);
	int chars_converted = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), buffer.data(), static_cast<int>(buffer.size()), NULL, NULL);

	return std::string(buffer.data(), chars_converted);
}

std::wstring Utilities::ConvertToWString(const std::string& str)
{
	size_t size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0);

	std::vector<wchar_t> buffer(size);
	int chars_converted = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), buffer.data(), static_cast<int>(buffer.size()));

	return std::wstring(buffer.data(), chars_converted);
}