#include "material.h"

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
