#include "parsedblockimpl.h"
#include "core/utils.h"
#include "scene/component.h"
#include "serialization/sceneparser.h"

#include <vector>
#include <cstdarg>
#include <random>

void Get3Floats(int srcLine, char* expression, float& d1, float& d2, float& d3)
{
	int l = (int)strlen(expression);
	for (int i = 0; i < l; i++) {
		char c = expression[i];
		if (c == '(' || c == ')' || c == ',') expression[i] = ' ';
	}
	if (3 != sscanf(expression, "%f%f%f", &d1, &d2, &d3)) {
		throw SyntaxError(srcLine, "Expected three double values");
	}
}

bool GetFrontToken(char* s, char* frontToken)
{
	int l = (int)strlen(s);
	int i = 0;
	while (i < l && isspace(s[i])) i++;
	if (i == l) return false;
	int j = 0;
	while (i < l && !isspace(s[i])) frontToken[j++] = s[i++];
	if (i == l) return false;
	frontToken[j] = 0;
	j = 0;
	while (i <= l) s[j++] = s[i++];
	return true;
}

bool GetLastToken(char* s, char* backToken)
{
	int l = (int)strlen(s);
	int i = l - 1;
	while (i >= 0 && isspace(s[i])) i--;
	if (i < 0) return false;
	int j = i;
	while (j >= 0 && !isspace(s[j])) j--;
	if (j < 0) return false;
	strncpy(backToken, &s[j + 1], i - j);
	backToken[i - j] = 0;
	s[++j] = 0;
	return true;
}

void StripPunctuation(char* s)
{
	char temp[1024];
	strncpy(temp, s, sizeof(temp) - 1);
	int l = (int)strlen(temp);
	int j = 0;
	for (int i = 0; i < l; i++) {
		if (!isspace(temp[i]) && temp[i] != ',')
			s[j++] = temp[i];
	}
	s[j] = 0;
}

SyntaxError::SyntaxError()
{
	line = -1;
	msg[0] = 0;
}

SyntaxError::SyntaxError(int line, const char* format, ...)
{
	this->line = line;
	va_list ap;
	va_start(ap, format);
#ifdef _MSC_VER
#	define vsnprintf _vsnprintf
#endif
	vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);
}

FileNotFoundError::FileNotFoundError() {}
FileNotFoundError::FileNotFoundError(int line, const char* filename)
{
	this->line = line;
	strcpy(this->filename, filename);
}

bool ParsedBlockImpl::FindProperty(const char* name, int& i_s, int& line_s, char*& value)
{
	for (int i = 0; i < (int)m_Lines.size(); i++) if (!stricmp(m_Lines[i].propName, name)) {
		i_s = i;
		line_s = m_Lines[i].line;
		value = m_Lines[i].propValue;
		m_Lines[i].recognized = true;
		return true;
	}
	return false;
}

#define PBEGIN\
	int i_s = 0; \
	char* value_s; \
	int line = 0; \
	if (!FindProperty(name, i_s, line, value_s)) return false;
bool ParsedBlockImpl::GetProperty(const char* name, int& value, int minValue, int maxValue)
{
	PBEGIN;
	int x;
	if (1 != sscanf(value_s, "%d", &x)) throw SyntaxError(line, "Invalid integer");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%d .. %d)\n", minValue, maxValue);
	value = x;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, uint32_t& value, uint32_t minValue, uint32_t maxValue)
{
	PBEGIN;
	int x;
	if (1 != sscanf(value_s, "%u", &x)) throw SyntaxError(line, "Invalid integer");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%u .. %u)\n", minValue, maxValue);
	value = x;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, bool& value)
{
	PBEGIN;
	if (!strcmp(value_s, "off") || !strcmp(value_s, "false") || !strcmp(value_s, "0")) value = false;
	else value = true;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, bool& value, bool defaultValue)
{
	value = defaultValue;
	return GetProperty(name, value);
}

bool ParsedBlockImpl::GetProperty(const char* name, float& value, float minValue, float maxValue)
{
	PBEGIN;
	float x;
	if (1 != sscanf(value_s, "%f", &x)) throw SyntaxError(line, "Invalid float");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%f .. %f)\n", minValue, maxValue);
	value = x;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, double& value, double minValue, double maxValue)
{
	PBEGIN;
	double x;
	if (1 != sscanf(value_s, "%lf", &x)) throw SyntaxError(line, "Invalid double");
	if (x < minValue || x > maxValue) throw SyntaxError(line, "Value outside the allowed bounds (%f .. %f)\n", minValue, maxValue);
	value = x;
	return true;
}

void stripBracesAndCommas(char* s)
{
	int l = (int)strlen(s);
	for (int i = 0; i < l; i++) {
		char& c = s[i];
		if (c == ',' || c == '(' || c == ')') c = ' ';
	}
}

bool ParsedBlockImpl::GetProperty(const char* name, glm::vec4& value, float minCompValue, float maxCompValue)
{
	PBEGIN;
	stripBracesAndCommas(value_s);
	glm::vec4 c;
	if (3 != sscanf(value_s, "%f%f%f", &c.r, &c.g, &c.b)) throw SyntaxError(line, "Invalid color");
	if (c.r < minCompValue || c.r > maxCompValue) throw SyntaxError(line, "Color R value outside the allowed bounds (%f .. %f)\n", minCompValue, maxCompValue);
	if (c.g < minCompValue || c.g > maxCompValue) throw SyntaxError(line, "Color G value outside the allowed bounds (%f .. %f)\n", minCompValue, maxCompValue);
	if (c.b < minCompValue || c.b > maxCompValue) throw SyntaxError(line, "Color B value outside the allowed bounds (%f .. %f)\n", minCompValue, maxCompValue);
	value = c;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, glm::vec3& value)
{
	PBEGIN;
	stripBracesAndCommas(value_s);
	glm::vec3 v;
	if (3 != sscanf(value_s, "%f%f%f", &v.x, &v.y, &v.z)) throw SyntaxError(line, "Invalid vector");
	value = v;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, MeshPtr& value)
{
	PBEGIN;
	MeshPtr g = parser->FindMeshByName(value_s);
	if (!g) throw SyntaxError(line, "Mesh not defined");
	value = g;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, MaterialPtr& value)
{
	PBEGIN;
	MaterialPtr s = parser->FindMaterialByName(value_s);
	if (!s) throw SyntaxError(line, "Material not defined");
	value = s;
	return true;
}

bool ParsedBlockImpl::GetProperty(const char* name, TexturePtr& value)
{
	PBEGIN;
	TexturePtr t = parser->FindTextureByName(value_s);
	if (!t) throw SyntaxError(line, "Texture not defined");
	value = t;
	return true;
}

// TODO VIK
//bool ParsedBlockImpl::getNodeProp(const char* name, Node** value)
//{
//	PBEGIN;
//	Node* n = parser->findNodeByName(value_s);
//	if (!n) throw SyntaxError(line, "Node not defined");
//	*value = n;
//	return true;
//}

bool ParsedBlockImpl::GetProperty(const char* name, char* value)
{
	PBEGIN;
	strcpy(value, value_s);
	return true;
}

bool ParsedBlockImpl::GetFilenameProp(const char* name, char* value)
{
	PBEGIN;
	strcpy(value, value_s);
	if (parser->ResolveFullPath(value)) return true;
	else throw FileNotFoundError(line, value_s);
}

void ParsedBlockImpl::GetProperty(TransformComponent& T)
{
	for (int i = 0; i < (int)m_Lines.size(); i++) {
		float x, y, z;
		if (!strcmp(m_Lines[i].propName, "scale"))
		{
			m_Lines[i].recognized = true;
			Get3Floats(m_Lines[i].line, m_Lines[i].propValue, x, y, z);
			T.Scale = { x, y, z };
			continue;
		}
		if (!strcmp(m_Lines[i].propName, "rotate"))
		{
			m_Lines[i].recognized = true;
			Get3Floats(m_Lines[i].line, m_Lines[i].propValue, x, y, z);
			T.Rotation = { y, -x, z }; // Note: Map between both coord systems, rotation here: Z -> Y -> X, hexray: Z -> X -> Y
			continue;
		}
		if (!strcmp(m_Lines[i].propName, "translate"))
		{
			m_Lines[i].recognized = true;
			Get3Floats(m_Lines[i].line, m_Lines[i].propValue, x, y, z);
			T.Translation = { x, y, z };
			continue;
		}
	}
}

void ParsedBlockImpl::RequiredProp(const char* name)
{
	int k1, k2;
	char* strValue;
	if (!FindProperty(name, k1, k2, strValue))
	{
		throw SyntaxError(-1, "Required property `%s' not defined", name);
	}
}

void ParsedBlockImpl::SignalError(const char* msg)
{
	throw SyntaxError(blockEnd, msg);
}

void ParsedBlockImpl::SignalWarning(const char* msg)
{
	fprintf(stderr, "Warning (at line %d): %s\n", blockEnd, msg);
}


int ParsedBlockImpl::GetBlockLines()
{
	return (int)m_Lines.size();
}
void ParsedBlockImpl::GetBlockLine(int idx, int& srcLine, char head[], char tail[])
{
	m_Lines[idx].recognized = true;
	srcLine = m_Lines[idx].line;
	strcpy(head, m_Lines[idx].propName);
	strcpy(tail, m_Lines[idx].propValue);
}
