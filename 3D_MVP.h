#pragma once

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#ifdef _WIN32
	#define VK_USE_PLATFORM_WIN32_KHR
	#define GLFW_EXPOSE_NATIVE_WIN32
#elif __APPLE__
	#define VK_USE_PLATFORM_MACOS_MVK
	#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cassert>
#include <iostream>

#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#endif
#include <vulkan/vulkan.hpp>

#define ENSURE(expr) \
    ([&]() -> bool { \
        if (!(expr)) { \
            std::cerr << "[ENSURE FAILED] " << #expr \
                      << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            return false; \
        } \
        return true; \
    }())
