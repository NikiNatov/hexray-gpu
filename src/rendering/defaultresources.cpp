#include "defaultresources.h"

#include "rendering/graphicscontext.h"

std::shared_ptr<Texture> DefaultResources::BlackTexture = nullptr;
std::shared_ptr<Texture> DefaultResources::BlackTextureCube = nullptr;
std::shared_ptr<Texture> DefaultResources::WhiteTexture = nullptr;
std::shared_ptr<Texture> DefaultResources::WhiteTextureCube = nullptr;
std::shared_ptr<Texture> DefaultResources::ErrorTexture = nullptr;
std::shared_ptr<Texture> DefaultResources::ErrorTextureCube = nullptr;
std::shared_ptr<Material> DefaultResources::DefaultMaterial = nullptr;
std::shared_ptr<Material> DefaultResources::ErrorMaterial = nullptr;
std::shared_ptr<Mesh> DefaultResources::TriangleMesh = nullptr;
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

        ErrorTexture = std::make_shared<Texture>(errorTextureDesc, L"DefaultResources::ErrorTexture");

        errorTextureDesc.IsCubeMap = true;

        ErrorTextureCube = std::make_shared<Texture>(errorTextureDesc, L"DefaultResources::ErrorTextureCube");

        const uint8_t errorTextureData[] = { 0xFF, 0x00, 0xFF, 0xFF };
        GraphicsContext::GetInstance()->UploadTextureData(ErrorTexture.get(), errorTextureData);

        for (uint32_t face = 0; face < 6; face++)
        {
            GraphicsContext::GetInstance()->UploadTextureData(ErrorTextureCube.get(), errorTextureData, 0, face);
        }
    }

    // Black textures
    {
        TextureDescription blackTextureDesc;
        blackTextureDesc.Width = 1;
        blackTextureDesc.Height = 1;
        blackTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        blackTextureDesc.MipLevels = 1;
        blackTextureDesc.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;

        BlackTexture = std::make_shared<Texture>(blackTextureDesc, L"DefaultResources::BlackTexture");

        blackTextureDesc.IsCubeMap = true;

        BlackTextureCube = std::make_shared<Texture>(blackTextureDesc, L"DefaultResources::BlackTextureCube");

        const byte blackTextureData[] = { 0x00, 0x00, 0x00, 0xFF };
        GraphicsContext::GetInstance()->UploadTextureData(BlackTexture.get(), blackTextureData);

        for (uint32_t face = 0; face < 6; face++)
        {
            GraphicsContext::GetInstance()->UploadTextureData(BlackTextureCube.get(), blackTextureData, 0, face);
        }
    }

    // White textures
    {
        TextureDescription whiteTextureDesc;
        whiteTextureDesc.Width = 1;
        whiteTextureDesc.Height = 1;
        whiteTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        whiteTextureDesc.MipLevels = 1;
        whiteTextureDesc.InitialState = D3D12_RESOURCE_STATE_COPY_DEST;

        WhiteTexture = std::make_shared<Texture>(whiteTextureDesc, L"DefaultResources::WhiteTexture");

        whiteTextureDesc.IsCubeMap = true;

        WhiteTextureCube = std::make_shared<Texture>(whiteTextureDesc, L"DefaultResources::WhiteTextureCube");

        const byte whiteTextureData[] = { 0xFF, 0xFF, 0xFF, 0xFF };
        GraphicsContext::GetInstance()->UploadTextureData(WhiteTexture.get(), whiteTextureData);

        for (uint32_t face = 0; face < 6; face++)
        {
            GraphicsContext::GetInstance()->UploadTextureData(WhiteTextureCube.get(), whiteTextureData, 0, face);
        }
    }

    // Default material
    DefaultMaterial = std::make_shared<Material>(MaterialFlags::TwoSided);
    DefaultMaterial->SetAlbedoColor(glm::vec3(1.0f, 1.0f, 1.0f));
    DefaultMaterial->SetMetalness(0.5f);
    DefaultMaterial->SetRoughness(0.5f);

    ErrorMaterial = std::make_shared<Material>(MaterialFlags::TwoSided);
    ErrorMaterial->SetAlbedoColor(glm::vec3(1.0f, 0.0f, 1.0f));
    ErrorMaterial->SetMetalness(0.0f);
    ErrorMaterial->SetRoughness(1.0f);

    // Quad mesh
    {
        MeshDescription quadMeshDesc;
        quadMeshDesc.Indices = { 0, 1, 2, 2, 3, 0 };
        quadMeshDesc.Positions = {
            { -1.0f, -1.0f, 0.0f },
            {  1.0f, -1.0f, 0.0f },
            {  1.0f,  1.0f, 0.0f },
            { -1.0f,  1.0f, 0.0f }
        };
        quadMeshDesc.UVs = {
            { 0.0f, 1.0f },
            { 1.0f, 1.0f },
            { 1.0f, 0.0f },
            { 0.0f, 0.0f }
        };
        quadMeshDesc.Submeshes = { SubmeshDescription{ 0, 4, 0, 6 } };
        quadMeshDesc.Materials = { DefaultMaterial };

        QuadMesh = std::make_shared<Mesh>(quadMeshDesc);
    }

    // Triangle mesh
    {
        MeshDescription triMeshDesc;
        triMeshDesc.Indices = { 0, 1, 2 };
        triMeshDesc.Positions = {
            {  0.0f,  1.0f, 0.0f },
            { -1.0f, -1.0f, 0.0f },
            {  1.0f, -1.0f, 0.0f }
        };
        triMeshDesc.UVs = {
            { 0.5f, 0.0f },
            { 0.0f, 1.0f },
            { 1.0f, 0.0f },
        };
        triMeshDesc.Submeshes = { SubmeshDescription{ 0, 3, 0, 3 } };
        triMeshDesc.Materials = { DefaultMaterial };

        TriangleMesh = std::make_shared<Mesh>(triMeshDesc);
    }
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
    ErrorMaterial = nullptr;

    // Meshes
    QuadMesh = nullptr;
    TriangleMesh = nullptr;
}
