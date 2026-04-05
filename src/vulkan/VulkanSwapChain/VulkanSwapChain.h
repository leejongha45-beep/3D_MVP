#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"
#include "utils/GpuStructs.h"

#include <vector>

class VulkanSwapChain : public QRenderObject
{
public:
	VulkanSwapChain(
		class VulkanDevice* vulkanDeviceRef,
		class VulkanPhysicalDevice* vulkanPhysicalDeviceRef,
		class VulkanSurface* vulkanSurfaceRef,
		GLFWwindow* windowRef);
	virtual ~VulkanSwapChain() override;

	void create() override;
	void drawFrame();
	void updateSceneData(const SceneData& sceneData);

private:
	vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
	vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const;
	vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
	void createImageViews(const vk::raii::Device& device);
	void createCommandPoolAndBuffers(const vk::raii::Device& device);
	void createStorageImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
	void createComputePipeline(const vk::raii::Device& device);
	void createSyncObjects(const vk::raii::Device& device);
	void createUniformBuffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
	void createDescriptorSet(const vk::raii::Device& device);
	uint32_t findMemoryType(const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
	vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::vector<uint32_t>& code) const;
	std::vector<uint32_t> readShaderFile(const std::string& filePath) const;

	const class VulkanDevice* vulkanDeviceRef = nullptr;
	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;
	const class VulkanSurface* vulkanSurfaceRef = nullptr;
	GLFWwindow* windowRef = nullptr;
	vk::raii::SwapchainKHR swapChainInst = nullptr;
	vk::Format swapChainFormatInst = vk::Format::eUndefined;
	vk::Extent2D swapChainExtentInst;
	std::vector<vk::Image> swapChainImagesInst;
	std::vector<vk::raii::ImageView> swapChainImageViewsInst;
	vk::raii::CommandPool commandPoolInst = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffersInst;
	vk::raii::Image storageImageInst = nullptr;
	vk::raii::DeviceMemory storageImageMemoryInst = nullptr;
	vk::raii::ImageView storageImageViewInst = nullptr;
	vk::raii::Buffer uniformBufferInst = nullptr;
	vk::raii::DeviceMemory uniformBufferMemoryInst = nullptr;
	void* uniformBufferMappedInst = nullptr;
	vk::raii::DescriptorSetLayout computeDescriptorSetLayoutInst = nullptr;
	vk::raii::PipelineLayout computePipelineLayoutInst = nullptr;
	vk::raii::Pipeline computePipelineInst = nullptr;
	vk::raii::DescriptorPool descriptorPoolInst = nullptr;
	vk::raii::DescriptorSet computeDescriptorSetInst = nullptr;
	std::vector<vk::raii::Semaphore> imageAvailableSemaphoresInst;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphoresInst;
	std::vector<vk::raii::Fence> inFlightFencesInst;
	static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrameInst = 0;
};
