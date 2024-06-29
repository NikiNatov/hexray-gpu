#pragma once
#include <string>
#include <vector>
#include <filesystem>

bool ReadFile(const std::filesystem::path& path, std::vector<uint8_t>& outData);
bool ReadFile(const std::filesystem::path& path, std::string& outData);

std::string Printf(const char* format, ...);
std::string ToString(const std::wstring& str);
std::wstring ToWString(const std::string& str);
std::vector<std::string> Tokenize(const std::string& str);

bool AreAllBytesZero(void* data, size_t size);

/// a simple RAII class for FILE* pointers.
class FileRAII
{
	FILE* held;
public:
	FileRAII(FILE* init) : held(init) {}
	~FileRAII() { if (held) fclose(held); held = NULL; }
	FileRAII(const FileRAII&) = delete;
	FileRAII& operator = (const FileRAII&) = delete;
};

// Based on boost::hash_combine
template <typename T>
void HashCombine(size_t& accumulatedHash, const T& value)
{
	accumulatedHash ^= std::hash<T>()(value) + 0x9e3779b9 + (accumulatedHash << 6) + (accumulatedHash >> 2);
}

template <typename T, typename U>
struct std::hash<std::pair<T, U>>
{
	std::size_t operator()(const std::pair<T, U>& p) const noexcept
	{
		std::size_t hash = 1337;
		HashCombine(hash, p.first);
		HashCombine(hash, p.second);
		return hash;
	}
};