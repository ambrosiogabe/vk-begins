#ifndef VK_BEGINS_FILE_H
#define VK_BEGINS_FILE_H

#include <cppUtils/cppUtils.hpp>

struct vkb_FileContents
{
	uint8* data;
	uint32 size;
};

vkb_FileContents vkb_file_read(const char* filename);

void vkb_file_free(vkb_FileContents& fileContents);

#endif 