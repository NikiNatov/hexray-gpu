#pragma once

#include "core/core.h"

class Uuid
{
public:
    static Uuid Invalid;
public:
    Uuid();
    Uuid(uint64_t uuid);

    inline operator uint64_t() const { return m_ID; }
private:
    uint64_t m_ID;
};

template<>
struct std::hash<Uuid>
{
    size_t operator()(const Uuid& uuid) const
    {
        return (uint64_t)uuid;
    }
};