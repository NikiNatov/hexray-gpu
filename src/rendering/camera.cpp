#include "camera.h"

#include "core/window.h"
#include "core/input.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>

// ------------------------------------------------------------------------------------------------------------------------------------
Camera::Camera(float fov, float aspectRatio, const glm::vec3& position, float yaw, float pitch)
    : m_PerspectiveFOV(fov), m_AspectRatio(aspectRatio), m_Position(position), m_YawAngle(yaw), m_PitchAngle(pitch)
{
    RecalculateProjection();
    RecalculateView();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::OnUpdate(float dt)
{
    m_HasMoved = false;

    const glm::vec2 mousePos = Input::GetMousePosition();
    glm::vec2 mouseDelta = mousePos - m_LastMousePosition;
    m_LastMousePosition = mousePos;

    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Movement
    if (Input::IsKeyPressed(Key::W))
    {
        m_Position += GetCameraFront() * m_MovementSpeed * dt;
        m_HasMoved = true;
    }
    else if (Input::IsKeyPressed(Key::S))
    {
        m_Position -= GetCameraFront() * m_MovementSpeed * dt;
        m_HasMoved = true;
    }
    if (Input::IsKeyPressed(Key::A))
    {
        m_Position -= GetCameraRight() * m_MovementSpeed * dt;
        m_HasMoved = true;
    }
    else if (Input::IsKeyPressed(Key::D))
    {
        m_Position += GetCameraRight() * m_MovementSpeed * dt;
        m_HasMoved = true;
    }
    if (Input::IsKeyPressed(Key::LShift))
    {
        m_Position -= upVector * m_MovementSpeed * dt;
        m_HasMoved = true;
    }
    else if (Input::IsKeyPressed(Key::Space))
    {
        m_Position += upVector * m_MovementSpeed * dt;
        m_HasMoved = true;
    }

    // Rotation
    if (Input::IsMouseButtonPressed(MouseButton::Right) && (mouseDelta.x || mouseDelta.y))
    {
        m_YawAngle -= mouseDelta.x * m_RotationSpeed * dt;
        m_PitchAngle -= mouseDelta.y * m_RotationSpeed * dt;

        m_PitchAngle = std::clamp(m_PitchAngle, -89.0f, 89.0f);

        m_HasMoved = true;
    }

    if (m_HasMoved)
    {
        RecalculateView();
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
glm::quat Camera::GetOrientation() const
{
    return glm::normalize(glm::angleAxis(glm::radians(-m_YawAngle), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::angleAxis(glm::radians(-m_PitchAngle), glm::vec3(1.0f, 0.0f, 0.0f)));
}

// ------------------------------------------------------------------------------------------------------------------------------------
glm::vec3 Camera::GetCameraUp() const
{
    return glm::normalize(GetOrientation() * glm::vec3(0.0f, 1.0f, 0.0f));
}

// ------------------------------------------------------------------------------------------------------------------------------------
glm::vec3 Camera::GetCameraRight() const
{
    return glm::normalize(GetOrientation() * glm::vec3(1.0f, 0.0f, 0.0f));
}

// ------------------------------------------------------------------------------------------------------------------------------------
glm::vec3 Camera::GetCameraFront() const
{
    return glm::normalize(GetOrientation() * glm::vec3(0.0f, 0.0f, 1.0f));
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::SetViewportSize(uint32_t width, uint32_t height)
{
    if (height == 0)
        return;

    m_AspectRatio = (float)width / (float)height;
    RecalculateProjection();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::RecalculateProjection()
{
    m_ProjectionMatrix = glm::perspectiveLH_ZO(glm::radians(m_PerspectiveFOV), m_AspectRatio, 0.1f, 1000.0f);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::RecalculateView()
{
    m_ViewMatrix = glm::lookAtLH(m_Position, m_Position + GetCameraFront(), glm::vec3(0.0f, 1.0f, 0.0f));
}
