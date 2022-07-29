#include "VulkanBegins/App.h"

#include <cppUtils/cppUtils.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

static int constexpr windowWidth = 1920;
static int constexpr windowHeight = 1080;
static const char* windowTitle = "Vulkan Begins";
static GLFWwindow* window;

void vkb_app_init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (!window)
    {
        g_logger_error("Failed to create window.");
        return;
    }

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    g_logger_info("%d number of Vulkan extensions supported.", extensionCount);
}

void vkb_app_run()
{
    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void vkb_app_free()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}