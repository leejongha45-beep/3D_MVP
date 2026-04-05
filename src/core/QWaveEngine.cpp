#include "QWaveEngine.h"
#include "QWaveRenderer.h"
#include "QWaveWorld.h"

QWaveEngine::QWaveEngine()
{
}

QWaveEngine::~QWaveEngine()
{
}

void QWaveEngine::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void QWaveEngine::initWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	pWindow = glfwCreateWindow(windowWidth, windowHeight, "QuantumWave Renderer", nullptr, nullptr);
	assert(pWindow);
}

void QWaveEngine::initVulkan()
{
	worldInst = std::make_unique<QWaveWorld>(pWindow);
	if (ENSURE(worldInst))
	{
		worldInst->initialize();
	}

	rendererInst = std::make_unique<QWaveRenderer>(pWindow, worldInst.get());
	if (ENSURE(rendererInst))
	{
		rendererInst->initialize();
	}
}

void QWaveEngine::mainLoop()
{
	double previousTime = glfwGetTime();
	constexpr double targetFrameTime = 1.0 / 60.0;

	while (!glfwWindowShouldClose(pWindow))
	{
		double currentTime = glfwGetTime();
		double elapsedTime = currentTime - previousTime;

		if (elapsedTime < targetFrameTime)
			continue;

		float deltaSeconds = static_cast<float>(elapsedTime);
		previousTime = currentTime;

		glfwPollEvents();
		if (ENSURE(worldInst))
		{
			worldInst->update(deltaSeconds);
		}
		if (ENSURE(rendererInst))
		{
			rendererInst->update(deltaSeconds);
		}
	}
}

void QWaveEngine::cleanup()
{
	if (ENSURE(rendererInst))
	{
		rendererInst->release();
	}
	rendererInst.reset();

	if (ENSURE(worldInst))
	{
		worldInst->release();
	}
	worldInst.reset();

	if (pWindow)
	{
		glfwDestroyWindow(pWindow);
	}
	glfwTerminate();
}
