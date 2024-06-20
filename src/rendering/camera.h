#pragma once

#include "core/core.h"

#include <glm.hpp>

class Camera
{
public:
	Camera(float fov, float aspectRatio, const glm::vec3& position, float yaw, float pitch);

	void OnUpdate(float dt);

	glm::quat GetOrientation() const;
	glm::vec3 GetCameraUp() const;
	glm::vec3 GetCameraRight() const;
	glm::vec3 GetCameraFront() const;

	void SetViewportSize(uint32_t width, uint32_t height);

	inline void SetPerspectiveFOV(float fov) { m_PerspectiveFOV = fov; RecalculateProjection(); }

	inline float GetPerspectiveFOV() const { return m_PerspectiveFOV; }
	inline float GetYawAngle() const { return m_YawAngle; }
	inline float GetPitchAngle() const { return m_PitchAngle; }
	inline const glm::vec3& GetPosition() const { return m_Position; }
	inline const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }
	inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
private:
	void RecalculateProjection();
	void RecalculateView();
private:
	glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
	float m_AspectRatio = 16.0f / 9.0f;
	float m_PerspectiveFOV = 45.0f;
	float m_YawAngle = 0.0f;
	float m_PitchAngle = 30.0f;
	float m_RotationSpeed = 30.0f;
	float m_MovementSpeed = 1130.0f;
	glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec2 m_LastMousePosition = glm::vec2(0.0f);
};