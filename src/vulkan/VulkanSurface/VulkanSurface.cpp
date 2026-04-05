#include "VulkanSurface.h"
#include "vulkan/VulkanInstance/VulkanInstance.h"

VulkanSurface::VulkanSurface(VulkanInstance* vulkanInstanceRef, GLFWwindow* windowRef)
	: vulkanInstanceRef(vulkanInstanceRef), windowRef(windowRef)
{
	assert(vulkanInstanceRef);
	assert(windowRef);
}

VulkanSurface::~VulkanSurface()
{
}

void VulkanSurface::create()
{
	if (!ENSURE(vulkanInstanceRef))
		return;
	if (!ENSURE(windowRef))
		return;

	const vk::raii::Instance* pInstance = vulkanInstanceRef->getInstanceInst();
	if (!ENSURE(pInstance))
		return;

	VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
	VkResult result = glfwCreateWindowSurface(**pInstance, windowRef, nullptr, &rawSurface);
	assert(result == VK_SUCCESS);

	surfaceInst = vk::raii::SurfaceKHR(*pInstance, rawSurface);
}
