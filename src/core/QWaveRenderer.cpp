#include "QWaveRenderer.h"
#include "interface/QRenderObject.h"
#include "vulkan/VulkanInstance/VulkanInstance.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"
#include "vulkan/VulkanSurface/VulkanSurface.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanSwapChain/VulkanSwapChain.h"
#include "vulkan/buffer/VulkanUniformBuffer.h"
#include "vulkan/buffer/VulkanStorageBuffer.h"
#include "vulkan/VulkanCommandPool/VulkanCommandPool.h"
#include "QWaveWorld.h"
#include "camera/QWaveCamera.h"
#include "worldobject/plane/QWavePlane.h"
#include "worldobject/light/QWaveLight.h"
#include "utils/GpuStructs.h"

#include <array>

QWaveRenderer::QWaveRenderer(GLFWwindow* windowRef, QWaveWorld* worldRef)
	: windowRef(windowRef), worldRef(worldRef)
{
	assert(windowRef);
	assert(worldRef);
}

QWaveRenderer::~QWaveRenderer()
{
	if (ENSURE(vulkanDeviceRef))
	{
		const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
		if (ENSURE(pDevice))
		{
			pDevice->waitIdle();
		}
	}
}

void QWaveRenderer::initialize()
{
	auto pVulkanInstance = new VulkanInstance();
	renderObjects.emplace_back(pVulkanInstance);

	auto pVulkanSurface = new VulkanSurface(pVulkanInstance, windowRef);
	renderObjects.emplace_back(pVulkanSurface);

	auto pVulkanPhysicalDevice = new VulkanPhysicalDevice(pVulkanInstance, pVulkanSurface);
	renderObjects.emplace_back(pVulkanPhysicalDevice);

	vulkanDeviceRef = new VulkanDevice(pVulkanPhysicalDevice, pVulkanSurface);
	renderObjects.emplace_back(vulkanDeviceRef);

	vulkanUniformBufferRef = new VulkanUniformBuffer(vulkanDeviceRef, pVulkanPhysicalDevice);
	renderObjects.emplace_back(vulkanUniformBufferRef);

	vulkanStorageBufferRef = new VulkanStorageBuffer(vulkanDeviceRef, pVulkanPhysicalDevice);
	renderObjects.emplace_back(vulkanStorageBufferRef);

	swapChainRef = new VulkanSwapChain(vulkanDeviceRef, pVulkanPhysicalDevice, pVulkanSurface, vulkanUniformBufferRef, vulkanStorageBufferRef, windowRef);
	renderObjects.emplace_back(swapChainRef);

	vulkanCommandPoolRef = new VulkanCommandPool(vulkanDeviceRef, swapChainRef);
	renderObjects.emplace_back(vulkanCommandPoolRef);

	for (auto& pRenderObject : renderObjects)
	{
		if (ENSURE(pRenderObject))
		{
			pRenderObject->create();
		}
	}

	createSyncObjects();
	markStaticObjects();
}

void QWaveRenderer::update(float deltaSeconds)
{
	if (!ENSURE(vulkanDeviceRef))
		return;
	if (!ENSURE(swapChainRef))
		return;
	if (!ENSURE(vulkanCommandPoolRef))
		return;

	const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
	if (!ENSURE(pDevice))
		return;

	const vk::raii::Queue* pGraphicsQueue = vulkanDeviceRef->getGraphicsQueueInst();
	if (!ENSURE(pGraphicsQueue))
		return;

	const vk::raii::Queue* pPresentQueue = vulkanDeviceRef->getPresentQueueInst();
	if (!ENSURE(pPresentQueue))
		return;

	// SceneData 업데이트
	updateSceneData();

	// Fence 대기
	auto waitResult = pDevice->waitForFences(*inFlightFencesInst[currentFrameInst], VK_TRUE, UINT64_MAX);
	pDevice->resetFences(*inFlightFencesInst[currentFrameInst]);

	// 스왑체인 이미지 획득
	const vk::raii::SwapchainKHR* pSwapChain = swapChainRef->getSwapChainInst();
	if (!ENSURE(pSwapChain))
		return;

	auto [result, imageIndex] = pSwapChain->acquireNextImage(UINT64_MAX, *imageAvailableSemaphoresInst[currentFrameInst]);

	// 커맨드 버퍼 기록
	const vk::raii::CommandBuffer* pCommandBuffer = vulkanCommandPoolRef->getCommandBufferInst(currentFrameInst);
	if (!ENSURE(pCommandBuffer))
		return;

	const vk::Extent2D& extent = swapChainRef->getExtentInst();
	const vk::raii::Image* pStorageImage = swapChainRef->getStorageImageInst();
	const auto& swapChainImages = swapChainRef->getSwapChainImagesInst();

	pCommandBuffer->reset();
	pCommandBuffer->begin(vk::CommandBufferBeginInfo{});

	// Storage Image → General 전환
	vk::ImageMemoryBarrier storageBarrier{
		.srcAccessMask    = vk::AccessFlagBits::eNone,
		.dstAccessMask    = vk::AccessFlagBits::eShaderWrite,
		.oldLayout        = vk::ImageLayout::eUndefined,
		.newLayout        = vk::ImageLayout::eGeneral,
		.image            = **pStorageImage,
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	pCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eComputeShader,
		{}, {}, {}, storageBarrier);

	// 파동 시뮬레이션 Compute 디스패치
	pCommandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, **swapChainRef->getWaveSimulationPipelineInst());
	pCommandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute, **swapChainRef->getPipelineLayoutInst(), 0, **swapChainRef->getDescriptorSetInst(), {});
	pCommandBuffer->dispatch(128 / 8, 128 / 8, 128 / 8);

	// 파동 시뮬레이션 → 렌더링 배리어
	vk::MemoryBarrier2 computeBarrier{
		.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
		.srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
		.dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
		.dstAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite
	};
	vk::DependencyInfo dependencyInfo{
		.memoryBarrierCount = 1,
		.pMemoryBarriers    = &computeBarrier
	};
	pCommandBuffer->pipelineBarrier2(dependencyInfo);

	// 렌더링 Compute 디스패치
	pCommandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, **swapChainRef->getRenderPipelineInst());
	pCommandBuffer->dispatch(
		(extent.width + 15) / 16,
		(extent.height + 15) / 16,
		1);

	// Storage Image → TransferSrc, SwapChain Image → TransferDst 전환
	vk::ImageMemoryBarrier storageToSrcBarrier{
		.srcAccessMask    = vk::AccessFlagBits::eShaderWrite,
		.dstAccessMask    = vk::AccessFlagBits::eTransferRead,
		.oldLayout        = vk::ImageLayout::eGeneral,
		.newLayout        = vk::ImageLayout::eTransferSrcOptimal,
		.image            = **pStorageImage,
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	vk::ImageMemoryBarrier swapChainToDstBarrier{
		.srcAccessMask    = vk::AccessFlagBits::eNone,
		.dstAccessMask    = vk::AccessFlagBits::eTransferWrite,
		.oldLayout        = vk::ImageLayout::eUndefined,
		.newLayout        = vk::ImageLayout::eTransferDstOptimal,
		.image            = swapChainImages[imageIndex],
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	std::array<vk::ImageMemoryBarrier, 2> blitBarriers = {storageToSrcBarrier, swapChainToDstBarrier};
	pCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader,
		vk::PipelineStageFlagBits::eTransfer,
		{}, {}, {}, blitBarriers);

	// Storage Image → SwapChain Image blit
	vk::ImageBlit blitRegion{
		.srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		.srcOffsets     = std::array<vk::Offset3D, 2>{
			vk::Offset3D{0, 0, 0},
			vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}
		},
		.dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		.dstOffsets     = std::array<vk::Offset3D, 2>{
			vk::Offset3D{0, 0, 0},
			vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), 1}
		}
	};
	pCommandBuffer->blitImage(
		**pStorageImage, vk::ImageLayout::eTransferSrcOptimal,
		swapChainImages[imageIndex], vk::ImageLayout::eTransferDstOptimal,
		blitRegion, vk::Filter::eNearest);

	// SwapChain Image → PresentSrc 전환
	vk::ImageMemoryBarrier presentBarrier{
		.srcAccessMask    = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask    = vk::AccessFlagBits::eNone,
		.oldLayout        = vk::ImageLayout::eTransferDstOptimal,
		.newLayout        = vk::ImageLayout::ePresentSrcKHR,
		.image            = swapChainImages[imageIndex],
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	pCommandBuffer->pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		{}, {}, {}, presentBarrier);

	pCommandBuffer->end();

	// 커맨드 버퍼 제출
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader;
	vk::SubmitInfo submitInfo{
		.waitSemaphoreCount   = 1,
		.pWaitSemaphores      = &*imageAvailableSemaphoresInst[currentFrameInst],
		.pWaitDstStageMask    = &waitStage,
		.commandBufferCount   = 1,
		.pCommandBuffers      = &**pCommandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores    = &*renderFinishedSemaphoresInst[currentFrameInst]
	};
	pGraphicsQueue->submit(submitInfo, *inFlightFencesInst[currentFrameInst]);

	// 프레젠트
	vk::PresentInfoKHR presentInfo{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = &*renderFinishedSemaphoresInst[currentFrameInst],
		.swapchainCount     = 1,
		.pSwapchains        = &**pSwapChain,
		.pImageIndices      = &imageIndex
	};
	auto presentResult = pPresentQueue->presentKHR(presentInfo);

	currentFrameInst = (currentFrameInst + 1) % swapChainRef->getImageCount();
}

void QWaveRenderer::release()
{
	if (ENSURE(vulkanDeviceRef))
	{
		const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
		if (ENSURE(pDevice))
		{
			pDevice->waitIdle();
		}
	}

	imageAvailableSemaphoresInst.clear();
	renderFinishedSemaphoresInst.clear();
	inFlightFencesInst.clear();

	while (!renderObjects.empty())
	{
		renderObjects.pop_back();
	}
}

void QWaveRenderer::createSyncObjects()
{
	if (!ENSURE(vulkanDeviceRef))
		return;
	if (!ENSURE(swapChainRef))
		return;

	const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
	if (!ENSURE(pDevice))
		return;

	uint32_t imageCount = swapChainRef->getImageCount();

	vk::SemaphoreCreateInfo semaphoreCreateInfo{};
	vk::FenceCreateInfo fenceCreateInfo{
		.flags = vk::FenceCreateFlagBits::eSignaled
	};

	for (uint32_t i = 0; i < imageCount; ++i)
	{
		imageAvailableSemaphoresInst.emplace_back(*pDevice, semaphoreCreateInfo);
		renderFinishedSemaphoresInst.emplace_back(*pDevice, semaphoreCreateInfo);
		inFlightFencesInst.emplace_back(*pDevice, fenceCreateInfo);
	}
}

void QWaveRenderer::updateSceneData()
{
	if (!ENSURE(worldRef))
		return;
	if (!ENSURE(vulkanUniformBufferRef))
		return;

	QWaveCamera* pCamera = worldRef->getCameraRef();
	QWavePlane* pPlane = worldRef->getPlaneRef();
	QWaveLight* pLight = worldRef->getLightRef();

	if (!ENSURE(pCamera) || !ENSURE(pPlane) || !ENSURE(pLight))
		return;

	SceneData sceneData{};
	sceneData.camera.position = pCamera->getPosition();
	sceneData.camera.forward  = pCamera->getForward();
	sceneData.camera.up       = pCamera->getUp();
	sceneData.camera.fov      = pCamera->getFov();

	sceneData.plane.position = pPlane->getPosition();
	sceneData.plane.normal   = pPlane->getNormal();
	sceneData.plane.color    = pPlane->getColor();
	sceneData.plane.size     = pPlane->getSize();

	sceneData.light.direction        = pLight->getDirection();
	sceneData.light.frequency        = pLight->getFrequency();
	sceneData.light.propagationSpeed = pLight->getPropagationSpeed();
	sceneData.light.damping          = pLight->getDamping();

	vulkanUniformBufferRef->updateSceneData(sceneData);
}

void QWaveRenderer::markStaticObjects()
{
	if (!ENSURE(worldRef))
		return;
	if (!ENSURE(vulkanStorageBufferRef))
		return;

	QWavePlane* pPlane = worldRef->getPlaneRef();
	if (!ENSURE(pPlane))
		return;

	glm::vec3 planePosition = pPlane->getPosition();
	glm::vec3 planeColor = pPlane->getColor();
	float planeSize = pPlane->getSize();
	float halfSize = planeSize * 0.5f;
	uint32_t gridSize = vulkanStorageBufferRef->getGridSize();

	// 플레인 영역을 격자에 마킹 (y 고정, xz 평면)
	int gridY = static_cast<int>(planePosition.y);
	int startX = static_cast<int>(planePosition.x - halfSize);
	int endX = static_cast<int>(planePosition.x + halfSize);
	int startZ = static_cast<int>(planePosition.z - halfSize);
	int endZ = static_cast<int>(planePosition.z + halfSize);

	for (int z = startZ; z <= endZ; ++z)
	{
		for (int x = startX; x <= endX; ++x)
		{
			glm::vec3 worldPos = glm::vec3(static_cast<float>(x), static_cast<float>(gridY), static_cast<float>(z));
			vulkanStorageBufferRef->markStaticObject(worldPos, planeColor);
		}
	}
}
