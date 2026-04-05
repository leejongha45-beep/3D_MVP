#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

class VulkanDevice : public QRenderObject
{
public:
	VulkanDevice(class VulkanPhysicalDevice* vulkanPhysicalDeviceRef);
	virtual ~VulkanDevice() override;

	void create() override;

	const vk::raii::Device* getDeviceInst() const { return &deviceInst; }
	const vk::raii::Queue* getGraphicsQueueInst() const { return &graphicsQueueInst; }
	uint32_t getGraphicsFamily() const { return queueFamilyIndicesInst.graphicsFamily; }
	uint32_t getComputeFamily() const { return queueFamilyIndicesInst.computeFamily; }

private:
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily = UINT32_MAX;
		uint32_t computeFamily = UINT32_MAX;
		uint32_t transferFamily = UINT32_MAX;

		bool isComplete() const
		{
			return graphicsFamily != UINT32_MAX
				&& computeFamily != UINT32_MAX
				&& transferFamily != UINT32_MAX;
		}
	};

	QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device) const;

	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;
	vk::raii::Device deviceInst = nullptr;
	vk::raii::Queue graphicsQueueInst = nullptr;
	vk::raii::Queue computeQueueInst = nullptr;
	vk::raii::Queue transferQueueInst = nullptr;
	QueueFamilyIndices queueFamilyIndicesInst;
};
