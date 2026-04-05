#include "VulkanInstance.h"

VulkanInstance::VulkanInstance()
{
}

VulkanInstance::~VulkanInstance()
{
}

void VulkanInstance::create()
{
	vk::ApplicationInfo applicationInfo{
		.pApplicationName   = "QuantumWave Renderer",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName        = "QWave",
		.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion         = VK_API_VERSION_1_4
	};

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	assert(glfwExtensions);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	vk::InstanceCreateInfo instanceCreateInfo{
		.pApplicationInfo        = &applicationInfo,
		.enabledLayerCount       = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
		.ppEnabledLayerNames     = enableValidationLayers ? validationLayers.data() : nullptr,
		.enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};

	instanceInst = vk::raii::Instance(contextInst, instanceCreateInfo);

	if (enableValidationLayers)
	{
		setupDebugMessenger();
	}
}

void VulkanInstance::setupDebugMessenger()
{
	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
						 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
						 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		.messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
						 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
						 | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		.pfnUserCallback = debugCallback
	};

	debugMessengerInst = vk::raii::DebugUtilsMessengerEXT(instanceInst, debugCreateInfo);
}

vk::Bool32 VulkanInstance::debugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageType,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		if (ENSURE(pCallbackData))
		{
			std::cerr << "[Vulkan Validation] " << pCallbackData->pMessage << std::endl;
		}
	}
	return VK_FALSE;
}
