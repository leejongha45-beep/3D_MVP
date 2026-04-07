#pragma once

#include "3D_MVP.h"

#include <glm/glm.hpp>

class QWaveLight
{
public:
	QWaveLight();
	~QWaveLight();

	glm::vec3 getDirection() const { return direction; }
	float getFrequency() const { return frequency; }
	float getPropagationSpeed() const { return propagationSpeed; }
	float getDamping() const { return damping; }

	void setDirection(const glm::vec3& value) { direction = value; }
	void setFrequency(float value) { frequency = value; }
	void setPropagationSpeed(float value) { propagationSpeed = value; }
	void setDamping(float value) { damping = value; }

private:
	glm::vec3 direction = glm::normalize(glm::vec3(0.85f, 0.15f, 0.5f));
	float frequency = 0.25f;
	float propagationSpeed = 0.7f;
	float damping = 0.9988f;
};
