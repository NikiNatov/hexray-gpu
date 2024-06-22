#include "utils.h"

#include <cstdarg>
#include <fstream>

// ------------------------------------------------------------------------------------------------------------------------------------
bool ReadFile(const std::filesystem::path& path, std::vector<uint8_t>& outData)
{
	std::ifstream ifs(path, std::ios::in | std::ios::binary | std::ios::ate);

	if (!ifs)
	{
		return false;
	}

	uint32_t fileSize = ifs.tellg();
	outData.resize(fileSize);
	ifs.seekg(0, std::ios::beg);
	ifs.read((char*)outData.data(), fileSize);
	ifs.close();
	return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool ReadFile(const std::filesystem::path& path, std::string& outData)
{
	std::ifstream ifs(path, std::ios::in | std::ios::binary | std::ios::ate);

	if (!ifs)
	{
		return false;
	}

	uint32_t fileSize = ifs.tellg();
	outData.resize(fileSize);
	ifs.seekg(0, std::ios::beg);
	ifs.read(outData.data(), fileSize);
	ifs.close();
	return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
std::string Printf(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	constexpr uint32_t kStartSize = 256;
	char buffer[kStartSize];

	int32_t result = _vsnprintf(buffer, kStartSize, format, args);
	if (result == -1)
	{
		uint32_t numTry = 2;
		std::string heapBuffer;
		while (result == -1)
		{
			heapBuffer.resize(kStartSize * numTry);
			result = _vsnprintf(heapBuffer.data(), heapBuffer.size(), format, args);
			numTry++;
		}

		heapBuffer.resize(uint32_t(result));
		va_end(args);
		return heapBuffer;
	}

	va_end(args);
	return buffer;
}

// ------------------------------------------------------------------------------------------------------------------------------------
std::string ToString(const std::wstring& str)
{
	std::string temp(str.length(), '0');
	std::copy(str.begin(), str.end(), temp.begin());
	return temp;
}

// ------------------------------------------------------------------------------------------------------------------------------------
std::wstring ToWString(const std::string& str)
{
	std::wstring temp(str.length(), L'0');
	std::copy(str.begin(), str.end(), temp.begin());
	return temp;
}

// ------------------------------------------------------------------------------------------------------------------------------------
std::vector<std::string> Tokenize(const std::string& s)
{
	int i = 0, j, l = (int)s.length();
	std::vector<std::string> result;
	while (i < l) {
		while (i < l && isspace(s[i])) i++;
		if (i >= l) break;
		j = i;
		while (j < l && !isspace(s[j])) j++;
		result.push_back(s.substr(i, j - i));
		i = j;
	}
	return result;
}
