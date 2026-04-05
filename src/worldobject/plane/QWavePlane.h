#pragma once

#include "3D_MVP.h"
#include "interface/QObject.h"

#include <glm/glm.hpp>

class QWavePlane : public QObject
{
public:
	QWavePlane();
	virtual ~QWavePlane() override;

	void initialize() override;
	void update(float deltaSeconds) override;
	void release() override;

	glm::vec3 getPosition() const { return position; }
	glm::vec3 getNormal() const { return normal; }
	glm::vec3 getColor() const { return color; }
	float getSize() const { return size; }

	void setPosition(const glm::vec3& value) { position = value; }
	void setNormal(const glm::vec3& value) { normal = value; }
	void setColor(const glm::vec3& value) { color = value; }
	void setSize(float value) { size = value; }

private:
	glm::vec3 position = glm::vec3(0.0f, -0.5f, 0.0f);
	glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 color = glm::vec3(0.4f, 0.4f, 0.4f);
	float size = 100.0f;
};
