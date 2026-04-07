#include "VulkanSwapChain.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"
#include "vulkan/VulkanSurface/VulkanSurface.h"
#include "vulkan/buffer/VulkanUniformBuffer.h"
#include "vulkan/buffer/VulkanStorageBuffer.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <string>

VulkanSwapChain::VulkanSwapChain(
	VulkanDevice* vulkanDeviceRef, VulkanPhysicalDevice* vulkanPhysicalDeviceRef, VulkanSurface* vulkanSurfaceRef,
	VulkanUniformBuffer* vulkanUniformBufferRef, VulkanStorageBuffer* vulkanStorageBufferRef, GLFWwindow* windowRef)
	: vulkanDeviceRef(vulkanDeviceRef), vulkanPhysicalDeviceRef(vulkanPhysicalDeviceRef),
	  vulkanSurfaceRef(vulkanSurfaceRef), vulkanUniformBufferRef(vulkanUniformBufferRef),
	  vulkanStorageBufferRef(vulkanStorageBufferRef), windowRef(windowRef)
{
	assert(vulkanDeviceRef);
	assert(vulkanPhysicalDeviceRef);
	assert(vulkanSurfaceRef);
	assert(vulkanUniformBufferRef);
	assert(vulkanStorageBufferRef);
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

	createSwapChain(*pDevice, *pPhysicalDevice, *pSurface);
	createImageViews(*pDevice);
	createStorageImage(*pDevice, *pPhysicalDevice);
	createDescriptorSetLayout(*pDevice);
	createComputePipelines(*pDevice);
	createDescriptorPoolAndSet(*pDevice);
}

void VulkanSwapChain::createSwapChain(
	const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface)
{
	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

	std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(*surface);
	if (!ENSURE(!formats.empty()))
	{
		std::cerr << "[VulkanSwapChain] No surface formats available" << std::endl;
		return;
	}

	std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
	if (!ENSURE(!presentModes.empty()))
	{
		std::cerr << "[VulkanSwapChain] No present modes available" << std::endl;
		return;
	}

	vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
	swapChainFormatInst				   = surfaceFormat.format;
	swapChainPresentModeInst		   = choosePresentMode(presentModes);
	swapChainExtentInst				   = chooseExtent(capabilities);

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	uint32_t graphicsFamily					   = vulkanDeviceRef->getGraphicsFamily();
	uint32_t presentFamily					   = vulkanDeviceRef->getPresentFamily();
	std::array<uint32_t, 2> queueFamilyIndices = {graphicsFamily, presentFamily};

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{
		.surface		  = *surface,
		.minImageCount	  = imageCount,
		.imageFormat	  = surfaceFormat.format,
		.imageColorSpace  = surfaceFormat.colorSpace,
		.imageExtent	  = swapChainExtentInst,
		.imageArrayLayers = 1,
		.imageUsage		  = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
		.imageSharingMode =
			(graphicsFamily != presentFamily) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
		.queueFamilyIndexCount =
			(graphicsFamily != presentFamily) ? static_cast<uint32_t>(queueFamilyIndices.size()) : 0,
		.pQueueFamilyIndices = (graphicsFamily != presentFamily) ? queueFamilyIndices.data() : nullptr,
		.preTransform		 = capabilities.currentTransform,
		.compositeAlpha		 = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode		 = swapChainPresentModeInst,
		.clipped			 = VK_TRUE};

	swapChainInst = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
	if (!ENSURE(*swapChainInst))
	{
		std::cerr << "[VulkanSwapChain] Failed to create swap chain" << std::endl;
		return;
	}

	swapChainImagesInst = swapChainInst.getImages();
}

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

// Compute Shader 출력용 Storage Image 생성 (스왑체인과 동일 해상도, eStorage + eTransferSrc)
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
	if (!ENSURE(memoryTypeIndex != UINT32_MAX))
		return;

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

uint32_t VulkanSwapChain::findMemoryType(
	const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
	vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	std::cerr << "[VulkanSwapChain] Failed to find suitable memory type" << std::endl;
	return UINT32_MAX;
}

// SoA: 0=StorageImage, 1=UBO, 2=fieldXYZ, 3=fieldW, 4=velocityXYZ, 5=velocityW, 6=accumCounter
void VulkanSwapChain::createDescriptorSetLayout(const vk::raii::Device& device)
{
	std::array<vk::DescriptorSetLayoutBinding, 9> bindings = {
		vk::DescriptorSetLayoutBinding{.binding = 0, .descriptorType = vk::DescriptorType::eStorageImage, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 2, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 3, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 4, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 5, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 6, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 7, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute},
		vk::DescriptorSetLayoutBinding{.binding = 8, .descriptorType = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eCompute}
	};

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings    = bindings.data()
	};

	descriptorSetLayoutInst = vk::raii::DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);
	if (!ENSURE(*descriptorSetLayoutInst))
	{
		std::cerr << "[VulkanSwapChain] Failed to create descriptor set layout" << std::endl;
	}
}

// PipelineLayout + 파동 시뮬레이션 파이프라인 + 렌더링 파이프라인 생성
void VulkanSwapChain::createComputePipelines(const vk::raii::Device& device)
{
	// PipelineLayout
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.setLayoutCount = 1,
		.pSetLayouts    = &*descriptorSetLayoutInst
	};

	pipelineLayoutInst = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);
	if (!ENSURE(*pipelineLayoutInst))
	{
		std::cerr << "[VulkanSwapChain] Failed to create pipeline layout" << std::endl;
		return;
	}

	// 파동 시뮬레이션 파이프라인
	auto waveShaderCode = readShaderFile(SHADER_DIR "wave_simulation.spv");
	if (!ENSURE(!waveShaderCode.empty()))
		return;

	auto waveShaderModule = createShaderModule(device, waveShaderCode);

	vk::PipelineShaderStageCreateInfo waveStageCreateInfo{
		.stage  = vk::ShaderStageFlagBits::eCompute,
		.module = *waveShaderModule,
		.pName  = "main"
	};

	vk::ComputePipelineCreateInfo waveComputeCreateInfo{
		.stage  = waveStageCreateInfo,
		.layout = *pipelineLayoutInst
	};

	waveSimulationPipelineInst = vk::raii::Pipeline(device, nullptr, waveComputeCreateInfo);
	if (!ENSURE(*waveSimulationPipelineInst))
	{
		std::cerr << "[VulkanSwapChain] Failed to create wave simulation pipeline" << std::endl;
		return;
	}

	// 렌더링 파이프라인
	auto renderShaderCode = readShaderFile(SHADER_DIR "render.spv");
	if (!ENSURE(!renderShaderCode.empty()))
		return;

	auto renderShaderModule = createShaderModule(device, renderShaderCode);

	vk::PipelineShaderStageCreateInfo renderStageCreateInfo{
		.stage  = vk::ShaderStageFlagBits::eCompute,
		.module = *renderShaderModule,
		.pName  = "main"
	};

	vk::ComputePipelineCreateInfo renderComputeCreateInfo{
		.stage  = renderStageCreateInfo,
		.layout = *pipelineLayoutInst
	};

	renderPipelineInst = vk::raii::Pipeline(device, nullptr, renderComputeCreateInfo);
	if (!ENSURE(*renderPipelineInst))
	{
		std::cerr << "[VulkanSwapChain] Failed to create render pipeline" << std::endl;
	}
}

// DescriptorPool 생성 + 리소스 바인딩
void VulkanSwapChain::createDescriptorPoolAndSet(const vk::raii::Device& device)
{
	// Pool: StorageImage 1 + UBO 1 + StorageBuffer 4 (SoA)
	std::array<vk::DescriptorPoolSize, 3> poolSizes = {
		vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 1},
		vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1},
		vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 7}
	};

	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets       = 1,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes    = poolSizes.data()
	};

	descriptorPoolInst = vk::raii::DescriptorPool(device, descriptorPoolCreateInfo);
	if (!ENSURE(*descriptorPoolInst))
	{
		std::cerr << "[VulkanSwapChain] Failed to create descriptor pool" << std::endl;
		return;
	}

	// Set 할당
	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
		.descriptorPool     = *descriptorPoolInst,
		.descriptorSetCount = 1,
		.pSetLayouts        = &*descriptorSetLayoutInst
	};

	auto descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);
	descriptorSetInst = std::move(descriptorSets[0]);

	// 리소스 바인딩
	if (!ENSURE(vulkanUniformBufferRef))
		return;
	if (!ENSURE(vulkanStorageBufferRef))
		return;

	vk::DescriptorImageInfo storageImageInfo{
		.imageView   = *storageImageViewInst,
		.imageLayout = vk::ImageLayout::eGeneral
	};

	vk::DescriptorBufferInfo uniformBufferInfo{
		.buffer = **(vulkanUniformBufferRef->getBufferInst()),
		.offset = 0,
		.range  = vulkanUniformBufferRef->getBufferSize()
	};

	vk::DescriptorBufferInfo fieldXYZInfo{
		.buffer = **(vulkanStorageBufferRef->getFieldXYZBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getFieldXYZBufferSize()
	};
	vk::DescriptorBufferInfo fieldWInfo{
		.buffer = **(vulkanStorageBufferRef->getFieldWBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getFieldWBufferSize()
	};
	vk::DescriptorBufferInfo velocityXYZInfo{
		.buffer = **(vulkanStorageBufferRef->getVelocityXYZBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getVelocityXYZBufferSize()
	};
	vk::DescriptorBufferInfo velocityWInfo{
		.buffer = **(vulkanStorageBufferRef->getVelocityWBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getVelocityWBufferSize()
	};
	vk::DescriptorBufferInfo accumCounterInfo{
		.buffer = **(vulkanStorageBufferRef->getAccumCounterBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getAccumCounterBufferSize()
	};
	vk::DescriptorBufferInfo normalInfo{
		.buffer = **(vulkanStorageBufferRef->getNormalBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getNormalBufferSize()
	};
	vk::DescriptorBufferInfo reflectedFieldXYZInfo{
		.buffer = **(vulkanStorageBufferRef->getReflectedFieldXYZBufferInst()),
		.offset = 0,
		.range  = vulkanStorageBufferRef->getReflectedFieldXYZBufferSize()
	};

	std::array<vk::WriteDescriptorSet, 9> writeDescriptorSets = {
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 0, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageImage, .pImageInfo = &storageImageInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 1, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &uniformBufferInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 2, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &fieldXYZInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 3, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &fieldWInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 4, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &velocityXYZInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 5, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &velocityWInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 6, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &accumCounterInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 7, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &normalInfo},
		vk::WriteDescriptorSet{.dstSet = *descriptorSetInst, .dstBinding = 8, .descriptorCount = 1, .descriptorType = vk::DescriptorType::eStorageBuffer, .pBufferInfo = &reflectedFieldXYZInfo}
	};

	device.updateDescriptorSets(writeDescriptorSets, nullptr);
}

std::vector<uint32_t> VulkanSwapChain::readShaderFile(const std::string& filePath) const
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!ENSURE(file.is_open()))
	{
		std::cerr << "[VulkanSwapChain] Failed to open shader file: " << filePath << std::endl;
		return {};
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

vk::raii::ShaderModule VulkanSwapChain::createShaderModule(const vk::raii::Device& device, const std::vector<uint32_t>& code) const
{
	if (!ENSURE(!code.empty()))
	{
		std::cerr << "[VulkanSwapChain] Shader code is empty" << std::endl;
		return nullptr;
	}

	vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
		.codeSize = code.size() * sizeof(uint32_t),
		.pCode    = code.data()
	};

	return vk::raii::ShaderModule(device, shaderModuleCreateInfo);
}

// B8G8R8A8_SRGB + SRGB_NONLINEAR 우선, 없으면 첫 번째
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

// Mailbox(트리플 버퍼링) 우선, 없으면 FIFO(수직동기화)
vk::PresentModeKHR VulkanSwapChain::choosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const
{
	for (const auto& mode : availableModes)
	{
		if (mode == vk::PresentModeKHR::eMailbox)
			return mode;
	}
	return vk::PresentModeKHR::eFifo;
}

// 현재 해상도 사용, HiDPI 환경에서는 glfwGetFramebufferSize로 실제 픽셀 해상도 조회
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
