#pragma once

#include "3D_MVP.h"

#include <glm/glm.hpp>

class QWaveLight
{
public:
	QWaveLight();
	~QWaveLight();

	glm::vec3 getDirection() const { return direction; }
	glm::vec3 getColor() const { return color; }
	float getIntensity() const { return intensity; }
	float getAmbient() const { return ambient; }

	void setDirection(const glm::vec3& value) { direction = value; }
	void setColor(const glm::vec3& value) { color = value; }
	void setIntensity(float value) { intensity = value; }
	void setAmbient(float value) { ambient = value; }

private:
	glm::vec3 direction = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	float intensity = 0.85f;
	float ambient = 0.15f;
};
