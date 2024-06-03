#include "uuid.h"

#include <random>

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid::Uuid()
    : m_ID(s_UniformDistribution(s_Engine))
{
}

// ------------------------------------------------------------------------------------------------------------------------------------
Uuid::Uuid(uint64_t uuid)
    : m_ID(uuid)
{
}
