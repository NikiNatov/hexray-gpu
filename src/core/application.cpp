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

#include <sstream>

static const char* s_DefaultScenePath = "scenes/sponza/sponza_test.hexray";
Application* Application::ms_Instance = nullptr;

static void CreateScene_Bumpmap()
{
    std::filesystem::path scenePath = "scenes/bumpmap/bumpmap.hexray";
    AssetManager::Initialize(scenePath.parent_path() / "assets");

    Camera camera(60.0f, 16.0f / 9.0f, glm::vec3(45.0f, 180.0f, -240.0f), 5.0f, -20.0f);
    std::shared_ptr<Scene> scene = std::make_shared<Scene>("Bumpmap Scene", camera);
    {
        Entity pointLight = scene->CreateEntity("Sun");
        pointLight.GetComponent<TransformComponent>().Translation = { -90.0f, 1200.0f, -750.0f };

        PointLightComponent& pointLightComponent = pointLight.AddComponent<PointLightComponent>();
        pointLightComponent.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        pointLightComponent.Intensity = 1200000.0f;
        pointLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity e = scene->CreateEntity("Floor");

        TransformComponent& tc = e.GetComponent<TransformComponent>();
        tc.Translation.y = -0.01f;
        tc.Scale = { 200.0f, 1.0f, 200.0f };

        // Create plane material
        Uuid matID = AssetImporter::CreateMaterialAsset("materials/floor_diffuse.hexmat", MaterialType::Lambert, MaterialFlags::TwoSided);
        MaterialPtr mat = AssetManager::GetAsset<Material>(matID);

        TexturePtr albedoTexture = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset("data/texture/wood.bmp"));
        mat->SetTexture(MaterialTextureType::Albedo, albedoTexture);
        
        AssetSerializer::Serialize(mat->GetAssetFilepath(), mat);

        // Create plane mesh
        MeshDescription meshDesc;
        meshDesc.Submeshes = { Submesh{ 0, 4, 0, 6, 0} };
        meshDesc.MaterialTable = std::make_shared<MaterialTable>(1);
        meshDesc.MaterialTable->SetMaterial(0, mat);

        Vertex vertexData[] = {
            Vertex{ glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
            Vertex{ glm::vec3( 1.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
            Vertex{ glm::vec3( 1.0f, 0.0f,  1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
            Vertex{ glm::vec3(-1.0f, 0.0f,  1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
        };

        uint32_t indexData[] = { 0, 1, 2, 2, 3, 0 };

        Uuid meshID = AssetImporter::CreateMeshAsset("meshes/plane.hexmesh", meshDesc, vertexData, indexData);

        MeshComponent& mesh = e.AddComponent<MeshComponent>();
        mesh.Mesh = AssetManager::GetAsset<Mesh>(meshID);
    }

    {
        Entity e = scene->CreateEntity("Die");

        TransformComponent& tc = e.GetComponent<TransformComponent>();
        tc.Translation = { 0.0f, 60.0f, 0.0f };
        tc.Rotation = { 134.0f, 0.0f, 0.0f };
        tc.Scale = { 15.0f, 15.0f, 15.0f };

        // Create die material
        Uuid matID = AssetImporter::CreateMaterialAsset("materials/die_faces_diffuse.hexmat", MaterialType::Lambert);
        MaterialPtr mat = AssetManager::GetAsset<Material>(matID);

        TexturePtr albedoTexture = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset("data/texture/zar-texture.bmp"));
        mat->SetTexture(MaterialTextureType::Albedo, albedoTexture);

        TexturePtr bumpTexture = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset("data/texture/zar-bump.bmp"));
        mat->SetTexture(MaterialTextureType::Normal, bumpTexture);

        AssetSerializer::Serialize(mat->GetAssetFilepath(), mat);

        // Create die mesh
        MeshComponent& mesh = e.AddComponent<MeshComponent>();
        mesh.Mesh = AssetManager::GetAsset<Mesh>(AssetImporter::ImportMeshAsset("data/geom/truncated_cube.obj"));
        mesh.OverrideMaterialTable = std::make_shared<MaterialTable>(1);
        mesh.OverrideMaterialTable->SetMaterial(0, mat);
    }

    {
        Entity sky = scene->CreateEntity("Sky");
        sky.AddComponent<SkyLightComponent>().EnvironmentMap = AssetManager::GetAsset<Texture>(AssetImporter::ImportTextureAsset("data/env/forest/skybox.dds"));
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
    rendererDesc.RayRecursionDepth = 3;

    m_SceneRenderer = std::make_shared<Renderer>(rendererDesc);
    m_SceneRenderer->SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());

    ParseCommandlineArgs();

    //CreateScene_Bumpmap();

    if (!m_Scene)
    {
        OpenScene(s_DefaultScenePath);
    }

    SetMaxFPS(60);
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

    AssetManager::Initialize(filepath.parent_path() / "assets");

    if (!SceneSerializer::Deserialize(filepath, m_Scene))
    {
        HEXRAY_ERROR("Failed loading scene with path {}. Loading default...", filepath.string());
        AssetManager::Shutdown();
        return;
    }
}
