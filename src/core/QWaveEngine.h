#pragma once

#include "3D_MVP.h"

#include <memory>

class QWaveEngine
{
public:
	QWaveEngine();
	~QWaveEngine();

	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	GLFWwindow* pWindow = nullptr;
	uint32_t windowWidth = 1280;
	uint32_t windowHeight = 720;

	std::unique_ptr<class QWaveRenderer> rendererInst;
	std::unique_ptr<class QWaveWorld> worldInst;
};
