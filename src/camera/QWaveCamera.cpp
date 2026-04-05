#include "QWaveCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

QWaveCamera::QWaveCamera(GLFWwindow* windowRef)
	: windowRef(windowRef)
{
	assert(windowRef);
}

QWaveCamera::~QWaveCamera()
{
}

void QWaveCamera::initialize()
{
	// 초기 방향 벡터를 position → target에서 계산
	glm::vec3 direction = glm::normalize(target - position);
	yaw = glm::degrees(std::atan2(direction.z, direction.x));
	pitch = glm::degrees(std::asin(direction.y));
	updateVectors();
}

void QWaveCamera::update(float deltaSeconds)
{
	processKeyboard(deltaSeconds);
	processMouse(deltaSeconds);
}

void QWaveCamera::release()
{
}

// WASD 키보드 이동
void QWaveCamera::processKeyboard(float deltaSeconds)
{
	float velocity = moveSpeed * deltaSeconds;

	if (glfwGetKey(windowRef, GLFW_KEY_W) == GLFW_PRESS)
	{
		position += forward * velocity;
		dirty = true;
	}
	if (glfwGetKey(windowRef, GLFW_KEY_S) == GLFW_PRESS)
	{
		position -= forward * velocity;
		dirty = true;
	}
	if (glfwGetKey(windowRef, GLFW_KEY_A) == GLFW_PRESS)
	{
		position -= right * velocity;
		dirty = true;
	}
	if (glfwGetKey(windowRef, GLFW_KEY_D) == GLFW_PRESS)
	{
		position += right * velocity;
		dirty = true;
	}

	if (dirty)
	{
		target = position + forward;
	}
}

// 마우스 우클릭 드래그 회전
void QWaveCamera::processMouse(float deltaSeconds)
{
	double mouseX = 0.0;
	double mouseY = 0.0;
	glfwGetCursorPos(windowRef, &mouseX, &mouseY);

	if (glfwGetMouseButton(windowRef, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		if (!rightMousePressed)
		{
			rightMousePressed = true;
			lastMouseX = mouseX;
			lastMouseY = mouseY;
		}

		float offsetX = static_cast<float>(mouseX - lastMouseX) * mouseSensitivity;
		float offsetY = static_cast<float>(lastMouseY - mouseY) * mouseSensitivity;

		lastMouseX = mouseX;
		lastMouseY = mouseY;

		if (offsetX != 0.0f || offsetY != 0.0f)
		{
			yaw += offsetX;
			pitch += offsetY;

			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;

			updateVectors();
			target = position + forward;
			dirty = true;
		}
	}
	else
	{
		rightMousePressed = false;
	}
}

// yaw/pitch로부터 forward/right/up 벡터 재계산
void QWaveCamera::updateVectors()
{
	float yawRad = glm::radians(yaw);
	float pitchRad = glm::radians(pitch);

	forward.x = std::cos(yawRad) * std::cos(pitchRad);
	forward.y = std::sin(pitchRad);
	forward.z = std::sin(yawRad) * std::cos(pitchRad);
	forward = glm::normalize(forward);

	right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	up = glm::normalize(glm::cross(right, forward));
}
