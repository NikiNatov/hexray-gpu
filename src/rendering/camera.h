#pragma once

#include "core/core.h"

#include <glm.hpp>

class Camera
{
public:
	Camera(float fov = 45.0f, float aspectRatio = 16.0f / 9.0f, float nearPlane = 0.1f, float farPlane = 1000.0f);

	void OnUpdate(float dt);

	glm::quat GetOrientation() const;
	glm::vec3 GetCameraUp() const;
	glm::vec3 GetCameraRight() const;
	glm::vec3 GetCameraFront() const;

	void SetViewportSize(uint32_t width, uint32_t height);
	void SetPerspective(float fov, float nearPlane, float farPlane);

	inline void SetPerspectiveFOV(float fov) { m_PerspectiveFOV = fov; RecalculateProjection(); }
	inline void SetPerspectiveNear(float value) { m_PerspectiveNear = value; RecalculateProjection(); }
	inline void SetPerspectiveFar(float value) { m_PerspectiveFar = value; RecalculateProjection(); }
	inline void SetDistance(float distance) { m_Distance = distance; }

	inline float GetPerspectiveFOV() const { return m_PerspectiveFOV; }
	inline float GetPerspectiveNear() const { return m_PerspectiveNear; }
	inline float GetPerspectiveFar() const { return m_PerspectiveFar; }
	inline float GetDistance() const { return m_Distance; }
	inline float GetYawAngle() const { return m_YawAngle; }
	inline float GetPitchAngle() const { return m_PitchAngle; }
	inline const glm::vec3& GetPosition() const { return m_Position; }
	inline const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }
	inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
private:
	void RecalculateProjection();
	void RecalculateViewMatrix();
	void Pan(const glm::vec2& mouseDelta, float dt);
	void Rotate(const glm::vec2& mouseDelta, float dt);
	void Zoom(float zoomDelta, float dt);
private:
	glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
	glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
	float m_AspectRatio = 16.0f / 9.0f;
	float m_PerspectiveFOV = 45.0f;
	float m_PerspectiveNear = 0.1f;
	float m_PerspectiveFar = 1000.0f;
	float m_Distance = 0.0f;
	float m_YawAngle = 0.0f;
	float m_PitchAngle = 0.0f;
	float m_PanSpeed = 1.0f;
	float m_RotationSpeed = 30.0f;
	float m_ZoomSpeed = 3.0f;
	glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 m_FocalPoint = glm::vec3(0.0f);
	glm::vec2 m_LastMousePosition = glm::vec2(0.0f);
};