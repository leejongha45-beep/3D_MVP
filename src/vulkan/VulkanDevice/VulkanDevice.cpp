#include "VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"
#include "vulkan/VulkanSurface/VulkanSurface.h"

#include <set>
#include <vector>

VulkanDevice::VulkanDevice(VulkanPhysicalDevice* vulkanPhysicalDeviceRef, VulkanSurface* vulkanSurfaceRef)
	: vulkanPhysicalDeviceRef(vulkanPhysicalDeviceRef), vulkanSurfaceRef(vulkanSurfaceRef)
{
	assert(vulkanPhysicalDeviceRef);
	assert(vulkanSurfaceRef);
}

VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::create()
{
	if (!ENSURE(vulkanPhysicalDeviceRef))
		return;
	if (!ENSURE(vulkanSurfaceRef))
		return;

	const vk::raii::PhysicalDevice* pPhysicalDevice = vulkanPhysicalDeviceRef->getPhysicalDeviceInst();
	if (!ENSURE(pPhysicalDevice))
		return;

	const vk::raii::SurfaceKHR* pSurface = vulkanSurfaceRef->getSurfaceInst();
	if (!ENSURE(pSurface))
		return;

	queueFamilyIndicesInst = findQueueFamilies(*pPhysicalDevice, *pSurface);
	if (!ENSURE(queueFamilyIndicesInst.isComplete()))
	{
		std::cerr << "[VulkanDevice] Failed to find all required queue families" << std::endl;
		return;
	}

	// 중복 제거된 큐 패밀리 인덱스
	std::set<uint32_t> uniqueQueueFamilies = {
		queueFamilyIndicesInst.graphicsFamily, queueFamilyIndicesInst.computeFamily,
		queueFamilyIndicesInst.transferFamily, queueFamilyIndicesInst.presentFamily};

	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo{
			.queueFamilyIndex = queueFamily, .queueCount = 1, .pQueuePriorities = &queuePriority};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// GPU가 지원하는 피처를 pNext 체인으로 쿼리
	auto supportedFeatures2 = pPhysicalDevice->getFeatures2<
		vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan12Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceVulkan14Features>();

	auto& supported11Features = supportedFeatures2.get<vk::PhysicalDeviceVulkan11Features>();
	auto& supported12Features = supportedFeatures2.get<vk::PhysicalDeviceVulkan12Features>();
	auto& supported13Features = supportedFeatures2.get<vk::PhysicalDeviceVulkan13Features>();
	auto& supported14Features = supportedFeatures2.get<vk::PhysicalDeviceVulkan14Features>();

	// 쿼리된 피처를 그대로 활성화 (GPU가 지원하는 것만 켜짐)
	vk::PhysicalDeviceVulkan11Features vulkan11Features = supported11Features;
	vulkan11Features.pNext = nullptr;
	vk::PhysicalDeviceVulkan12Features vulkan12Features = supported12Features;
	vulkan12Features.pNext = &vulkan11Features;
	vk::PhysicalDeviceVulkan13Features vulkan13Features = supported13Features;
	vulkan13Features.pNext = &vulkan12Features;
	vk::PhysicalDeviceVulkan14Features vulkan14Features = supported14Features;
	vulkan14Features.pNext = &vulkan13Features;

	std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	vk::DeviceCreateInfo deviceCreateInfo{
		.pNext                   = &vulkan14Features,
		.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos       = queueCreateInfos.data(),
		.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures        = &supportedFeatures2.get<vk::PhysicalDeviceFeatures2>().features
	};

	deviceInst = vk::raii::Device(*pPhysicalDevice, deviceCreateInfo);
	if (!ENSURE(*deviceInst))
	{
		std::cerr << "[VulkanDevice] Failed to create logical device" << std::endl;
		return;
	}

	graphicsQueueInst = deviceInst.getQueue(queueFamilyIndicesInst.graphicsFamily, 0);
	computeQueueInst  = deviceInst.getQueue(queueFamilyIndicesInst.computeFamily, 0);
	transferQueueInst = deviceInst.getQueue(queueFamilyIndicesInst.transferFamily, 0);
	presentQueueInst  = deviceInst.getQueue(queueFamilyIndicesInst.presentFamily, 0);

	if (!ENSURE(*graphicsQueueInst))
		std::cerr << "[VulkanDevice] Failed to get graphics queue" << std::endl;
	if (!ENSURE(*computeQueueInst))
		std::cerr << "[VulkanDevice] Failed to get compute queue" << std::endl;
	if (!ENSURE(*transferQueueInst))
		std::cerr << "[VulkanDevice] Failed to get transfer queue" << std::endl;
	if (!ENSURE(*presentQueueInst))
		std::cerr << "[VulkanDevice] Failed to get present queue" << std::endl;
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(
	const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) const
{
	QueueFamilyIndices indices;
	auto queueFamilies = device.getQueueFamilyProperties();

	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i)
	{
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
			indices.graphicsFamily = i;

		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)
			indices.computeFamily = i;

		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer)
			indices.transferFamily = i;

		if (device.getSurfaceSupportKHR(i, *surface))
			indices.presentFamily = i;

		if (indices.isComplete())
			break;
	}

	return indices;
}
