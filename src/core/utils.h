#pragma once
#include <string>
#include <vector>

std::string ToString(const std::wstring& str);
std::wstring ToWString(const std::string& str);
std::vector<std::string> Tokenize(const std::string& str);

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