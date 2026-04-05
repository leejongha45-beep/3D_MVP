#include "VulkanSwapChain.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"
#include "vulkan/VulkanSurface/VulkanSurface.h"

#include <algorithm>
#include <fstream>
#include <string>

VulkanSwapChain::VulkanSwapChain(
	VulkanDevice* vulkanDeviceRef, VulkanPhysicalDevice* vulkanPhysicalDeviceRef, VulkanSurface* vulkanSurfaceRef,
	GLFWwindow* windowRef)
	: vulkanDeviceRef(vulkanDeviceRef), vulkanPhysicalDeviceRef(vulkanPhysicalDeviceRef),
	  vulkanSurfaceRef(vulkanSurfaceRef), windowRef(windowRef)
{
	assert(vulkanDeviceRef);
	assert(vulkanPhysicalDeviceRef);
	assert(vulkanSurfaceRef);
	assert(windowRef);
}

VulkanSwapChain::~VulkanSwapChain()
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

// 스왑체인 생성: Surface 속성 조회 후 포맷/프레젠트모드/해상도 결정, 스왑체인 생성 및 이미지 획득
void VulkanSwapChain::create()
{
	if (!ENSURE(vulkanDeviceRef))
		return;
	if (!ENSURE(vulkanPhysicalDeviceRef))
		return;
	if (!ENSURE(vulkanSurfaceRef))
		return;

	const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
	if (!ENSURE(pDevice))
		return;

	const vk::raii::PhysicalDevice* pPhysicalDevice = vulkanPhysicalDeviceRef->getPhysicalDeviceInst();
	if (!ENSURE(pPhysicalDevice))
		return;

	const vk::raii::SurfaceKHR* pSurface = vulkanSurfaceRef->getSurfaceInst();
	if (!ENSURE(pSurface))
		return;

	vk::SurfaceCapabilitiesKHR capabilities		 = pPhysicalDevice->getSurfaceCapabilitiesKHR(**pSurface);
	std::vector<vk::SurfaceFormatKHR> formats	 = pPhysicalDevice->getSurfaceFormatsKHR(**pSurface);
	std::vector<vk::PresentModeKHR> presentModes = pPhysicalDevice->getSurfacePresentModesKHR(**pSurface);

	vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
	vk::PresentModeKHR presentMode	   = choosePresentMode(presentModes);
	vk::Extent2D extent				   = chooseExtent(capabilities);

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{
		.surface		  = **pSurface,
		.minImageCount	  = imageCount,
		.imageFormat	  = surfaceFormat.format,
		.imageColorSpace  = surfaceFormat.colorSpace,
		.imageExtent	  = extent,
		.imageArrayLayers = 1,
		.imageUsage		  = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform	  = capabilities.currentTransform,
		.compositeAlpha	  = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode	  = presentMode,
		.clipped		  = VK_TRUE};

	swapChainInst		= vk::raii::SwapchainKHR(*pDevice, swapChainCreateInfo);
	swapChainFormatInst = surfaceFormat.format;
	swapChainExtentInst = extent;
	swapChainImagesInst = swapChainInst.getImages();

	createImageViews(*pDevice);
	createStorageImage(*pDevice, *pPhysicalDevice);
	createUniformBuffer(*pDevice, *pPhysicalDevice);
	createCommandPoolAndBuffers(*pDevice);
	createComputePipeline(*pDevice);
	createDescriptorSet(*pDevice);
	createSyncObjects(*pDevice);
}

// 스왑체인 이미지 개수만큼 ImageView 생성
void VulkanSwapChain::createImageViews(const vk::raii::Device& device)
{
	swapChainImageViewsInst.reserve(swapChainImagesInst.size());

	for (const auto& image : swapChainImagesInst)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo{
			.image	  = image,
			.viewType = vk::ImageViewType::e2D,
			.format	  = swapChainFormatInst,
			.components =
				{.r = vk::ComponentSwizzle::eIdentity,
				 .g = vk::ComponentSwizzle::eIdentity,
				 .b = vk::ComponentSwizzle::eIdentity,
				 .a = vk::ComponentSwizzle::eIdentity},
			.subresourceRange = {
				.aspectMask		= vk::ImageAspectFlagBits::eColor,
				.baseMipLevel	= 0,
				.levelCount		= 1,
				.baseArrayLayer = 0,
				.layerCount		= 1}};

		swapChainImageViewsInst.emplace_back(device, imageViewCreateInfo);
	}
}

// Graphics 큐 패밀리로 CommandPool 생성, 스왑체인 이미지 개수만큼 CommandBuffer 할당
void VulkanSwapChain::createCommandPoolAndBuffers(const vk::raii::Device& device)
{
	vk::CommandPoolCreateInfo commandPoolCreateInfo{
		.flags			  = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = vulkanDeviceRef->getGraphicsFamily()};

	commandPoolInst = vk::raii::CommandPool(device, commandPoolCreateInfo);

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
		.commandPool		= *commandPoolInst,
		.level				= vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = static_cast<uint32_t>(swapChainImagesInst.size())};

	commandBuffersInst = device.allocateCommandBuffers(commandBufferAllocateInfo);
}

// Compute Shader 출력용 Storage Image 생성 (스왑체인과 동일한 해상도)
void VulkanSwapChain::createStorageImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
{
	vk::ImageCreateInfo imageCreateInfo{
		.imageType	   = vk::ImageType::e2D,
		.format		   = vk::Format::eR8G8B8A8Unorm,
		.extent		   = {swapChainExtentInst.width, swapChainExtentInst.height, 1},
		.mipLevels	   = 1,
		.arrayLayers   = 1,
		.samples	   = vk::SampleCountFlagBits::e1,
		.tiling		   = vk::ImageTiling::eOptimal,
		.usage		   = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
		.sharingMode   = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined};

	storageImageInst = vk::raii::Image(device, imageCreateInfo);

	vk::MemoryRequirements memoryRequirements = storageImageInst.getMemoryRequirements();
	uint32_t memoryTypeIndex =
		findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::MemoryAllocateInfo memoryAllocateInfo{
		.allocationSize = memoryRequirements.size, .memoryTypeIndex = memoryTypeIndex};

	storageImageMemoryInst = vk::raii::DeviceMemory(device, memoryAllocateInfo);
	storageImageInst.bindMemory(*storageImageMemoryInst, 0);

	vk::ImageViewCreateInfo imageViewCreateInfo{
		.image			  = *storageImageInst,
		.viewType		  = vk::ImageViewType::e2D,
		.format			  = vk::Format::eR8G8B8A8Unorm,
		.subresourceRange = {
			.aspectMask		= vk::ImageAspectFlagBits::eColor,
			.baseMipLevel	= 0,
			.levelCount		= 1,
			.baseArrayLayer = 0,
			.layerCount		= 1}};

	storageImageViewInst = vk::raii::ImageView(device, imageViewCreateInfo);
}

// SceneData용 Uniform Buffer 생성 (HOST_VISIBLE | HOST_COHERENT, persistently mapped)
void VulkanSwapChain::createUniformBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
{
	vk::BufferCreateInfo bufferCreateInfo{
		.size        = sizeof(SceneData),
		.usage       = vk::BufferUsageFlagBits::eUniformBuffer,
		.sharingMode = vk::SharingMode::eExclusive
	};

	uniformBufferInst = vk::raii::Buffer(device, bufferCreateInfo);

	vk::MemoryRequirements memoryRequirements = uniformBufferInst.getMemoryRequirements();
	uint32_t memoryTypeIndex = findMemoryType(
		physicalDevice,
		memoryRequirements.memoryTypeBits,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	vk::MemoryAllocateInfo memoryAllocateInfo{
		.allocationSize  = memoryRequirements.size,
		.memoryTypeIndex = memoryTypeIndex
	};

	uniformBufferMemoryInst = vk::raii::DeviceMemory(device, memoryAllocateInfo);
	uniformBufferInst.bindMemory(*uniformBufferMemoryInst, 0);

	uniformBufferMappedInst = uniformBufferMemoryInst.mapMemory(0, sizeof(SceneData));
	assert(uniformBufferMappedInst);
}

// CPU에서 SceneData를 매핑된 Uniform Buffer에 복사
void VulkanSwapChain::updateSceneData(const SceneData& sceneData)
{
	if (ENSURE(uniformBufferMappedInst))
	{
		memcpy(uniformBufferMappedInst, &sceneData, sizeof(SceneData));
	}
}

// 메모리 타입 인덱스 검색
uint32_t VulkanSwapChain::findMemoryType(
	const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
	vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	assert(false);
	return UINT32_MAX;
}

// SPIR-V 바이너리 파일 읽기
std::vector<uint32_t> VulkanSwapChain::readShaderFile(const std::string& filePath) const
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	assert(file.is_open());

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

// SPIR-V 코드로 ShaderModule 생성
vk::raii::ShaderModule VulkanSwapChain::createShaderModule(
	const vk::raii::Device& device, const std::vector<uint32_t>& code) const
{
	vk::ShaderModuleCreateInfo shaderModuleCreateInfo{.codeSize = code.size() * sizeof(uint32_t), .pCode = code.data()};

	return vk::raii::ShaderModule(device, shaderModuleCreateInfo);
}

// Compute Pipeline 생성: DescriptorSetLayout → PipelineLayout → Pipeline
void VulkanSwapChain::createComputePipeline(const vk::raii::Device& device)
{
	// Storage Image 바인딩 (Compute Shader 출력용)
	// Uniform Buffer 바인딩 (SceneData)
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
		vk::DescriptorSetLayoutBinding{
			.binding         = 0,
			.descriptorType  = vk::DescriptorType::eStorageImage,
			.descriptorCount = 1,
			.stageFlags      = vk::ShaderStageFlagBits::eCompute
		},
		vk::DescriptorSetLayoutBinding{
			.binding         = 1,
			.descriptorType  = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags      = vk::ShaderStageFlagBits::eCompute
		}
	};

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings    = bindings.data()
	};

	computeDescriptorSetLayoutInst = vk::raii::DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.setLayoutCount = 1, .pSetLayouts = &*computeDescriptorSetLayoutInst};

	computePipelineLayoutInst = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);

	// Compute Shader 로드
	auto shaderCode	  = readShaderFile(SHADER_DIR "slang.spv");
	auto shaderModule = createShaderModule(device, shaderCode);

	vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
		.stage = vk::ShaderStageFlagBits::eCompute, .module = *shaderModule, .pName = "main"};

	vk::ComputePipelineCreateInfo computePipelineCreateInfo{
		.stage = shaderStageCreateInfo, .layout = *computePipelineLayoutInst};

	computePipelineInst = vk::raii::Pipeline(device, nullptr, computePipelineCreateInfo);
}

// 프레임 동기화 객체 생성: Semaphore(GPU간 동기화), Fence(CPU-GPU 동기화)
void VulkanSwapChain::createSyncObjects(const vk::raii::Device& device)
{
	uint32_t imageCount = static_cast<uint32_t>(swapChainImagesInst.size());

	vk::SemaphoreCreateInfo semaphoreCreateInfo{};
	vk::FenceCreateInfo fenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled};

	for (uint32_t i = 0; i < imageCount; ++i)
	{
		imageAvailableSemaphoresInst.emplace_back(device, semaphoreCreateInfo);
		renderFinishedSemaphoresInst.emplace_back(device, semaphoreCreateInfo);
		inFlightFencesInst.emplace_back(device, fenceCreateInfo);
	}
}

// DescriptorPool 생성 후 Storage Image + Uniform Buffer를 바인딩한 DescriptorSet 할당
void VulkanSwapChain::createDescriptorSet(const vk::raii::Device& device)
{
	std::array<vk::DescriptorPoolSize, 2> poolSizes = {
		vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 1},
		vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1}
	};

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets       = 1,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes    = poolSizes.data()
	};

	descriptorPoolInst = vk::raii::DescriptorPool(device, descriptorPoolCreateInfo);

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
		.descriptorPool     = *descriptorPoolInst,
		.descriptorSetCount = 1,
		.pSetLayouts        = &*computeDescriptorSetLayoutInst
	};

	auto descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);
	computeDescriptorSetInst = std::move(descriptorSets[0]);

	vk::DescriptorImageInfo storageImageInfo{
		.imageView   = *storageImageViewInst,
		.imageLayout = vk::ImageLayout::eGeneral
	};

	vk::DescriptorBufferInfo uniformBufferInfo{
		.buffer = *uniformBufferInst,
		.offset = 0,
		.range  = sizeof(SceneData)
	};

	std::array<vk::WriteDescriptorSet, 2> writeDescriptorSets = {
		vk::WriteDescriptorSet{
			.dstSet          = *computeDescriptorSetInst,
			.dstBinding      = 0,
			.descriptorCount = 1,
			.descriptorType  = vk::DescriptorType::eStorageImage,
			.pImageInfo      = &storageImageInfo
		},
		vk::WriteDescriptorSet{
			.dstSet          = *computeDescriptorSetInst,
			.dstBinding      = 1,
			.descriptorCount = 1,
			.descriptorType  = vk::DescriptorType::eUniformBuffer,
			.pBufferInfo     = &uniformBufferInfo
		}
	};

	device.updateDescriptorSets(writeDescriptorSets, nullptr);
}

// 렌더 루프: Fence 대기 → 이미지 획득 → Compute 실행 → Storage→SwapChain blit → 프레젠트
void VulkanSwapChain::drawFrame()
{
	if (!ENSURE(vulkanDeviceRef))
		return;

	const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
	if (!ENSURE(pDevice))
		return;

	const vk::raii::Queue* pGraphicsQueue = vulkanDeviceRef->getGraphicsQueueInst();
	if (!ENSURE(pGraphicsQueue))
		return;

	// CPU-GPU 동기화: 현재 프레임의 이전 작업 완료 대기
	auto waitResult = pDevice->waitForFences(*inFlightFencesInst[currentFrameInst], VK_TRUE, UINT64_MAX);
	pDevice->resetFences(*inFlightFencesInst[currentFrameInst]);

	// 스왑체인에서 다음 이미지 획득
	auto [result, imageIndex] =
		swapChainInst.acquireNextImage(UINT64_MAX, *imageAvailableSemaphoresInst[currentFrameInst]);

	auto& commandBuffer = commandBuffersInst[currentFrameInst];
	commandBuffer.reset();
	commandBuffer.begin(vk::CommandBufferBeginInfo{});

	// Storage Image를 General 레이아웃으로 전환
	vk::ImageMemoryBarrier storageBarrier{
		.srcAccessMask	  = vk::AccessFlagBits::eNone,
		.dstAccessMask	  = vk::AccessFlagBits::eShaderWrite,
		.oldLayout		  = vk::ImageLayout::eUndefined,
		.newLayout		  = vk::ImageLayout::eGeneral,
		.image			  = *storageImageInst,
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, storageBarrier);

	// Compute Shader 디스패치
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipelineInst);
	commandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eCompute, *computePipelineLayoutInst, 0, *computeDescriptorSetInst, {});
	commandBuffer.dispatch((swapChainExtentInst.width + 15) / 16, (swapChainExtentInst.height + 15) / 16, 1);

	// Storage Image → TransferSrc, SwapChain Image → TransferDst 전환
	vk::ImageMemoryBarrier storageToSrcBarrier{
		.srcAccessMask	  = vk::AccessFlagBits::eShaderWrite,
		.dstAccessMask	  = vk::AccessFlagBits::eTransferRead,
		.oldLayout		  = vk::ImageLayout::eGeneral,
		.newLayout		  = vk::ImageLayout::eTransferSrcOptimal,
		.image			  = *storageImageInst,
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
	vk::ImageMemoryBarrier swapChainToDstBarrier{
		.srcAccessMask	  = vk::AccessFlagBits::eNone,
		.dstAccessMask	  = vk::AccessFlagBits::eTransferWrite,
		.oldLayout		  = vk::ImageLayout::eUndefined,
		.newLayout		  = vk::ImageLayout::eTransferDstOptimal,
		.image			  = swapChainImagesInst[imageIndex],
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
	std::array<vk::ImageMemoryBarrier, 2> blitBarriers = {storageToSrcBarrier, swapChainToDstBarrier};
	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, blitBarriers);

	// Storage Image → SwapChain Image blit
	vk::ImageBlit blitRegion{
		.srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		.srcOffsets =
			std::array<vk::Offset3D, 2>{
				vk::Offset3D{0, 0, 0},
				vk::Offset3D{
					static_cast<int32_t>(swapChainExtentInst.width), static_cast<int32_t>(swapChainExtentInst.height),
					1}},
		.dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		.dstOffsets		= std::array<vk::Offset3D, 2>{
			vk::Offset3D{0, 0, 0},
			vk::Offset3D{
				static_cast<int32_t>(swapChainExtentInst.width), static_cast<int32_t>(swapChainExtentInst.height), 1}}};
	commandBuffer.blitImage(
		*storageImageInst, vk::ImageLayout::eTransferSrcOptimal, swapChainImagesInst[imageIndex],
		vk::ImageLayout::eTransferDstOptimal, blitRegion, vk::Filter::eNearest);

	// SwapChain Image → PresentSrc 전환
	vk::ImageMemoryBarrier presentBarrier{
		.srcAccessMask	  = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask	  = vk::AccessFlagBits::eNone,
		.oldLayout		  = vk::ImageLayout::eTransferDstOptimal,
		.newLayout		  = vk::ImageLayout::ePresentSrcKHR,
		.image			  = swapChainImagesInst[imageIndex],
		.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, presentBarrier);

	commandBuffer.end();

	// 커맨드 버퍼 제출
	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eComputeShader;
	vk::SubmitInfo submitInfo{
		.waitSemaphoreCount	  = 1,
		.pWaitSemaphores	  = &*imageAvailableSemaphoresInst[currentFrameInst],
		.pWaitDstStageMask	  = &waitStage,
		.commandBufferCount	  = 1,
		.pCommandBuffers	  = &*commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores	  = &*renderFinishedSemaphoresInst[currentFrameInst]};
	pGraphicsQueue->submit(submitInfo, *inFlightFencesInst[currentFrameInst]);

	// 프레젠트
	vk::PresentInfoKHR presentInfo{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores	= &*renderFinishedSemaphoresInst[currentFrameInst],
		.swapchainCount		= 1,
		.pSwapchains		= &*swapChainInst,
		.pImageIndices		= &imageIndex};
	auto presentResult = pGraphicsQueue->presentKHR(presentInfo);

	currentFrameInst = (currentFrameInst + 1) % static_cast<uint32_t>(swapChainImagesInst.size());
}

// B8G8R8A8_SRGB + SRGB_NONLINEAR 우선 선택, 없으면 첫 번째 포맷 사용
vk::SurfaceFormatKHR VulkanSwapChain::chooseSurfaceFormat(
	const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
{
	for (const auto& format : availableFormats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			return format;
	}
	return availableFormats[0];
}

// Mailbox(트리플 버퍼링) 우선 선택, 없으면 FIFO(수직동기화) 사용
vk::PresentModeKHR VulkanSwapChain::choosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const
{
	for (const auto& mode : availableModes)
	{
		if (mode == vk::PresentModeKHR::eMailbox)
			return mode;
	}
	return vk::PresentModeKHR::eFifo;
}

// 현재 해상도 사용, 레티나/HiDPI 환경에서는 glfwGetFramebufferSize로 실제 픽셀 해상도 조회
vk::Extent2D VulkanSwapChain::chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;

	int framebufferWidth  = 0;
	int framebufferHeight = 0;
	glfwGetFramebufferSize(windowRef, &framebufferWidth, &framebufferHeight);

	vk::Extent2D extent = {static_cast<uint32_t>(framebufferWidth), static_cast<uint32_t>(framebufferHeight)};
	extent.width  = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return extent;
}
