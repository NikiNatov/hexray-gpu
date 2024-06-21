#pragma once
#include "rendering/resources_fwd.h"

#include <climits>
#include <cfloat>
#include <glm.hpp>

struct TransformComponent;
class SceneParser;


class ParsedBlock
{
public:
	virtual ~ParsedBlock() {}
	// All these methods are intended to be called by SceneElement implementations
	// each method accepts two parameters: a name, and a value pointer.
	// Each method does one of these things:
	//  - the property with the given name is found and parsed successfully, in which
	//    case the value is filled in, and the method returns true.
	//  - The property is found and it wasn't parsed successfully, in which case
	//    a syntax error exception is raised.
	//  - The property is missing in the scene file, the value is untouched, and the method
	//    returns false (if this is an error, you can signal it with pb.RequiredProp(name))
	//
	// Some properties also have min/max ranges. If they are specified and the parsed value
	// does not pass the range check, then a SyntaxError is raised.
	virtual bool GetProperty(const char* name, int& value, int minValue = INT_MIN, int maxValue = INT_MAX) = 0;
	virtual bool GetProperty(const char* name, bool& value) = 0;
	virtual bool GetProperty(const char* name, float& value, float minValue = -FLT_MAX, float maxValue = FLT_MAX) = 0;
	virtual bool GetProperty(const char* name, double& value, double minValue = -DBL_MAX, double maxValue = DBL_MAX) = 0;
	virtual bool GetProperty(const char* name, glm::vec4& value, float minCompValue = -FLT_MAX, float maxCompValue = FLT_MAX) = 0;
	virtual bool GetProperty(const char* name, glm::vec3& value) = 0;
	virtual bool GetProperty(const char* name, MeshPtr& value) = 0;
	virtual bool GetProperty(const char* name, MaterialPtr& value) = 0;
	virtual bool GetProperty(const char* name, TexturePtr& value) = 0;
	virtual bool GetProperty(const char* name, char* value) = 0; // the buffer should be 256 chars long

	virtual void RequiredProp(const char* name) = 0; // signal an error (missing property of the given name)

	template <typename T, typename ... Args>
	bool GetRequiredProperty(const char* name, T& value, Args&&... args)
	{
		RequiredProp(name);
		return GetProperty(name, value, std::forward<Args>(args)...);
	}

	// useful for scene assets like textures, mesh files, etc.
	// the value will hold the full filename to the file.
	// If the file/dir is not found, a FileNotFound exception is raised.
	virtual bool GetFilenameProp(const char* name, char* value) = 0;

	// Gets a transform from the parsed block. Namely, it searches for all properties named
	// "scale", "rotate" and "translate" and applies them to T.
	virtual void GetProperty(TransformComponent& T) = 0;

	virtual void SignalError(const char* msg) = 0; // signal an error with a specified message
	virtual void SignalWarning(const char* msg) = 0; // signal a warning with a specified message

	// some functions for direct parsed block access:
	virtual int GetBlockLines() = 0;
	virtual void GetBlockLine(int idx, int& srcLine, char head[], char tail[]) = 0;
};

struct SyntaxError
{
	char msg[128];
	int line;
	SyntaxError();
	SyntaxError(int line, const char* format, ...);
};

struct FileNotFoundError
{
	char filename[245];
	int line;
	FileNotFoundError();
	FileNotFoundError(int line, const char* filename);
};
