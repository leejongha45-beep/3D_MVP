#include "VulkanCommandPool.h"
#include "vulkan/VulkanDevice/VulkanDevice.h"
#include "vulkan/VulkanSwapChain/VulkanSwapChain.h"

VulkanCommandPool::VulkanCommandPool(VulkanDevice* vulkanDeviceRef, VulkanSwapChain* vulkanSwapChainRef)
	: vulkanDeviceRef(vulkanDeviceRef), vulkanSwapChainRef(vulkanSwapChainRef)
{
	assert(vulkanDeviceRef);
	assert(vulkanSwapChainRef);
}

VulkanCommandPool::~VulkanCommandPool()
{
}

void VulkanCommandPool::create()
{
	if (!ENSURE(vulkanDeviceRef))
		return;
	if (!ENSURE(vulkanSwapChainRef))
		return;

	const vk::raii::Device* pDevice = vulkanDeviceRef->getDeviceInst();
	if (!ENSURE(pDevice))
		return;

	uint32_t imageCount = vulkanSwapChainRef->getImageCount();
	if (!ENSURE(imageCount > 0))
	{
		std::cerr << "[VulkanCommandPool] Swap chain image count is 0" << std::endl;
		return;
	}

	vk::CommandPoolCreateInfo commandPoolCreateInfo{
		.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = vulkanDeviceRef->getGraphicsFamily()
	};

	commandPoolInst = vk::raii::CommandPool(*pDevice, commandPoolCreateInfo);
	if (!ENSURE(*commandPoolInst))
	{
		std::cerr << "[VulkanCommandPool] Failed to create command pool" << std::endl;
		return;
	}

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
		.commandPool        = *commandPoolInst,
		.level              = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = imageCount
	};

	commandBuffersInst = pDevice->allocateCommandBuffers(commandBufferAllocateInfo);
	if (!ENSURE(!commandBuffersInst.empty()))
	{
		std::cerr << "[VulkanCommandPool] Failed to allocate command buffers" << std::endl;
	}
}

const vk::raii::CommandBuffer* VulkanCommandPool::getCommandBufferInst(uint32_t index) const
{
	if (!ENSURE(index < commandBuffersInst.size()))
		return nullptr;

	return &commandBuffersInst[index];
}
