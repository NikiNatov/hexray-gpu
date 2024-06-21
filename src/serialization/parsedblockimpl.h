#pragma once
#include "serialization/parsedblock.h"

#include <vector>

class SceneParser;

class ParsedBlockImpl : public ParsedBlock
{
public:
	virtual bool GetProperty(const char* name, int& value, int minValue = INT_MIN, int maxValue = INT_MAX);
	virtual bool GetProperty(const char* name, bool& value);
	virtual bool GetProperty(const char* name, float& value, float minValue = -FLT_MAX, float maxValue = FLT_MAX);
	virtual bool GetProperty(const char* name, double& value, double minValue = -DBL_MAX, double maxValue = DBL_MAX);
	virtual bool GetProperty(const char* name, glm::vec4& value, float minValue = -FLT_MAX, float maxValue = FLT_MAX);
	virtual bool GetProperty(const char* name, glm::vec3& value);
	virtual bool GetProperty(const char* name, MeshPtr& value);
	virtual bool GetProperty(const char* name, MaterialPtr& value);
	virtual bool GetProperty(const char* name, TexturePtr& value);
	virtual bool GetProperty(const char* name, char* value);

	bool FindProperty(const char* name, int& i_s, int& line_s, char*& value);
	virtual bool GetFilenameProp(const char* name, char* value);
	virtual void GetProperty(TransformComponent& T);

	virtual void RequiredProp(const char* name);

	void SignalError(const char* msg);
	void SignalWarning(const char* msg);
	int GetBlockLines();
	void GetBlockLine(int idx, int& srcLine, char head[], char tail[]);
private:
	friend class DefaultSceneParser;
	struct LineInfo
	{
		int line;
		char propName[128];
		char propValue[256];
		bool recognized;

		LineInfo() {}
		LineInfo(int line, const char* name, const char* value) : line(line)
		{
			strncpy(propName, name, sizeof(propName) - 1);
			strncpy(propValue, value, sizeof(propValue) - 1);
			recognized = false;
		}
	};
	std::vector<LineInfo> m_Lines;
	int blockBegin, blockEnd; // line numbers
	SceneParser* parser;
};