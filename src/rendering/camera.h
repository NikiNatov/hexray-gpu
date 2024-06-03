#pragma once

#include "core/core.h"

#include <glm.hpp>

class Camera
{
public:
	Camera();

	void SetViewportSize(uint32_t width, uint32_t height);
	void SetPerspective(float fov, float nearPlane, float farPlane);
	inline void SetPerspectiveFOV(float fov) { m_PerspectiveFOV = fov; RecalculateProjection(); }
	inline void SetPerspectiveNear(float value) { m_PerspectiveNear = value; RecalculateProjection(); }
	inline void SetPerspectiveFar(float value) { m_PerspectiveFar = value; RecalculateProjection(); }
	inline float GetPerspectiveFOV() const { return m_PerspectiveFOV; }
	inline float GetPerspectiveNear() const { return m_PerspectiveNear; }
	inline float GetPerspectiveFar() const { return m_PerspectiveFar; }

	const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }
private:
	void RecalculateProjection();
private:
	glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
	float m_AspectRatio = 16.0f / 9.0f;
	float m_PerspectiveFOV = 45.0f;
	float m_PerspectiveNear = 0.1f;
	float m_PerspectiveFar = 1000.0f;
};