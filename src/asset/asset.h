#pragma once

#include "core/core.h"
#include "core/uuid.h"

enum class AssetType
{
    Texture,
    Mesh,
    Material,
    NumTypes
};

enum class AssetFlags
{
    None = 0,
    Serialized
};

ENUM_FLAGS(AssetFlags)

struct AssetMetaData
{
    Uuid ID;
    AssetType Type;
    AssetFlags Flags;
    std::filesystem::path SourceFilepath;
    std::filesystem::path AssetFilepath;
};

class Asset
{
    friend class AssetSerializer;
    friend class AssetImporter;
public:
    inline static const char* AssetFileExtensions[(uint32_t)AssetType::NumTypes] =
    {
        ".hextex",
        ".hexmesh",
        ".hexmat",
    };
public:
    virtual ~Asset() = default;

    inline void SetAssetFlag(AssetFlags flag, bool state = true) { if (state) m_MetaData.Flags |= flag; else m_MetaData.Flags &= ~flag; }
    inline Uuid GetID() const { return m_MetaData.ID; }
    inline AssetType GetAssetType() const { return m_MetaData.Type; }
    inline bool GetAssetFlag(AssetFlags flag) const { return (m_MetaData.Flags & flag) != AssetFlags::None; }
    inline AssetFlags GetAssetFlags() const { return m_MetaData.Flags; }
    inline const std::filesystem::path& GetAssetFilepath() const { return m_MetaData.AssetFilepath; }
    inline const std::filesystem::path& GetSourceFilepath() const { return m_MetaData.SourceFilepath; }
    inline const AssetMetaData& GetMetaData() const { return m_MetaData; }
protected:
    Asset(AssetType type, AssetFlags flags = AssetFlags::None)
    {
        m_MetaData.Type = type;
        m_MetaData.Flags = flags;
    }
protected:
    AssetMetaData m_MetaData;
};