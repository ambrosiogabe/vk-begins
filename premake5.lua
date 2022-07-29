workspace "VulkanBegins"
    architecture "x64"

    configurations {
        "Debug",
        "Release",
        "Dist"
    }

    startproject "VulkanBegins"

-- This is a helper variable, to concatenate the sys-arch
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "VulkanBegins"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "VulkanBegins/src/**.cpp",
        "VulkanBegins/include/**.h",
        "VulkanBegins/vendor/glm/glm/**.hpp",
		"VulkanBegins/vendor/glm/glm/**.inl"
    }

    includedirs {
        "VulkanBegins/include",
        "VulkanBegins/vendor/GLFW/include",
        "VulkanBegins/vendor/cppUtils/single_include/",
        "VulkanBegins/vendor/glm/",
        "VulkanBegins/vendor/stb/",
        -- SUPER ICKY: Anybody know a way around this?
        "C:/VulkanSDK/1.3.216.0/Include"
    }

    systemversion "latest"
    defines  { "_CRT_SECURE_NO_WARNINGS" }

    links {
        "GLFW",
        "vulkan-1.lib"
    }

    libdirs {
        -- ALSO SUPER ICKY: See note above
        "C:/VulkanSDK/1.3.216.0/Lib"
    }

    filter { "configurations:Debug" }
        buildoptions "/MTd"
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        buildoptions "/MT"
        runtime "Release"
        optimize "on"

        defines {
            "_RELEASE"
        }

project "GLFW"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    includedirs { 
        "VulkanBegins/vendor/GLFW/include"
    }

    files { 
        "VulkanBegins/vendor/GLFW/include/GLFW/glfw3.h",
        "VulkanBegins/vendor/GLFW/include/GLFW/glfw3native.h",
        "VulkanBegins/vendor/GLFW/src/glfw_config.h",
        "VulkanBegins/vendor/GLFW/src/context.c",
        "VulkanBegins/vendor/GLFW/src/init.c",
        "VulkanBegins/vendor/GLFW/src/input.c",
        "VulkanBegins/vendor/GLFW/src/monitor.c",
        "VulkanBegins/vendor/GLFW/src/vulkan.c",
        "VulkanBegins/vendor/GLFW/src/window.c",
        "VulkanBegins/vendor/GLFW/src/platform.c",
        "VulkanBegins/vendor/GLFW/src/null_init.c",
        "VulkanBegins/vendor/GLFW/src/null_joystick.c",
        "VulkanBegins/vendor/GLFW/src/null_monitor.c",
        "VulkanBegins/vendor/GLFW/src/null_window.c"
    }

    systemversion "latest"
    defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "system:windows"
        files {
            "VulkanBegins/vendor/GLFW/src/wgl_context.c",
            "VulkanBegins/vendor/GLFW/src/win32_init.c",
            "VulkanBegins/vendor/GLFW/src/win32_joystick.c",
            "VulkanBegins/vendor/GLFW/src/win32_module.c",
            "VulkanBegins/vendor/GLFW/src/win32_monitor.c",
            "VulkanBegins/vendor/GLFW/src/win32_thread.c",
            "VulkanBegins/vendor/GLFW/src/win32_time.c",
            "VulkanBegins/vendor/GLFW/src/win32_window.c",

            "VulkanBegins/vendor/GLFW/src/osmesa_context.c",
            "VulkanBegins/vendor/GLFW/src/egl_context.c"
        }

        defines  {
            "_GLFW_WIN32",
            "_GLFW_VULKAN_STATIC"
        }

    filter "configurations:Debug"
        buildoptions "/MTd"
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        buildoptions "/MT"
        symbols "Off"
        warnings "Extra"    
