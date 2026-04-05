#pragma once

#include "3D_MVP.h"
#include "interface/QObject.h"

#include <memory>
#include <vector>

class QWaveRenderer : public QObject
{
public:
	QWaveRenderer(GLFWwindow* windowRef, class QWaveWorld* worldRef);
	virtual ~QWaveRenderer() override;

	void initialize() override;
	void update(float deltaSeconds) override;
	void release() override;

private:
	GLFWwindow* windowRef = nullptr;
	const class QWaveWorld* worldRef = nullptr;
	std::vector<std::unique_ptr<class QRenderObject>> renderObjects;
	class VulkanSwapChain* swapChainRef = nullptr;
};
