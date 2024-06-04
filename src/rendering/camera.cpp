#include "camera.h"

#include "core/window.h"
#include "core/input.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>

// ------------------------------------------------------------------------------------------------------------------------------------
Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_PerspectiveFOV(fov), m_AspectRatio(aspectRatio), m_PerspectiveNear(nearPlane), m_PerspectiveFar(farPlane)
{
    RecalculateProjection();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::OnUpdate(float dt)
{
    const glm::vec2 mousePos = Input::GetMousePosition();
    glm::vec2 delta = mousePos - m_LastMousePosition;
    m_LastMousePosition = mousePos;

    if (Input::IsKeyPressed(Key::LAlt))
    {
        if (Input::IsMouseButtonPressed(MouseButton::Middle))
            Pan(delta, dt);
        else if (Input::IsMouseButtonPressed(MouseButton::Left))
            Rotate(delta, dt);
        else if (Input::IsMouseButtonPressed(MouseButton::Right))
            Zoom(delta.y, dt);
    }

    RecalculateViewMatrix();
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
    return glm::normalize(GetOrientation() * glm::vec3(0.0f, 0.0f, -1.0f));
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::SetPerspective(float fov, float nearPlane, float farPlane)
{
    m_PerspectiveFOV = fov;
    m_PerspectiveNear = nearPlane;
    m_PerspectiveFar = farPlane;

    RecalculateProjection();
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
    m_ProjectionMatrix = glm::perspectiveRH_ZO(glm::radians(m_PerspectiveFOV), m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::RecalculateViewMatrix()
{
    m_Position = m_FocalPoint - GetCameraFront() * m_Distance;
    glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(GetOrientation()));
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * rotationMatrix * glm::translate(glm::mat4(1.0f), -m_Position);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::Pan(const glm::vec2& mouseDelta, float dt)
{
    m_FocalPoint += -GetCameraRight() * mouseDelta.x * m_PanSpeed * dt;
    m_FocalPoint += GetCameraUp() * mouseDelta.y * m_PanSpeed * dt;
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::Rotate(const glm::vec2& mouseDelta, float dt)
{
    float sign = GetCameraUp().y < 0.0f ? -1.0f : 1.0f;
    m_YawAngle += sign * mouseDelta.x * m_RotationSpeed * dt;
    m_PitchAngle += mouseDelta.y * m_RotationSpeed * dt;

    if (m_PitchAngle > 90.0f)
    {
        m_PitchAngle = 90.0f;
    }
    else if (m_PitchAngle < -90.0f)
    {
        m_PitchAngle = -90.0f;
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::Zoom(float zoomDelta, float dt)
{
    m_Distance -= zoomDelta * m_ZoomSpeed * dt;

    if (m_Distance < 1.0f)
        m_Distance = 1.0f;
}
