
// SILENCE WARNINGS FROM GLM
// 4201: warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)

#include <glm/gtc/matrix_transform.hpp>

#pragma warning(pop)

#include "base_camera.h"

BaseCamera::BaseCamera() : m_viewMatrix(glm::identity<glm::mat4>()), m_projMatrix(glm::identity<glm::mat4>())
{
	// Nothing to do here
}

BaseCamera::~BaseCamera()
{
	// Nothing to do here
}

BaseCamera::BaseCamera(const BaseCamera& other) : m_viewMatrix(other.m_viewMatrix), m_projMatrix(other.m_projMatrix)
{
	// Nothing to do here
}

BaseCamera::BaseCamera(BaseCamera&& other) noexcept : m_viewMatrix(std::move(other.m_viewMatrix)), m_projMatrix(std::move(other.m_projMatrix))
{
	// Nothing to do here
}

BaseCamera& BaseCamera::operator=(const BaseCamera& other)
{
	// Protect against self-assignment
	if (this == &other)
	{
		return *this;
	}

	m_viewMatrix = other.m_viewMatrix;

	return *this;
}

glm::mat4 BaseCamera::GetViewMatrix() const
{
	return glm::inverse(m_viewMatrix);
}

glm::mat4 BaseCamera::GetProjMatrix() const
{
	return m_projMatrix;
}

glm::vec3 BaseCamera::GetPosition() const
{
	return glm::vec3(m_viewMatrix[3]);
}