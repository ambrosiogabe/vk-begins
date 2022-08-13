#include "VulkanBegins/File.h"

#include <stdio.h>

vkb_FileContents vkb_file_read(const char* filename)
{
	FILE* fp = fopen(filename, "rb");

	if (fp == nullptr)
	{
		g_logger_error("Could not open file '%s'", filename);
		return vkb_FileContents{nullptr, 0};
	}

	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8* result = (uint8*)g_memory_allocate(sizeof(uint8) * fileSize);
	if (result == nullptr)
	{
		g_logger_error("Out of RAM.");
		fileSize = 0;
	}
	else
	{
		fread(result, fileSize, 1, fp);
	}

	fclose(fp);

	return vkb_FileContents{result, (uint32)fileSize};
}

void vkb_file_free(vkb_FileContents& fileContents)
{
	if (fileContents.data != nullptr)
	{
		g_memory_free(fileContents.data);
		fileContents.data = nullptr;
	}

	fileContents.size = 0;
}