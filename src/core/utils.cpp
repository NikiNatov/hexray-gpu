#include "utils.h"

std::string ToString(const std::wstring& str)
{
	std::string temp(str.length(), '0');
	std::copy(str.begin(), str.end(), temp.begin());
	return temp;
}

std::wstring ToWString(const std::string& str)
{
	std::wstring temp(str.length(), L'0');
	std::copy(str.begin(), str.end(), temp.begin());
	return temp;
}
