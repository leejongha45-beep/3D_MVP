#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

#include <vector>

class VulkanCommandPool : public QRenderObject
{
public:
	VulkanCommandPool(class VulkanDevice* vulkanDeviceRef, class VulkanSwapChain* vulkanSwapChainRef);
	virtual ~VulkanCommandPool() override;

	void create() override;

	const vk::raii::CommandPool* getCommandPoolInst() const { return &commandPoolInst; }
	const vk::raii::CommandBuffer* getCommandBufferInst(uint32_t index) const;
	uint32_t getCommandBufferCount() const { return static_cast<uint32_t>(commandBuffersInst.size()); }

private:
	const class VulkanDevice* vulkanDeviceRef = nullptr;
	const class VulkanSwapChain* vulkanSwapChainRef = nullptr;

	vk::raii::CommandPool commandPoolInst = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffersInst;
};
