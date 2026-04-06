#include "QWaveCube.h"

QWaveCube::QWaveCube(const glm::vec3& position, const glm::vec3& size, const glm::vec3& reflectSpectrum)
	: position(position), size(size), reflectSpectrum(reflectSpectrum)
{
}

QWaveCube::~QWaveCube()
{
}

void QWaveCube::initialize()
{
	int startX = static_cast<int>(position.x - size.x * 0.5f);
	int endX = static_cast<int>(position.x + size.x * 0.5f);
	int startY = static_cast<int>(position.y);
	int endY = static_cast<int>(position.y + size.y);
	int startZ = static_cast<int>(position.z - size.z * 0.5f);
	int endZ = static_cast<int>(position.z + size.z * 0.5f);

	for (int y = startY; y <= endY; ++y)
	{
		for (int z = startZ; z <= endZ; ++z)
		{
			for (int x = startX; x <= endX; ++x)
			{
				VertexData vertex{};
				vertex.position = glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
				vertex.reflectSpectrum = reflectSpectrum;
				verticesInst.push_back(vertex);
			}
		}
	}
}

void QWaveCube::update(float deltaSeconds)
{
}

void QWaveCube::release()
{
}
