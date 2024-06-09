#include "utils.h"

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
