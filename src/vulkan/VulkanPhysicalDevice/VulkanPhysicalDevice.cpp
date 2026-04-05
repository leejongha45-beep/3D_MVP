#include "VulkanPhysicalDevice.h"
#include "vulkan/VulkanInstance/VulkanInstance.h"

#include <ranges>
#include <set>
#include <string>

VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance* vulkanInstanceRef) : vulkanInstanceRef(vulkanInstanceRef)
{
	assert(vulkanInstanceRef);
}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{
}

void VulkanPhysicalDevice::create()
{
	if (!ENSURE(vulkanInstanceRef))
		return;

	const vk::raii::Instance* pInstance = vulkanInstanceRef->getInstanceInst();
	if (!ENSURE(pInstance))
		return;

	vk::raii::PhysicalDevices physicalDevices(*pInstance);
	assert(!physicalDevices.empty());

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

	if (suitableDevices.empty())
	{
		std::cerr << "No suitable physical device found!" << std::endl;
		return;
	}

	std::cout << "=== Suitable Physical Devices ===" << std::endl;
	for (size_t i = 0; i < suitableDevices.size(); ++i)
	{
		vk::PhysicalDeviceProperties properties = suitableDevices[i]->getProperties();
		std::cout << "  [" << i << "] " << vk::to_string(properties.deviceType) << " - " << properties.deviceName << std::endl;
	}
	std::cout << "Select device (0 = default): ";

	int selection = 0;
	std::cin >> selection;

	if (selection < 0 || selection >= static_cast<int>(suitableDevices.size()))
		selection = 0;

	physicalDeviceInst = std::move(*suitableDevices[selection]);
	std::cout << "=== Selected: " << physicalDeviceInst.getProperties().deviceName << " ===" << std::endl;
}

bool VulkanPhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice& device) const
{
	bool isDiscreteGpu = device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
	return isDiscreteGpu
		&& hasRequiredQueueFamilies(device)
		&& hasRequiredFeatures(device)
		&& hasRequiredExtensions(device);
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
		&& features.multiDrawIndirect;
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
