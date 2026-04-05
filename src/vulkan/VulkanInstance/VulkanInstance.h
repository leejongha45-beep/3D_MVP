#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

#include <vector>

class VulkanInstance : public QRenderObject
{
public:
	VulkanInstance();
	virtual ~VulkanInstance() override;

	void create() override;

	const vk::raii::Instance* getInstanceInst() const { return &instanceInst; }

private:
	void setupDebugMessenger();

	static vk::Bool32 debugCallback(
		vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		vk::DebugUtilsMessageTypeFlagsEXT messageType,
		const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	vk::raii::Context contextInst;
	vk::raii::Instance instanceInst = nullptr;
	vk::raii::DebugUtilsMessengerEXT debugMessengerInst = nullptr;

#ifdef NDEBUG
	static constexpr bool enableValidationLayers = false;
#else
	static constexpr bool enableValidationLayers = true;
#endif

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
};
