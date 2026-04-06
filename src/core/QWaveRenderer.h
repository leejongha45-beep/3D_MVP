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
	void createSyncObjects();
	void updateSceneData();

	std::vector<std::unique_ptr<class QRenderObject>> renderObjects;
	class VulkanSwapChain* swapChainRef = nullptr;
	class VulkanDevice* vulkanDeviceRef = nullptr;
	class VulkanUniformBuffer* vulkanUniformBufferRef = nullptr;
	class VulkanCommandPool* vulkanCommandPoolRef = nullptr;

	std::vector<vk::raii::Semaphore> imageAvailableSemaphoresInst;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphoresInst;
	std::vector<vk::raii::Fence> inFlightFencesInst;
	uint32_t currentFrameInst = 0;
};
