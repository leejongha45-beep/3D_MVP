#pragma once

#include "3D_MVP.h"
#include "interface/QObject.h"

#include <glm/glm.hpp>

class QWaveCamera : public QObject
{
public:
	QWaveCamera(GLFWwindow* windowRef);
	virtual ~QWaveCamera() override;

	void initialize() override;
	void update(float deltaSeconds) override;
	void release() override;

	glm::vec3 getPosition() const { return position; }
	glm::vec3 getTarget() const { return target; }
	glm::vec3 getUp() const { return up; }
	glm::vec3 getForward() const { return forward; }
	float getFov() const { return fov; }
	bool isDirty() const { return dirty; }
	void clearDirty() { dirty = false; }

	void setPosition(const glm::vec3& value) { position = value; dirty = true; }
	void setFov(float value) { fov = value; dirty = true; }

private:
	void processKeyboard(float deltaSeconds);
	void processMouse(float deltaSeconds);
	void updateVectors();

	GLFWwindow* windowRef = nullptr;

	glm::vec3 position = glm::vec3(0.0f, 30.0f, -40.0f);
	glm::vec3 target = glm::vec3(0.0f, 5.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

	float yaw = -90.0f;
	float pitch = 0.0f;
	float moveSpeed = 3.0f;
	float mouseSensitivity = 0.15f;
	float fov = 60.0f;

	double lastMouseX = 0.0;
	double lastMouseY = 0.0;
	bool rightMousePressed = false;
	bool dirty = true;
};
