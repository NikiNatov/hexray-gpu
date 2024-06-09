#include "material.h"
#include "serialization/parsedblock.h"

// ------------------------------------------------------------------------------------------------------------------------------------
Material::Material(MaterialFlags flags)
    : m_Flags(flags)
{
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Material::SetFlag(MaterialFlags flag, bool state)
{
    if (state)
        m_Flags |= flag;
    else
        m_Flags &= ~flag;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Material::Serialize(ParsedBlock& pb)
{
    pb.GetProperty("color", m_AlbedoColor);
    pb.GetProperty("texture", m_AlbedoMap);
}
