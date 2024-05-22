#include "application.h"

#include "core/logger.h"

Application* Application::ms_Instance = nullptr;

// ------------------------------------------------------------------------------------------------------------------------------------
Application::Application(const ApplicationDescription& description)
    : m_Description(description)
{
    HEXRAY_ASSERT(!ms_Instance, "Application already created");
    ms_Instance = this;

    Logger::Initialize();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Application::Run()
{
    m_IsRunning = true;

    while (m_IsRunning)
    {

    }
}
