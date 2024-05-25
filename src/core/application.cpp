#include "application.h"

#include "core/logger.h"
#include "core/timer.h"

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

    // Create d3d12 graphics context
    GraphicsContextDescription gfxContextDesc;
    gfxContextDesc.Window = m_Window.get();
    gfxContextDesc.EnableDebugLayer = m_Description.EnableAPIValidation;

    m_GraphicsContext = std::make_unique<GraphicsContext>(gfxContextDesc);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::Run()
{
    m_IsRunning = true;

    Timer frameTimer;
    while (m_IsRunning)
    {
        frameTimer.Reset();

        m_Window->ProcessEvents();

        m_GraphicsContext->BeginFrame();
        m_GraphicsContext->EndFrame();

        frameTimer.Stop();

        HEXRAY_INFO("FPS {}", 1.0f / frameTimer.GetElapsedTime());
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