#include "VulkanPhysicalDevice.h"
#include "vulkan/VulkanInstance/VulkanInstance.h"
#include "vulkan/VulkanSurface/VulkanSurface.h"

#include <ranges>
#include <set>
#include <string>

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance* vulkanInstanceRef, VulkanSurface* vulkanSurfaceRef)
	: vulkanInstanceRef(vulkanInstanceRef), vulkanSurfaceRef(vulkanSurfaceRef)
{
	assert(vulkanInstanceRef);
	assert(vulkanSurfaceRef);
}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{
}

void VulkanPhysicalDevice::create()
{
	if (!ENSURE(vulkanInstanceRef))
		return;
	if (!ENSURE(vulkanSurfaceRef))
		return;

	const vk::raii::Instance* pInstance = vulkanInstanceRef->getInstanceInst();
	if (!ENSURE(pInstance))
		return;

	vk::raii::PhysicalDevices physicalDevices(*pInstance);
	if (!ENSURE(!physicalDevices.empty()))
	{
		std::cerr << "[VulkanPhysicalDevice] No physical devices found" << std::endl;
		return;
	}

	auto suitableDeviceView = std::ranges::filter_view(
		physicalDevices,
		[this](const vk::raii::PhysicalDevice& device)
		{
			return isDeviceSuitable(device);
		});

	std::vector<vk::raii::PhysicalDevice*> suitableDevices;
	for (auto& device : suitableDeviceView)
	{
		suitableDevices.push_back(&device);
	}

	if (!ENSURE(!suitableDevices.empty()))
	{
		std::cerr << "[VulkanPhysicalDevice] No suitable physical device found" << std::endl;
		return;
	}

	std::cout << "=== Suitable Physical Devices ===" << std::endl;
	for (size_t i = 0; i < suitableDevices.size(); ++i)
	{
		vk::PhysicalDeviceProperties properties = suitableDevices[i]->getProperties();
		std::cout << "  [" << i << "] " << vk::to_string(properties.deviceType) << " - " << properties.deviceName << std::endl;
	}

	int selection = 0;

	physicalDeviceInst = std::move(*suitableDevices[selection]);
	std::cout << "=== Selected: " << physicalDeviceInst.getProperties().deviceName << " ===" << std::endl;
}

bool VulkanPhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice& device) const
{
	bool isDiscreteGpu = device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
	return isDiscreteGpu
		&& hasRequiredQueueFamilies(device)
		&& hasRequiredFeatures(device)
		&& hasRequiredExtensions(device)
		&& hasSurfacePresentSupport(device);
}

bool VulkanPhysicalDevice::hasRequiredQueueFamilies(const vk::raii::PhysicalDevice& device) const
{
	auto queueFamilies = device.getQueueFamilyProperties();

	bool hasGraphics = false;
	bool hasCompute = false;
	bool hasTransfer = false;

	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			hasGraphics = true;
		if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
			hasCompute = true;
		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			hasTransfer = true;
	}

	return hasGraphics && hasCompute && hasTransfer;
}

bool VulkanPhysicalDevice::hasRequiredFeatures(const vk::raii::PhysicalDevice& device) const
{
	vk::PhysicalDeviceFeatures features = device.getFeatures();

	return features.geometryShader
		&& features.tessellationShader
		&& features.samplerAnisotropy
		&& features.fillModeNonSolid
		&& features.multiDrawIndirect
		&& features.shaderFloat64
		&& features.shaderInt64
		&& features.shaderStorageBufferArrayDynamicIndexing
		&& features.shaderStorageImageArrayDynamicIndexing
		&& features.shaderUniformBufferArrayDynamicIndexing
		&& features.fragmentStoresAndAtomics
		&& features.vertexPipelineStoresAndAtomics
		&& features.shaderImageGatherExtended
		&& features.largePoints
		&& features.wideLines
		&& features.shaderStorageImageWriteWithoutFormat;
}

bool VulkanPhysicalDevice::hasRequiredExtensions(const vk::raii::PhysicalDevice& device) const
{
	std::set<std::string> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	auto availableExtensions = device.enumerateDeviceExtensionProperties();
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool VulkanPhysicalDevice::hasSurfacePresentSupport(const vk::raii::PhysicalDevice& device) const
{
	const vk::raii::SurfaceKHR* pSurface = vulkanSurfaceRef->getSurfaceInst();
	if (!ENSURE(pSurface))
		return false;

	auto queueFamilies = device.getQueueFamilyProperties();
	for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i)
	{
		if (device.getSurfaceSupportKHR(i, **pSurface))
			return true;
	}

	return false;
}
