#include <cppUtils/cppUtils.hpp>
#include "VulkanBegins/App.h"

int main()
{
    g_memory_init(true, 1024);

    vkb_app_init();
    vkb_app_run();
    vkb_app_free();

    g_memory_dumpMemoryLeaks();

    return 0;
}