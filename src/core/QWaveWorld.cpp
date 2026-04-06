#include "QWaveWorld.h"
#include "camera/QWaveCamera.h"
#include "worldobject/plane/QWavePlane.h"
#include "worldobject/light/QWaveLight.h"
#include "worldobject/cube/QWaveCube.h"

QWaveWorld::QWaveWorld(GLFWwindow* windowRef)
	: windowRef(windowRef)
{
	assert(windowRef);
}

QWaveWorld::~QWaveWorld()
{
}

void QWaveWorld::initialize()
{
	cameraInst = std::make_unique<QWaveCamera>(windowRef);
	if (ENSURE(cameraInst))
	{
		cameraInst->initialize();
	}

	planeInst = std::make_unique<QWavePlane>();
	if (ENSURE(planeInst))
	{
		planeInst->initialize();
	}

	lightInst = std::make_unique<QWaveLight>();

	// 큐브 기둥 5개 (바닥 y=5 위에 세움)
	cubesInst.emplace_back(std::make_unique<QWaveCube>(glm::vec3(-20.0f, 5.0f, 0.0f), glm::vec3(8.0f, 40.0f, 8.0f), glm::vec3(0.8f, 0.2f, 0.2f)));
	cubesInst.emplace_back(std::make_unique<QWaveCube>(glm::vec3(-10.0f, 5.0f, 15.0f), glm::vec3(6.0f, 50.0f, 6.0f), glm::vec3(0.2f, 0.8f, 0.2f)));
	cubesInst.emplace_back(std::make_unique<QWaveCube>(glm::vec3(0.0f, 5.0f, -10.0f), glm::vec3(10.0f, 35.0f, 10.0f), glm::vec3(0.2f, 0.2f, 0.8f)));
	cubesInst.emplace_back(std::make_unique<QWaveCube>(glm::vec3(15.0f, 5.0f, 5.0f), glm::vec3(7.0f, 45.0f, 7.0f), glm::vec3(0.8f, 0.8f, 0.2f)));
	cubesInst.emplace_back(std::make_unique<QWaveCube>(glm::vec3(10.0f, 5.0f, -20.0f), glm::vec3(9.0f, 30.0f, 9.0f), glm::vec3(0.8f, 0.2f, 0.8f)));

	for (auto& cube : cubesInst)
	{
		if (ENSURE(cube))
		{
			cube->initialize();
		}
	}
}

void QWaveWorld::update(float deltaSeconds)
{
	if (ENSURE(cameraInst))
	{
		cameraInst->update(deltaSeconds);
	}
}

void QWaveWorld::release()
{
	cubesInst.clear();
	lightInst.reset();
	planeInst.reset();
	cameraInst.reset();
}
