#include "camera.h"

#include <gtc\matrix_transform.hpp>

// ------------------------------------------------------------------------------------------------------------------------------------
Camera::Camera()
{
    RecalculateProjection();
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
