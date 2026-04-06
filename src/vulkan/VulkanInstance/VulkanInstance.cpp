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
	if (!ENSURE(glfwExtensions))
		return;

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		// Validation Layer 지원 여부 확인
		auto availableLayers = vk::enumerateInstanceLayerProperties();
		for (const char* layerName : validationLayers)
		{
			bool found = false;
			for (const auto& layer : availableLayers)
			{
				if (strcmp(layerName, layer.layerName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!ENSURE(found))
			{
				std::cerr << "[VulkanInstance] Validation layer not found: " << layerName << std::endl;
				return;
			}
		}
	}

	vk::InstanceCreateInfo instanceCreateInfo{
		.pApplicationInfo        = &applicationInfo,
		.enabledLayerCount       = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
		.ppEnabledLayerNames     = enableValidationLayers ? validationLayers.data() : nullptr,
		.enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};

	instanceInst = vk::raii::Instance(contextInst, instanceCreateInfo);
	if (!ENSURE(*instanceInst))
		return;

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
	if (!ENSURE(*debugMessengerInst))
	{
		std::cerr << "[VulkanInstance] Failed to create debug messenger" << std::endl;
	}
}

vk::Bool32 VulkanInstance::debugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageType,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (!ENSURE(pCallbackData))
		return VK_FALSE;

	if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
	{
		std::cerr << "[Vulkan Validation] " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}
