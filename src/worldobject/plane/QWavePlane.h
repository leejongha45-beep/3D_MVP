#pragma once

#include "3D_MVP.h"
#include "interface/QObject.h"
#include "utils/GpuStructs.h"

#include <glm/glm.hpp>
#include <vector>

class QWavePlane : public QObject
{
public:
	QWavePlane();
	virtual ~QWavePlane() override;

	void initialize() override;
	void update(float deltaSeconds) override;
	void release() override;

	const std::vector<VertexData>& getVertices() const { return verticesInst; }
	glm::vec3 getPosition() const { return position; }
	float getSize() const { return size; }

	void setPosition(const glm::vec3& value) { position = value; }
	void setSize(float value) { size = value; }
	void setReflectSpectrum(const glm::vec3& value) { reflectSpectrum = value; }

private:
	glm::vec3 position = glm::vec3(0.0f, 5.0f, 0.0f);
	glm::vec3 reflectSpectrum = glm::vec3(0.4f, 0.4f, 0.4f);
	float size = 100.0f;

	std::vector<VertexData> verticesInst;
};
