#pragma once

#include "3D_MVP.h"
#include "interface/QObject.h"

#include <memory>

class QWaveWorld : public QObject
{
public:
	QWaveWorld(GLFWwindow* windowRef);
	virtual ~QWaveWorld() override;

	void initialize() override;
	void update(float deltaSeconds) override;
	void release() override;

	class QWaveCamera* getCameraRef() const { return cameraInst.get(); }
	class QWavePlane* getPlaneRef() const { return planeInst.get(); }
	class QWaveLight* getLightRef() const { return lightInst.get(); }

private:
	GLFWwindow* windowRef = nullptr;
	std::unique_ptr<class QWaveCamera> cameraInst;
	std::unique_ptr<class QWavePlane> planeInst;
	std::unique_ptr<class QWaveLight> lightInst;
};
