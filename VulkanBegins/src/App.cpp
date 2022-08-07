#include "VulkanBegins/App.h"

#include <cppUtils/cppUtils.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <array>
#include <vector>
#include <set>

// NOTE: Look into dynamic rendering
// NOTE: Consider getting rid of renderpasses and framebuffers and focusing on 
// pipeline barriers in the beginning

// ------------ Internal structures ------------
constexpr uint32 NullQueueFamily = UINT32_MAX;

struct QueueFamilyIndices
{
	uint32 graphicsFamily;
	// TODO: The presentFamily typically ends up being equal to the graphicsFamily
	// Instead of adding synchronization for these, don't support separate queue families
	// or add extra logic to handle when they are the same
	uint32 presentFamily;

	inline bool isComplete()
	{
		return graphicsFamily != NullQueueFamily && presentFamily != NullQueueFamily;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// ------------ Internal Variables ------------
const std::array<const char*, 1> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::array<const char*, 1> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

// Window stuff
static int constexpr windowWidth = 1920;
static int constexpr windowHeight = 1080;
static const char* windowTitle = "Vulkan Begins";
static GLFWwindow* window;

// General vulkan stuff
static VkInstance vkInstance;
static VkDebugUtilsMessengerEXT debugMessenger;

// Device stuff
static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
static VkDevice logicalDevice;

// Queue stuff
static VkQueue graphicsQueue;
static VkQueue presentQueue;

// Swap chain stuff
static VkSurfaceKHR surface;
static VkSwapchainKHR swapChain;
static std::vector<VkImage> swapChainImages;
static VkFormat swapChainImageFormat;
static VkExtent2D swapChainExtent;
static std::vector<VkImageView> swapChainImageViews;

// ------------ Internal Functions ------------
static void initVulkan();
static void createInstance();
static void pickPhysicalDevice();
static void createLogicalDevice();
static void createSwapChain();
static void createSurface();
static void createImageViews();
static bool isDeviceSuitable(VkPhysicalDevice device);
static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
static bool checkForRequiredExts(const std::vector<const char*>& requiredExts);
static bool checkValidationLayerSupport();
static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
static std::vector<const char*> getRequiredExtensions();
static void setupDebugMessenger();
static void initDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

void vkb_app_init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
	if (!window)
	{
		g_logger_error("Failed to create window.");
		return;
	}

	initVulkan();

	g_logger_info("Successfully initialized Vulkan.");
}

void vkb_app_run()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void vkb_app_free()
{
	for (auto& swapChainImageView : swapChainImageViews)
	{
		vkDestroyImageView(logicalDevice, swapChainImageView, nullptr);
	}
	vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	vkDestroyDevice(logicalDevice, nullptr);

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(vkInstance, surface, nullptr);
	vkDestroyInstance(vkInstance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

// ------------ Internal Functions ------------
static void initVulkan()
{
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
}

static void createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Begins";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	g_logger_assert(checkForRequiredExts(extensions), "Missing required extensions.");
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers)
	{
		g_logger_assert(checkValidationLayerSupport(), "Missing validation layers.");
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();

		initDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (void*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
	g_logger_assert(result == VK_SUCCESS, "Failed to create vulkan instance.");
}

static void pickPhysicalDevice()
{
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
	g_logger_assert(deviceCount != 0, "No Graphics Cards found.");

	VkPhysicalDevice* devices = (VkPhysicalDevice*)g_memory_allocate(sizeof(VkPhysicalDevice) * deviceCount);
	vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);

	for (uint32 devicei = 0; devicei < deviceCount; devicei++)
	{
		if (isDeviceSuitable(devices[devicei]))
		{
			physicalDevice = devices[devicei];
			break;
		}
	}

	g_memory_free(devices);

	g_logger_assert(physicalDevice != VK_NULL_HANDLE, "Failed to find suitable graphics card for Vulkan.");

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	g_logger_info("Found suitable device: '%s'", deviceProperties.deviceName);
}

static void createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	std::set<uint32> uniqueIndices = { indices.graphicsFamily, indices.presentFamily };


	VkDeviceQueueCreateInfo* queueCreateInfos = (VkDeviceQueueCreateInfo*)g_memory_allocate(sizeof(VkDeviceQueueCreateInfo) * uniqueIndices.size());

	float queuePriority = 1.0f;
	{
		int i = 0;
		for (uint32 queueFamilyIndex : uniqueIndices)
		{
			queueCreateInfos[i] = {};
			queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfos[i].queueFamilyIndex = queueFamilyIndex;
			queueCreateInfos[i].queueCount = 1;
			queueCreateInfos[i].pQueuePriorities = &queuePriority;
			i++;
		}
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos;
	createInfo.queueCreateInfoCount = uniqueIndices.size();

	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = requiredDeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
	g_logger_assert(result == VK_SUCCESS, "failed to create logical device!");

	// List of Queues
	// Each queue family has a list of queues
	// GraphicsFamilyQueue 
	// {
	//   Queue[n]
	// }
	// PresentFamilyQueue 
	// {
	//   Queue[n]
	// }
	vkGetDeviceQueue(logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, indices.presentFamily, 0, &graphicsQueue);

	g_memory_free(queueCreateInfos);
}

static void createSwapChain() 
{
	SwapChainSupportDetails details = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
	swapChainExtent = chooseSwapExtent(details.capabilities);
	swapChainImageFormat = surfaceFormat.format;

	uint32 imageCount = details.capabilities.minImageCount + 1;
	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
	{
		imageCount = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapChainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32 queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	uint32 result = vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain);
	g_logger_assert(result == VK_SUCCESS, "Failed to create swap chain.");

	uint32 numImages;
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &numImages, nullptr);

	swapChainImages.resize(numImages);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &numImages, swapChainImages.data());
}

static void createSurface()
{
	uint32 result = glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);
	g_logger_assert(result == VK_SUCCESS, "Failed to create window surface.");
}

static void createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		uint32 result = vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]);
		g_logger_assert(result == VK_SUCCESS, "Failed to create swap chain image views.");
	}
}

static bool isDeviceSuitable(VkPhysicalDevice device)
{
	// TODO: Can use these and check for certain properties
	// VkPhysicalDeviceProperties deviceProperties;
	// vkGetPhysicalDeviceProperties(device, &deviceProperties);
	// 
	// VkPhysicalDeviceFeatures deviceFeatures;
	// vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails details = querySwapChainSupport(device);
		swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	indices.graphicsFamily = NullQueueFamily;

	// NOTE: We look for a queue family suitable to store graphics commands
	// which is our requirements for a suitable device
	// Logic to find graphics queue family
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)g_memory_allocate(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

	for (uint32 familyi = 0; familyi < queueFamilyCount; familyi++)
	{
		const VkQueueFamilyProperties& queueFamily = queueFamilies[familyi];
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = familyi;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, familyi, surface, &presentSupport);
		if (presentSupport)
		{
			indices.presentFamily = familyi;
		}

		if (indices.isComplete())
		{
			break;
		}
	}

	g_memory_free(queueFamilies);

	return indices;
}

static bool checkForRequiredExts(const std::vector<const char*>& requiredExts)
{
	bool res = true;

	// How to enumerate extensions
	uint32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	VkExtensionProperties* extensions = (VkExtensionProperties*)g_memory_allocate(sizeof(VkExtensionProperties) * extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
	for (uint32 requiredExti = 0; requiredExti < requiredExts.size(); requiredExti++)
	{
		bool foundExt = false;
		for (uint32 exti = 0; exti < extensionCount; exti++)
		{
			if (strcmp(extensions[exti].extensionName, requiredExts[requiredExti]) == 0)
			{
				foundExt = true;
				break;
			}
		}

		if (!foundExt)
		{
			g_logger_error("Missing required extension: '%s'", requiredExts[requiredExti]);
			res = false;
		}
	}

	g_memory_free(extensions);
	return res;
}

static bool checkValidationLayerSupport()
{
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	VkLayerProperties* availableLayers = (VkLayerProperties*)g_memory_allocate(sizeof(VkLayerProperties) * layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	bool res = true;
	for (uint32 requiredLayeri = 0; requiredLayeri < validationLayers.size(); requiredLayeri++)
	{
		bool foundLayer = false;
		for (uint32 layeri = 0; layeri < layerCount; layeri++)
		{
			if (strcmp(availableLayers[layeri].layerName, validationLayers[requiredLayeri]) == 0)
			{
				foundLayer = true;
				break;
			}
		}

		if (!foundLayer)
		{
			g_logger_error("Missing validation layer: '%s'", validationLayers[requiredLayeri]);
			res = false;
		}
	}

	g_memory_free(availableLayers);

	return res;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32 deviceExtensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);

	VkExtensionProperties* extensions = (VkExtensionProperties*)g_memory_allocate(sizeof(VkExtensionProperties) * deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, extensions);

	bool res = true;
	for (const char* requiredExt : requiredDeviceExtensions)
	{
		bool foundExt = false;
		for (int i = 0; i < deviceExtensionCount; i++)
		{
			if (strcmp(extensions[i].extensionName, requiredExt) == 0)
			{
				foundExt = true;
				break;
			}
		}

		if (!foundExt)
		{
			res = false;
			break;
		}
	}

	g_memory_free(extensions);

	return res;
}

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32 presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}
	
	return details;
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const VkSurfaceFormatKHR& format : availableFormats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	// TODO: Have some sort of selection heirarchy to choose the next best format
	return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& presentMode : availablePresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	VkExtent2D actualSize = {
		(uint32)width,
		(uint32)height
	};

	actualSize.width = glm::clamp(actualSize.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualSize.height = glm::clamp(actualSize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualSize;
}

static std::vector<const char*> getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

static void setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	initDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		g_logger_assert(false, "failed to set up debug messenger!");
	}
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

static void initDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		g_logger_error("Validation Layer: \n\t%s", pCallbackData->pMessage);
	}
	else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		g_logger_warning("Validation Layer: \n\t%s", pCallbackData->pMessage);
	}
	else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		g_logger_log("Validation Layer: \n\t%s", pCallbackData->pMessage);
	}
	return VK_FALSE;
}