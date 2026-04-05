#include "QWaveWorld.h"
#include "camera/QWaveCamera.h"
#include "worldobject/plane/QWavePlane.h"
#include "worldobject/light/QWaveLight.h"

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
	lightInst = std::make_unique<QWaveLight>();
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
	lightInst.reset();
	planeInst.reset();
	cameraInst.reset();
}
