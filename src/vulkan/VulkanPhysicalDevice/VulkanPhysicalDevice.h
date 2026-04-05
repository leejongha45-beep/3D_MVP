#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

class VulkanPhysicalDevice : public QRenderObject
{
public:
	VulkanPhysicalDevice(class VulkanInstance* vulkanInstanceRef);
	virtual ~VulkanPhysicalDevice() override;

	void create() override;

	const vk::raii::PhysicalDevice* getPhysicalDeviceInst() const { return &physicalDeviceInst; }

private:
	bool isDeviceSuitable(const vk::raii::PhysicalDevice& device) const;
	bool hasRequiredQueueFamilies(const vk::raii::PhysicalDevice& device) const;
	bool hasRequiredFeatures(const vk::raii::PhysicalDevice& device) const;
	bool hasRequiredExtensions(const vk::raii::PhysicalDevice& device) const;

	const class VulkanInstance* vulkanInstanceRef = nullptr;
	vk::raii::PhysicalDevice physicalDeviceInst = nullptr;
};
