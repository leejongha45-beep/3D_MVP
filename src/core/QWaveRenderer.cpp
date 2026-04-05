#include "QWaveRenderer.h"
#include "interface/QRenderObject.h"
#include "vulkan/VulkanInstance/VulkanInstance.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"
#include "vulkan/VulkanSurface/VulkanSurface.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanSwapChain/VulkanSwapChain.h"
#include "QWaveWorld.h"
#include "camera/QWaveCamera.h"
#include "worldobject/plane/QWavePlane.h"
#include "worldobject/light/QWaveLight.h"
#include "utils/GpuStructs.h"

QWaveRenderer::QWaveRenderer(GLFWwindow* windowRef, QWaveWorld* worldRef)
	: windowRef(windowRef), worldRef(worldRef)
{
	assert(windowRef);
	assert(worldRef);
}

QWaveRenderer::~QWaveRenderer()
{
}

void QWaveRenderer::initialize()
{
	auto pVulkanInstance = new VulkanInstance();
	renderObjects.emplace_back(pVulkanInstance);

	auto pVulkanSurface = new VulkanSurface(pVulkanInstance, windowRef);
	renderObjects.emplace_back(pVulkanSurface);

	auto pVulkanPhysicalDevice = new VulkanPhysicalDevice(pVulkanInstance);
	renderObjects.emplace_back(pVulkanPhysicalDevice);

	auto pVulkanDevice = new VulkanDevice(pVulkanPhysicalDevice);
	renderObjects.emplace_back(pVulkanDevice);

	swapChainRef = new VulkanSwapChain(pVulkanDevice, pVulkanPhysicalDevice, pVulkanSurface, windowRef);
	renderObjects.emplace_back(swapChainRef);

	for (auto& pRenderObject : renderObjects)
	{
		if (ENSURE(pRenderObject))
		{
			pRenderObject->create();
		}
	}
}

void QWaveRenderer::update(float deltaSeconds)
{
	if (!ENSURE(swapChainRef))
		return;
	if (!ENSURE(worldRef))
		return;

	QWaveCamera* pCamera = worldRef->getCameraRef();
	QWavePlane* pPlane = worldRef->getPlaneRef();
	QWaveLight* pLight = worldRef->getLightRef();

	if (ENSURE(pCamera) && ENSURE(pPlane) && ENSURE(pLight))
	{
		SceneData sceneData{};
		sceneData.camera.position = pCamera->getPosition();
		sceneData.camera.target   = pCamera->getTarget();
		sceneData.camera.up       = pCamera->getUp();
		sceneData.camera.fov      = pCamera->getFov();

		sceneData.plane.position = pPlane->getPosition();
		sceneData.plane.normal   = pPlane->getNormal();
		sceneData.plane.color    = pPlane->getColor();
		sceneData.plane.size     = pPlane->getSize();

		sceneData.light.direction = pLight->getDirection();
		sceneData.light.color     = pLight->getColor();
		sceneData.light.intensity = pLight->getIntensity();
		sceneData.light.ambient   = pLight->getAmbient();

		swapChainRef->updateSceneData(sceneData);
	}

	swapChainRef->drawFrame();
}

void QWaveRenderer::release()
{
	while (!renderObjects.empty())
	{
		renderObjects.pop_back();
	}
}
