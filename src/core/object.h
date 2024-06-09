#pragma once

class ParsedBlock;

class HObject
{
public:
	HObject() = default;
	virtual ~HObject() = default;

	virtual void Serialize(ParsedBlock& pb) {}
};