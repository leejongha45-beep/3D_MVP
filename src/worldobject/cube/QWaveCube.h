#pragma once

#include "3D_MVP.h"
#include "interface/QObject.h"
#include "utils/GpuStructs.h"

#include <glm/glm.hpp>
#include <vector>

class QWaveCube : public QObject
{
public:
	QWaveCube(const glm::vec3& position, const glm::vec3& size, const glm::vec3& reflectSpectrum, float roughness);
	virtual ~QWaveCube() override;

	void initialize() override;
	void update(float deltaSeconds) override;
	void release() override;

	const std::vector<VertexData>& getVertices() const { return verticesInst; }
	glm::vec3 getPosition() const { return position; }
	glm::vec3 getSize() const { return size; }

private:
	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 reflectSpectrum;
	float roughness;

	std::vector<VertexData> verticesInst;
};
