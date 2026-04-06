#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"
#include "utils/GpuStructs.h"

#include <vector>

class VulkanSwapChain : public QRenderObject
{
public:
	VulkanSwapChain(
		class VulkanDevice* vulkanDeviceRef, class VulkanPhysicalDevice* vulkanPhysicalDeviceRef,
		class VulkanSurface* vulkanSurfaceRef, class VulkanUniformBuffer* vulkanUniformBufferRef,
		class VulkanStorageBuffer* vulkanStorageBufferRef, GLFWwindow* windowRef);
	virtual ~VulkanSwapChain() override;

	void create() override;

	const vk::raii::SwapchainKHR* getSwapChainInst() const { return &swapChainInst; }
	const std::vector<vk::Image>& getSwapChainImagesInst() const { return swapChainImagesInst; }
	const vk::raii::Image* getStorageImageInst() const { return &storageImageInst; }
	const vk::Extent2D& getExtentInst() const { return swapChainExtentInst; }
	const vk::raii::DescriptorSet* getDescriptorSetInst() const { return &descriptorSetInst; }
	const vk::raii::PipelineLayout* getPipelineLayoutInst() const { return &pipelineLayoutInst; }
	const vk::raii::Pipeline* getWaveSimulationPipelineInst() const { return &waveSimulationPipelineInst; }
	const vk::raii::Pipeline* getRenderPipelineInst() const { return &renderPipelineInst; }
	uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImagesInst.size()); }

private:
	vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
	vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& availableModes) const;
	vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
	void createSwapChain(
		const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice,
		const vk::raii::SurfaceKHR& surface);
	void createImageViews(const vk::raii::Device& device);
	void createStorageImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
	void createDescriptorSetLayout(const vk::raii::Device& device);
	void createComputePipelines(const vk::raii::Device& device);
	void createDescriptorPoolAndSet(const vk::raii::Device& device);
	vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::vector<uint32_t>& code) const;
	std::vector<uint32_t> readShaderFile(const std::string& filePath) const;
	uint32_t findMemoryType(
		const vk::raii::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

	const class VulkanDevice* vulkanDeviceRef				  = nullptr;
	const class VulkanPhysicalDevice* vulkanPhysicalDeviceRef = nullptr;
	const class VulkanSurface* vulkanSurfaceRef				  = nullptr;
	class VulkanUniformBuffer* vulkanUniformBufferRef		  = nullptr;
	class VulkanStorageBuffer* vulkanStorageBufferRef	  = nullptr;
	GLFWwindow* windowRef									  = nullptr;

	vk::Format swapChainFormatInst				= vk::Format::eUndefined;
	vk::PresentModeKHR swapChainPresentModeInst = vk::PresentModeKHR::eFifo;
	vk::Extent2D swapChainExtentInst;

	vk::raii::SwapchainKHR swapChainInst = nullptr;
	std::vector<vk::Image> swapChainImagesInst;
	std::vector<vk::raii::ImageView> swapChainImageViewsInst;

	vk::raii::Image storageImageInst			  = nullptr;
	vk::raii::DeviceMemory storageImageMemoryInst = nullptr;
	vk::raii::ImageView storageImageViewInst	  = nullptr;

	vk::raii::DescriptorSetLayout descriptorSetLayoutInst = nullptr;
	vk::raii::PipelineLayout pipelineLayoutInst = nullptr;
	vk::raii::Pipeline waveSimulationPipelineInst = nullptr;
	vk::raii::Pipeline renderPipelineInst = nullptr;

	vk::raii::DescriptorPool descriptorPoolInst = nullptr;
	vk::raii::DescriptorSet descriptorSetInst = nullptr;
};
