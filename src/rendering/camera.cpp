#include "camera.h"

#include "core/window.h"
#include "core/input.h"
#include "serialization/parsedblock.h"

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
    glm::vec2 mouseDelta = mousePos - m_LastMousePosition;
    m_LastMousePosition = mousePos;

    if (Input::IsKeyPressed(Key::LAlt))
    {
        if (Input::IsMouseButtonPressed(MouseButton::Middle))
            Pan(mouseDelta, dt);
        else if (Input::IsMouseButtonPressed(MouseButton::Left))
            Rotate(mouseDelta, dt);
        else if (Input::IsMouseButtonPressed(MouseButton::Right))
            Zoom(mouseDelta.y, dt);

        RecalculateArcballViewMatrix();
    }
    else
    {
        if (Input::IsMouseButtonPressed(MouseButton::Right)) // Unity/Unreal style move
        {
            Rotate(mouseDelta, dt);
            Move(dt);
            RecalculateDefaultViewMatrix();
        }
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
void Camera::Serialize(ParsedBlock& pb)
{
    pb.GetRequiredProperty("pos", m_Position);
    pb.GetProperty("aspectRatio", m_AspectRatio, 1e-6);
    pb.GetProperty("fov", m_PerspectiveFOV, 0.0001, 179);
    pb.GetProperty("yaw", m_YawAngle);
    pb.GetProperty("pitch", m_PitchAngle, -90, 90);
    //pb.GetProperty("roll", m_Roll);
    //pb.GetProperty("fNumber", &fNumber, 0.5, 128.0);
    //pb.GetProperty("numSamples", &numSamples, 1);
    //pb.GetProperty("focalPlaneDist", &focalPlaneDist, 1e-3, 1e+6);
    //pb.GetProperty("dof", &dof);
    //pb.GetProperty("autoFocus", &autoFocus);
    //pb.GetProperty("stereoSeparation", &stereoSeparation, 0.0);

    RecalculateProjection();
    RecalculateDefaultViewMatrix();
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::RecalculateProjection()
{
    m_ProjectionMatrix = glm::perspectiveRH_ZO(glm::radians(m_PerspectiveFOV), m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::RecalculateArcballViewMatrix()
{
    m_Position = m_FocalPoint - GetCameraFront() * m_Distance;
    glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(GetOrientation()));
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * rotationMatrix * glm::translate(glm::mat4(1.0f), -m_Position);
}

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::RecalculateDefaultViewMatrix()
{
    m_ViewMatrix = glm::lookAtRH(m_Position, m_Position + GetCameraFront(), GetCameraUp());
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

// ------------------------------------------------------------------------------------------------------------------------------------
void Camera::Move(float dt)
{
    float horizontalSpeed = Input::IsKeyPressed(Key::A) ? -1.0f : (Input::IsKeyPressed(Key::D) ? 1.0f : 0.0f);
    float verticalSpeed = Input::IsKeyPressed(Key::S) ? -1.0f : (Input::IsKeyPressed(Key::W) ? 1.0f : 0.0f);

    if (horizontalSpeed || verticalSpeed)
    {
        glm::vec3 velocity = glm::normalize(GetCameraFront() * verticalSpeed + GetCameraRight() * horizontalSpeed) * (m_MovementSpeed * dt);
        m_Position += velocity;
    }
}
