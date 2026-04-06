#include "QWavePlane.h"

QWavePlane::QWavePlane()
{
}

QWavePlane::~QWavePlane()
{
}

void QWavePlane::initialize()
{
	float halfSize = size * 0.5f;
	int startX = static_cast<int>(position.x - halfSize);
	int endX = static_cast<int>(position.x + halfSize);
	int startZ = static_cast<int>(position.z - halfSize);
	int endZ = static_cast<int>(position.z + halfSize);
	int gridY = static_cast<int>(position.y);

	for (int z = startZ; z <= endZ; ++z)
	{
		for (int x = startX; x <= endX; ++x)
		{
			VertexData vertex{};
			vertex.position = glm::vec3(static_cast<float>(x), static_cast<float>(gridY), static_cast<float>(z));
			vertex.reflectSpectrum = reflectSpectrum;
			vertex.roughness = roughness;
			verticesInst.push_back(vertex);
		}
	}
}

void QWavePlane::update(float deltaSeconds)
{
}

void QWavePlane::release()
{
}
