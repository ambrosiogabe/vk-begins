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

// ------------ Internal Variables ------------
const std::array<const char*, 1> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef _DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

static int constexpr windowWidth = 1920;
static int constexpr windowHeight = 1080;
static const char* windowTitle = "Vulkan Begins";
static GLFWwindow* window;

static VkInstance vkInstance;
static VkDebugUtilsMessengerEXT debugMessenger;

// ------------ Internal Functions ------------
static void createInstance();
static bool checkForRequiredExts(const std::vector<const char*>& requiredExts);
static bool checkValidationLayerSupport();
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

	createInstance();

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
	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
	}

	vkDestroyInstance(vkInstance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

// ------------ Internal Functions ------------
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
	
	setupDebugMessenger();
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
	else
	{
		g_logger_log("Validation Layer: \n\t%s", pCallbackData->pMessage);
	}
	return VK_FALSE;
}