#include "defaultresources.h"

#include "rendering/graphicscontext.h"

std::shared_ptr<Texture> DefaultResources::BlackTexture = nullptr;
std::shared_ptr<Texture> DefaultResources::BlackTextureCube = nullptr;
std::shared_ptr<Texture> DefaultResources::WhiteTexture = nullptr;
std::shared_ptr<Texture> DefaultResources::WhiteTextureCube = nullptr;
std::shared_ptr<Texture> DefaultResources::ErrorTexture = nullptr;
std::shared_ptr<Texture> DefaultResources::ErrorTextureCube = nullptr;
std::shared_ptr<Material> DefaultResources::DefaultMaterial = nullptr;
std::shared_ptr<Mesh> DefaultResources::QuadMesh = nullptr;

// ------------------------------------------------------------------------------------------------------------------------------------
void DefaultResources::Initialize()
{
    // Error textures
    {
        TextureDescription errorTextureDesc;
        errorTextureDesc.Width = 1;
        errorTextureDesc.Height = 1;
        errorTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        errorTextureDesc.MipLevels = 1;
        errorTextureDesc.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;

        uint8_t errorTextureData[] = { 0xFF, 0x00, 0xFF, 0xFF };
        ErrorTexture = std::make_shared<Texture>(errorTextureDesc, L"DefaultResources::ErrorTexture");
        ErrorTexture->UploadGPUData(errorTextureData);

        errorTextureDesc.IsCubeMap = true;

        uint8_t errorTextureCubeData[] = { 
            0xFF, 0x00, 0xFF, 0xFF,
            0xFF, 0x00, 0xFF, 0xFF,
            0xFF, 0x00, 0xFF, 0xFF,
            0xFF, 0x00, 0xFF, 0xFF,
            0xFF, 0x00, 0xFF, 0xFF,
            0xFF, 0x00, 0xFF, 0xFF
        };

        ErrorTextureCube = std::make_shared<Texture>(errorTextureDesc, L"DefaultResources::ErrorTextureCube");
        ErrorTextureCube->UploadGPUData(errorTextureCubeData);
    }

    // Black textures
    {
        TextureDescription blackTextureDesc;
        blackTextureDesc.Width = 1;
        blackTextureDesc.Height = 1;
        blackTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        blackTextureDesc.MipLevels = 1;
        blackTextureDesc.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;

        uint8_t blackTextureData[] = { 0x00, 0x00, 0x00, 0xFF };
        BlackTexture = std::make_shared<Texture>(blackTextureDesc, L"DefaultResources::BlackTexture");
        BlackTexture->UploadGPUData(blackTextureData);

        blackTextureDesc.IsCubeMap = true;

        uint8_t blackTextureCubeData[] = {
            0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF
        };

        BlackTextureCube = std::make_shared<Texture>(blackTextureDesc, L"DefaultResources::BlackTextureCube");
        BlackTextureCube->UploadGPUData(blackTextureCubeData);
    }

    // White textures
    {
        TextureDescription whiteTextureDesc;
        whiteTextureDesc.Width = 1;
        whiteTextureDesc.Height = 1;
        whiteTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        whiteTextureDesc.MipLevels = 1;
        whiteTextureDesc.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;

        uint8_t whiteTextureData[] = { 0xFF, 0xFF, 0xFF, 0xFF };
        WhiteTexture = std::make_shared<Texture>(whiteTextureDesc, L"DefaultResources::WhiteTexture");
        WhiteTexture->UploadGPUData(whiteTextureData);

        whiteTextureDesc.IsCubeMap = true;

        uint8_t whiteTextureCubeData[] = {
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF
        };

        WhiteTextureCube = std::make_shared<Texture>(whiteTextureDesc, L"DefaultResources::WhiteTextureCube");
        WhiteTextureCube->UploadGPUData(whiteTextureCubeData);
    }

    // Default material
    DefaultMaterial = std::make_shared<Material>(MaterialType::PBR, MaterialFlags::TwoSided);
    DefaultMaterial->SetProperty(MaterialPropertyType::AlbedoColor, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    DefaultMaterial->SetProperty(MaterialPropertyType::Metalness, 0.5f);
    DefaultMaterial->SetProperty(MaterialPropertyType::Roughness, 0.5f);

    // Quad mesh
    MeshDescription quadMeshDesc;
    quadMeshDesc.Submeshes = { Submesh{ 0, 4, 0, 6, 0 } };
    quadMeshDesc.MaterialTable = std::make_shared<MaterialTable>(1);
    quadMeshDesc.MaterialTable->SetMaterial(0, DefaultMaterial);

    uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
    Vertex vertices[]  = {
        Vertex{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
        Vertex{ glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
        Vertex{ glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
        Vertex{ glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) }
    };

    QuadMesh = std::make_shared<Mesh>(quadMeshDesc, L"DefaultResources::Quad");
    QuadMesh->UploadGPUData(vertices, indices);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void DefaultResources::Shutdown()
{
    // Textures
    BlackTexture = nullptr;
    BlackTextureCube = nullptr;
    WhiteTexture = nullptr;
    WhiteTextureCube = nullptr;
    ErrorTexture = nullptr;
    ErrorTextureCube = nullptr;

    // Materials
    DefaultMaterial = nullptr;

    // Meshes
    QuadMesh = nullptr;
}
