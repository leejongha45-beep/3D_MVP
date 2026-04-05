#include "VulkanDevice.h"
#include "vulkan/VulkanPhysicalDevice/VulkanPhysicalDevice.h"

#include <set>
#include <vector>

VulkanDevice::VulkanDevice(VulkanPhysicalDevice* vulkanPhysicalDeviceRef)
	: vulkanPhysicalDeviceRef(vulkanPhysicalDeviceRef)
{
	assert(vulkanPhysicalDeviceRef);
}

VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::create()
{
	if (!ENSURE(vulkanPhysicalDeviceRef))
		return;

	const vk::raii::PhysicalDevice* pPhysicalDevice = vulkanPhysicalDeviceRef->getPhysicalDeviceInst();
	if (!ENSURE(pPhysicalDevice))
		return;

	queueFamilyIndicesInst = findQueueFamilies(*pPhysicalDevice);
	assert(queueFamilyIndicesInst.isComplete());

	std::set<uint32_t> uniqueQueueFamilies = {
		queueFamilyIndicesInst.graphicsFamily,
		queueFamilyIndicesInst.computeFamily,
		queueFamilyIndicesInst.transferFamily
	};

	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo{
			.queueFamilyIndex = queueFamily,
			.queueCount       = 1,
			.pQueuePriorities = &queuePriority
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures = pPhysicalDevice->getFeatures();

	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	vk::DeviceCreateInfo deviceCreateInfo{
		.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos       = queueCreateInfos.data(),
		.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures        = &deviceFeatures
	};

	deviceInst = vk::raii::Device(*pPhysicalDevice, deviceCreateInfo);

	ENSURE(*deviceInst);

	graphicsQueueInst = deviceInst.getQueue(queueFamilyIndicesInst.graphicsFamily, 0);
	computeQueueInst = deviceInst.getQueue(queueFamilyIndicesInst.computeFamily, 0);
	transferQueueInst = deviceInst.getQueue(queueFamilyIndicesInst.transferFamily, 0);

	ENSURE(*graphicsQueueInst);
	ENSURE(*computeQueueInst);
	ENSURE(*transferQueueInst);
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(const vk::raii::PhysicalDevice& device) const
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

		if (indices.isComplete())
			break;
	}

	return indices;
}
