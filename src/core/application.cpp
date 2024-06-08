#include "application.h"

#include "core/logger.h"
#include "core/timer.h"
#include "core/input.h"
#include "scene/component.h"
#include "rendering/defaultresources.h"
#include "resourceloaders/textureloader.h"
#include "resourceloaders/meshloader.h"

#include <sstream>

Application* Application::ms_Instance = nullptr;

// ------------------------------------------------------------------------------------------------------------------------------------
Application::Application(const ApplicationDescription& description)
    : m_Description(description)
{
    HEXRAY_ASSERT(!ms_Instance, "Application already created");
    ms_Instance = this;

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

    // Create scene
    m_Scene = std::make_unique<Scene>("Test scene");

    {
        Entity dirLight = m_Scene->CreateEntity("Directional Light");
        dirLight.GetComponent<TransformComponent>().Translation = { 1.0f, 1.0f, 1.0f };

        DirectionalLightComponent& dirLightComponent = dirLight.AddComponent<DirectionalLightComponent>();
        dirLightComponent.Color = glm::vec3(0.6f, 0.4f, 0.0f);
        dirLightComponent.Intensity = 0.5f;
    }

    {
        Entity pointLight = m_Scene->CreateEntity("Red Point Light");
        pointLight.GetComponent<TransformComponent>().Translation.y = 4.0f;
        pointLight.GetComponent<TransformComponent>().Translation.z = -8.0f;

        PointLightComponent& pointLightComponent = pointLight.AddComponent<PointLightComponent>();
        pointLightComponent.Color = glm::vec3(1.0f, 0.0f, 0.0f);
        pointLightComponent.Intensity = 10.0f;
        pointLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity pointLight = m_Scene->CreateEntity("Blue Point Light");
        pointLight.GetComponent<TransformComponent>().Translation.x = 8.0f;
        pointLight.GetComponent<TransformComponent>().Translation.y = 4.0f;

        PointLightComponent& pointLightComponent = pointLight.AddComponent<PointLightComponent>();
        pointLightComponent.Color = glm::vec3(0.0f, 0.0f, 1.0f);
        pointLightComponent.Intensity = 10.0f;
        pointLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity pointLight = m_Scene->CreateEntity("Yellow Point Light");
        pointLight.GetComponent<TransformComponent>().Translation.z = 8.0f;
        pointLight.GetComponent<TransformComponent>().Translation.y = 4.0f;

        PointLightComponent& pointLightComponent = pointLight.AddComponent<PointLightComponent>();
        pointLightComponent.Color = glm::vec3(1.0f, 1.0f, 0.0f);
        pointLightComponent.Intensity = 10.0f;
        pointLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity pointLight = m_Scene->CreateEntity("Green Point Light");
        pointLight.GetComponent<TransformComponent>().Translation.x = -8.0f;
        pointLight.GetComponent<TransformComponent>().Translation.y = 4.0f;

        PointLightComponent& pointLightComponent = pointLight.AddComponent<PointLightComponent>();
        pointLightComponent.Color = glm::vec3(0.0f, 1.0f, 0.0f);
        pointLightComponent.Intensity = 10.0f;
        pointLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity pointLight = m_Scene->CreateEntity("Pink Point Light");
        pointLight.GetComponent<TransformComponent>().Translation.y = 4.0f;

        PointLightComponent& pointLightComponent = pointLight.AddComponent<PointLightComponent>();
        pointLightComponent.Color = glm::vec3(1.0f, 0.0f, 1.0f);
        pointLightComponent.Intensity = 10.0f;
        pointLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity spotLight = m_Scene->CreateEntity("Spot Light");
        spotLight.GetComponent<TransformComponent>().Translation.y = 4.0f;
        spotLight.GetComponent<TransformComponent>().Translation.x = 12.0f;
        spotLight.GetComponent<TransformComponent>().Translation.z = 4.0f;

        SpotLightComponent& spotLightComponent = spotLight.AddComponent<SpotLightComponent>();
        spotLightComponent.Color = glm::vec3(0.0f, 1.0f, 1.0f);
        spotLightComponent.Direction = glm::normalize(glm::vec3(0.0f, -0.5f, -1.0f));
        spotLightComponent.Intensity = 30.0f;
        spotLightComponent.AttenuationFactors = { 0.5f, 0.5f, 0.5f };
    }

    {
        Entity sponza = m_Scene->CreateEntity("Sponza");

        TransformComponent& sponzaTransform = sponza.GetComponent<TransformComponent>();
        sponzaTransform.Scale = { 0.05f, 0.05f, 0.05f };

        MeshComponent& sponzaMesh = sponza.AddComponent<MeshComponent>();
        sponzaMesh.MeshObject = MeshLoader::LoadFromFile("data/meshes/Sponza/Sponza.fbx");
    }

    {
        Entity sky = m_Scene->CreateEntity("Sky");
        sky.AddComponent<SkyLightComponent>().EnvironmentMap = TextureLoader::LoadFromFile("data/textures/Skybox.dds");
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
Application::~Application()
{
    DefaultResources::Shutdown();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::Run()
{
    m_IsRunning = true;

    std::string newTitleStart = m_Window->GetTitle();

    Timer frameTimer;
    while (m_IsRunning)
    {
        frameTimer.Reset();

        m_Window->ProcessEvents();

        m_Scene->OnUpdate(frameTimer.GetElapsedTime());

        m_GraphicsContext->BeginFrame();

        m_Scene->OnRender(m_SceneRenderer);
        m_GraphicsContext->CopyTextureToSwapChain(m_SceneRenderer->GetFinalImage().get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_GraphicsContext->EndFrame();

        frameTimer.Stop();

        std::stringstream newTitle;
        newTitle << newTitleStart;
        newTitle << " ";
        newTitle << std::fixed << std::setprecision(0) << std::ceil(1.0f / frameTimer.GetElapsedTime());
        newTitle << "FPS";
        m_Window->SetTitle(newTitle.str());
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