#pragma once

#include "3D_MVP.h"
#include "interface/QRenderObject.h"

class VulkanSurface : public QRenderObject
{
public:
	VulkanSurface(class VulkanInstance* vulkanInstanceRef, GLFWwindow* windowRef);
	virtual ~VulkanSurface() override;

	void create() override;

	const vk::raii::SurfaceKHR* getSurfaceInst() const { return &surfaceInst; }

private:
	const class VulkanInstance* vulkanInstanceRef = nullptr;
	GLFWwindow* windowRef = nullptr;
	vk::raii::SurfaceKHR surfaceInst = nullptr;
};
