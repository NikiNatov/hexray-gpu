#include "application.h"

#include "core/logger.h"
#include "core/timer.h"
#include "core/input.h"
#include "scene/component.h"
#include "rendering/defaultresources.h"
#include "resourceloaders/textureloader.h"

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

    static auto cube = TextureLoader::LoadFromFile("data/textures/skybox.dds");
    Entity quad = m_Scene->CreateEntity("Quad");
    {
        std::shared_ptr<Material> japaneseDiffuseOnly = std::make_shared<Material>(MaterialFlags::TwoSided);
        japaneseDiffuseOnly->SetAlbedoColor(glm::vec3(0.8f, 0.4f, 0.2f));
        japaneseDiffuseOnly->SetAlbedoMap(TextureLoader::LoadFromFile("data/textures/japanese.png"));
        
        MeshComponent& quadMesh = quad.AddComponent<MeshComponent>();
        quadMesh.MeshObject = DefaultResources::QuadMesh;
        quadMesh.MeshObject->SetMaterial(0, japaneseDiffuseOnly);
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