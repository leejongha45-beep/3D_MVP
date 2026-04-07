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
	int startY = static_cast<int>(position.y - halfSize);
	int endY = static_cast<int>(position.y + halfSize);
	int gridZ = static_cast<int>(position.z);

	for (int y = startY; y <= endY; ++y)
	{
		for (int x = startX; x <= endX; ++x)
		{
			VertexData vertex{};
			vertex.position = glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(gridZ));
			vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
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
