#include "application.h"

#include "core/logger.h"
#include "core/timer.h"
#include "core/input.h"
#include "scene/component.h"
#include "scene/sceneserializer.h"
#include "rendering/defaultresources.h"
#include "asset/assetimporter.h"
#include "asset/assetmanager.h"
#include "asset/assetserializer.h"
#include "serialization/defaultsceneparser.h"

#include <sstream>

static const char* s_DefaultScenePath = "scenes/pbr_spheres/pbr_spheres.hexray";
Application* Application::ms_Instance = nullptr;

void CreatePBRTestScene()
{
    std::filesystem::path scenePath = "scenes/pbr_test/pbr_test.hexray";

    AssetManager::Initialize(scenePath.parent_path() / "assets");
    auto scene = std::make_shared<Scene>(scenePath.stem().string(), Camera(60.0f, 16.0f / 9.0f, glm::vec3(5.0f, 5.0f, -5.0f), 45.0f, -25.0f, 0.5f));
    {
        Entity e = scene->CreateEntity("SkyBox");
        e.AddComponent<SkyLightComponent>().EnvironmentMap = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset("data/texture/Skybox.dds"));
    }

    {
        Entity e = scene->CreateEntity("Sun");
        e.GetComponent<TransformComponent>().Translation = { 1.0f, 3.0f, 2.0f };
        auto& c = e.AddComponent<DirectionalLightComponent>();
        c.Color = { 0.8f, 0.5f, 0.1f };
        c.Intensity = 1.0f;
    }

    {
       
        Entity e = scene->CreateEntity("Pistol");
        e.GetComponent<TransformComponent>().Scale = { 0.2f, 0.2f, 0.2f };
        auto& mc = e.AddComponent<MeshComponent>();
        mc.Mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/gun/gun.fbx"));
    }

    SceneSerializer::Serialize(scenePath, scene);
}

void CreatePBRSpheresScene()
{
    std::filesystem::path scenePath = "scenes/pbr_spheres/pbr_spheres.hexray";

    AssetManager::Initialize(scenePath.parent_path() / "assets");
    auto scene = std::make_shared<Scene>(scenePath.stem().string(), Camera(60.0f, 16.0f / 9.0f, glm::vec3(5.0f, 5.0f, -5.0f), 45.0f, -25.0f, 0.5f));
    {
        Entity e = scene->CreateEntity("SkyBox");
        e.AddComponent<SkyLightComponent>().EnvironmentMap = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset("data/texture/Skybox.dds"));
    }

    {
        //Entity e = scene->CreateEntity("Sun");
        //e.GetComponent<TransformComponent>().Translation = { 1.0f, 3.0f, 2.0f };
        //auto& c = e.AddComponent<DirectionalLightComponent>();
        //c.Color = { 1.0f, 1.0f, 1.0f };
        //c.Intensity = 1.0f;
    }

    std::vector<std::string> materialPaths = {
        "materials/laminate.hexmat",
        "materials/marble.hexmat",
        "materials/sand.hexmat",
        "materials/worn_metal.hexmat",
    };

    std::vector<std::string> materialTextures = {
        "data/texture/pbr/laminate/laminate_albedo.png",
        "data/texture/pbr/laminate/laminate_normal.png",
        "data/texture/pbr/laminate/laminate_roughness.png",
        "data/texture/pbr/laminate/laminate_metalness.png",

        "data/texture/pbr/marble/marble_albedo.png",
        "data/texture/pbr/marble/marble_normal.png",
        "data/texture/pbr/marble/marble_roughness.png",
        "data/texture/pbr/marble/marble_metalness.png",

        "data/texture/pbr/sand/sand_albedo.png",
        "data/texture/pbr/sand/sand_normal.png",
        "data/texture/pbr/sand/sand_roughness.png",
        "data/texture/pbr/sand/sand_metalness.png",

        "data/texture/pbr/worn-metal/worn_metal_albedo.png",
        "data/texture/pbr/worn-metal/worn_metal_normal.png",
        "data/texture/pbr/worn-metal/worn_metal_roughness.png",
        "data/texture/pbr/worn-metal/worn_metal_metalness.png",
    };

    TextureImportOptions importOptions;
    importOptions.Compress = false;
    importOptions.GenerateMips = true;
    for (uint32_t i = 0; i < materialTextures.size() / 4; i++)
    {
        MaterialPtr material = AssetManager::GetAsset<Material>(AssetImporter::CreateMaterialAsset(materialPaths[i], MaterialType::PBR));
        material->SetTexture(MaterialTextureType::Albedo, AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset(materialTextures[i * 4 + 0], importOptions)));
        material->SetTexture(MaterialTextureType::Normal, AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset(materialTextures[i * 4 + 1], importOptions)));
        material->SetTexture(MaterialTextureType::Roughness, AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset(materialTextures[i * 4 + 2], importOptions)));
        material->SetTexture(MaterialTextureType::Metalness, AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset(materialTextures[i * 4 + 3], importOptions)));

        AssetSerializer::Serialize(material->GetAssetFilepath(), material);

        Entity e = scene->CreateEntity("Sphere");
        e.GetComponent<TransformComponent>().Translation = { i * 2.4f, 0.0f, 0.0f };
        e.GetComponent<TransformComponent>().Scale = { 2.0f, 2.0f, 2.0f };

        auto& mc = e.AddComponent<MeshComponent>();
        mc.Mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/sphere.fbx"));
        mc.OverrideMaterialTable = std::make_shared<MaterialTable>(1);
        mc.OverrideMaterialTable->SetMaterial(0, material);
    }

    {
        MaterialPtr material = AssetManager::GetAsset<Material>(AssetImporter::CreateMaterialAsset("materials/material_big.hexmat", MaterialType::PBR));
        material->SetProperty(MaterialPropertyType::AlbedoColor, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material->SetProperty(MaterialPropertyType::Roughness, 0.0f);
        material->SetProperty(MaterialPropertyType::Metalness, 1.0f);
        material->SetProperty(MaterialPropertyType::EmissiveColor, glm::vec4(0.8f, 0.5f, 0.1f, 1.0f));
        material->SetProperty(MaterialPropertyType::EmissionPower, 16.0f);

        AssetSerializer::Serialize(material->GetAssetFilepath(), material);

        Entity e = scene->CreateEntity("Sphere");
        e.GetComponent<TransformComponent>().Translation = { 0.0f, 0.0f, 15.0f };
        e.GetComponent<TransformComponent>().Scale = { 20.0f, 20.0f, 20.0f };

        auto& mc = e.AddComponent<MeshComponent>();
        mc.Mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/sphere.fbx"));
        mc.OverrideMaterialTable = std::make_shared<MaterialTable>(1);
        mc.OverrideMaterialTable->SetMaterial(0, material);
    }

    {
        MaterialPtr material = AssetManager::GetAsset<Material>(AssetImporter::CreateMaterialAsset("materials/material_floor.hexmat", MaterialType::PBR));
        material->SetProperty(MaterialPropertyType::AlbedoColor, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        material->SetProperty(MaterialPropertyType::Roughness, 1.0f);
        material->SetProperty(MaterialPropertyType::Metalness, 0.0f);

        AssetSerializer::Serialize(material->GetAssetFilepath(), material);

        Entity e = scene->CreateEntity("Floor");
        e.GetComponent<TransformComponent>().Translation = { 0.0f, -1.0f, 0.0f };
        e.GetComponent<TransformComponent>().Scale = { 100.0f, 0.1f, 100.0f };

        auto& mc = e.AddComponent<MeshComponent>();
        mc.Mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/meshes/cube.obj"));
        mc.OverrideMaterialTable = std::make_shared<MaterialTable>(1);
        mc.OverrideMaterialTable->SetMaterial(0, material);
    }

    SceneSerializer::Serialize(scenePath, scene);
}

// ------------------------------------------------------------------------------------------------------------------------------------
Application::Application(const ApplicationDescription& description)
    : m_Description(description)
    , m_LastTime(0.0)
    , m_DeltaTime(0.0)
    , m_CurrentTime(0.0)
    , m_RealTime(0.0)
    , m_FixedDeltaTime(0.0)
    , m_MaxFPS(0.0)
    , m_MaxDeltaTime(0.0)
    , m_AvgDeltaTimeMS(0.0)
{
    HEXRAY_ASSERT(!ms_Instance, "Application already created");
    ms_Instance = this;

    // Set exe path
    char exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    m_ExecutablePath = exePath;

    Logger::Initialize();

    // Create window
    WindowDescription windowDesc;
    windowDesc.Title = m_Description.Name;
    windowDesc.Width = m_Description.WindowWidth;
    windowDesc.Height = m_Description.WindowHeight;
    windowDesc.VSync = m_Description.VSync;
    windowDesc.EventCallback = HEXRAY_BIND_FN(OnEvent);

    m_Window = std::make_unique<Window>(windowDesc);

    Input::Initialize(m_Window->GetWindowHandle());

    // Create d3d12 graphics context
    GraphicsContextDescription gfxContextDesc;
    gfxContextDesc.Window = m_Window.get();
    gfxContextDesc.EnableDebugLayer = m_Description.EnableAPIValidation;

    m_GraphicsContext = std::make_unique<GraphicsContext>(gfxContextDesc);

    DefaultResources::Initialize();

    // Create renderer
    RendererDescription rendererDesc;
    rendererDesc.RayRecursionDepth = 8;
    rendererDesc.EnableBloom = true;
    rendererDesc.BloomDownsampleSteps = 8;
    rendererDesc.BloomStrength = 0.06f;

    m_SceneRenderer = std::make_shared<Renderer>(rendererDesc);
    m_SceneRenderer->SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());

    //CreatePBRSpheresScene();
    ParseCommandlineArgs();

    if (!m_Scene)
    {
        OpenScene(s_DefaultScenePath);
    }

    SetMaxFPS(120);
    SetMaxDeltaTime(2.0);
}

// ------------------------------------------------------------------------------------------------------------------------------------
Application::~Application()
{
    AssetManager::Shutdown();
    DefaultResources::Shutdown();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::Run()
{
    m_IsRunning = true;

    m_Timer.Reset();

    m_LastTime = m_Timer.GetTimeNow();

    while (m_IsRunning)
    {
        BeginFrame();

        Update();

        EndFrame();
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowClosedEvent>(HEXRAY_BIND_FN(OnWindowClosed));
    dispatcher.Dispatch<WindowResizedEvent>(HEXRAY_BIND_FN(OnWindowResized));
    dispatcher.Dispatch<KeyPressedEvent>(HEXRAY_BIND_FN(OnKeyPressed));
    dispatcher.Dispatch<MouseButtonPressedEvent>(HEXRAY_BIND_FN(OnMouseButtonPressed));
    dispatcher.Dispatch<MouseScrolledEvent>(HEXRAY_BIND_FN(OnMouseScrolledPressed));
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Application::OnWindowClosed(WindowClosedEvent& event)
{
    m_IsRunning = false;
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Application::OnWindowResized(WindowResizedEvent& event)
{
    if (m_Window && m_Window->IsMinimized())
    {
        // We don't want to resize anything if the window is minimized since its dimensions become 0 which causes a crash
        // in the swapchain
        return true;
    }

    if(m_GraphicsContext)
        m_GraphicsContext->ResizeSwapChain(event.GetWidth(), event.GetHeight());

    if (m_Scene)
        m_Scene->OnViewportResize(event.GetWidth(), event.GetHeight());

    if (m_SceneRenderer)
        m_SceneRenderer->SetViewportSize(event.GetWidth(), event.GetHeight());

    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Application::OnKeyPressed(KeyPressedEvent& event)
{
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Application::OnMouseButtonPressed(MouseButtonPressedEvent& event)
{
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool Application::OnMouseScrolledPressed(MouseScrolledEvent& event)
{
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::BeginFrame()
{
    double timeNow = m_Timer.GetTimeNow();
    double realTimeDelta = timeNow - m_LastTime;

    bool isUsingFixedDeltaTime = m_FixedDeltaTime != 0.0f;
    m_DeltaTime = isUsingFixedDeltaTime ? m_FixedDeltaTime : realTimeDelta;
    m_DeltaTime = std::min(m_DeltaTime, m_MaxDeltaTime);

    m_LastTime = timeNow;
    m_CurrentTime += m_DeltaTime;
    m_RealTime += realTimeDelta;

    double deltaTimeMS = (m_DeltaTime * 1000);
    m_AvgDeltaTimeMS = m_AvgDeltaTimeMS + (deltaTimeMS - m_AvgDeltaTimeMS) * 0.50;


    m_Window->ProcessEvents();

    m_GraphicsContext->BeginFrame();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::Update()
{
    m_Scene->OnUpdate(m_DeltaTime);
    m_Scene->OnRender(m_SceneRenderer);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::EndFrame()
{
    m_GraphicsContext->CopyTextureToSwapChain(m_SceneRenderer->GetFinalImage().get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_GraphicsContext->EndFrame();


    double frameEnd = m_Timer.GetTimeNow();
    double frameDuration = (frameEnd - m_LastTime);
    double frameDurationMS = frameDuration * 1000;

    if (m_MaxFPS > 0)
    {
        double frameDelayMS = 1000. / m_MaxFPS;
        if (frameDurationMS < frameDelayMS)
        {
            uint32_t millis = uint32_t(frameDelayMS - frameDurationMS);
#if defined(WIN32)
            // std::this_thread::sleep_for is not precise
            static thread_local HANDLE timer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            LARGE_INTEGER dueTime;
            dueTime.QuadPart = -int64_t(millis) * 10000;

            SetWaitableTimerEx(timer, &dueTime, 0, NULL, NULL, NULL, 0);
            WaitForSingleObject(timer, INFINITE);
#endif
        }
    }

    std::stringstream newTitle;
    newTitle << m_Description.Name;
    newTitle << " ";
    newTitle << std::fixed << std::setprecision(1) << (1000. / m_AvgDeltaTimeMS);
    newTitle << "FPS (";
    newTitle << std::fixed << std::setprecision(1) << m_AvgDeltaTimeMS;
    newTitle << " ms)";
    m_Window->SetTitle(newTitle.str());
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::ParseCommandlineArgs()
{
    const CommandLineArgs& args = m_Description.CommandLineArgs;
    for (int32_t i = 1; i < args.Count; i++)
    {
        if (strcmp(args[i], "-scene") == 0)
        {
            OpenScene(args[++i]);
        }
        else if (strcmp(args[i], "-hexray-scene") == 0)
        {
            std::filesystem::path filepath = args[++i];
            AssetManager::Initialize(filepath.parent_path() / "assets");
            m_Scene = std::make_shared<Scene>(filepath.string());
            DefaultSceneParser parser;
            parser.Parse(filepath.string().c_str(), m_Scene.get());
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::OpenScene(const std::filesystem::path& filepath)
{
    bool loadDefaultScene = false;

    if (!std::filesystem::exists(filepath))
    {
        HEXRAY_ERROR("Scene with path {} doesn't exist. Loading default...", filepath.string());
        return;
    }

    double startTime = m_Timer.GetTimeNow();

    AssetManager::Initialize(filepath.parent_path() / "assets");

    if (!SceneSerializer::Deserialize(filepath, m_Scene))
    {
        HEXRAY_ERROR("Failed loading scene with path {}. Loading default...", filepath.string());
        AssetManager::Shutdown();
        return;
    }

    double endTime = m_Timer.GetTimeNow();
    HEXRAY_INFO("Scene load took {}ms", endTime - startTime);
}
