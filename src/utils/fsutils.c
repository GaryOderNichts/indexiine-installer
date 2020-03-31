#include "fsutils.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include "dynamic_libs/fs_functions.h"
#include "stringutils.h"
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <malloc.h>
#include <errno.h>
#include "../main.h"

#define BUFFER_SIZE 0x80000

uint8_t removeRecursive(const char* path)
{
	DIR* dir = opendir(path);
	if (dir == NULL)
	{
		console_printf(1, "Error opening %s for removal!", path);
		return 0;
	}
		
	size_t path_len = strlen(path);

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) 
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char* full_path = calloc(path_len + strlen(entry->d_name) + 1, sizeof(char));
		strcpy(full_path, path);
		strcat(full_path, "/");
		strcat(full_path, entry->d_name);

		if (entry->d_type & DT_DIR)
		{
			removeRecursive(full_path);
		}
		else
		{		
			if (remove(full_path) != 0)
			{
				console_printf(1, "Error removing %s!", full_path);
			}
			else
			{
				console_printf(1, "Removed %s", full_path);
			}
			
		}

		free(full_path);
	}

	closedir(dir);

	if (strcmp(path, "dev:") != 0)
	{
		if (remove(path) != 0)
		{
			console_printf(1, "Error removing %s %i!", path, errno);
		}
		else
		{
			console_printf(1, "Removed %s", path);
		}
	}
		

	return 1;
}

uint8_t dirExists(const char* path)
{
	DIR* dir = opendir(path);
	if (dir != NULL)
	{
		closedir(dir);
		return 1;
	}
	
	return 0;
}

uint8_t fileExists(const char* path)
{
	FILE* file = fopen(path, "r");
	if (file != NULL)
	{
		fclose(file);
		return 1;
	}
	
	return 0;
}

uint8_t chmodSingle(int fsaFd, char *pPath, int mode)
{
	char* rootpath = calloc(FS_MAX_FULLPATH_SIZE, sizeof(char));
	strcpy(rootpath, pPath);
	stringReplace("dev:", MLC_MOUNT_PATH, rootpath);

	int ret = IOSUHAX_FSA_ChangeMode(fsaFd, rootpath, mode);
	if (ret < 0)
	{
		console_printf(1, "chmod 0x%X error %i for %s", mode, ret, pPath);
	} 
	else
	{
		console_printf(1, "chmod 0x%X successful for %s", mode, pPath);
		return 1;
	}

	return 0;
}

uint8_t copyDir(const char* src, const char* dst)
{
	DIR* dir = opendir(src);
	if (dir == NULL)
	{
		console_printf(1, "Cannot open %s\n", src);
		return 0;
	}

	if (!dirExists(dst))
		if(mkdir(dst, 0777) != 0)
		{
			console_printf(1, "Error creating %s", dst);
			return 0;
		}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if(strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0)
			continue;

		char* srcPath = calloc(FS_MAX_FULLPATH_SIZE, sizeof(char));
		strcpy(srcPath, src);
		strcat(srcPath, "/");
		strcat(srcPath, entry->d_name);

		char* dstPath = calloc(FS_MAX_FULLPATH_SIZE, sizeof(char));
		strcpy(dstPath, dst);
		strcat(dstPath, "/");
		strcat(dstPath, entry->d_name);

		if (entry->d_type & DT_DIR)
		{
			if (!dirExists(dstPath))
				if (mkdir(dstPath, 0777) != 0)
				{
					continue;
				}
			console_printf(1, "Created %s\n", entry->d_name);
			copyDir(srcPath, dstPath);
		}
		else
		{
			unsigned char* dataBuffer = memalign(0x40, BUFFER_SIZE);
			if (!dataBuffer)
			{
				console_printf(1, "Out of memory!\n");
				return 0;
			}

			FILE* readFile = fopen(srcPath, "rb");
			if(readFile == NULL)
			{
				console_printf(1, "Cannot open file %s\n", srcPath);
				continue;
			}

			FILE* writeFile = fopen(dstPath, "wb");
			if(writeFile == NULL)
			{
				console_printf(1, "Cannot create file %s\n", srcPath);
				continue;
			}

			unsigned int size = 0;
			unsigned int ret;
			u32 passedMs = 1;
			u64 startTime = OSGetTime();

			console_printf(1, "\n");
			while ((ret = fread(dataBuffer, 0x1, BUFFER_SIZE, readFile)) > 0)
			{
				passedMs = (OSGetTime() - startTime) * 4000ULL / BUS_SPEED;
				if(passedMs == 0)
					passedMs = 1; // avoid 0 div

				// write buffer to file
				fwrite(dataBuffer, 0x01, ret, writeFile);
				size += ret;
				console_printf(0, " %s - 0x%X (%i kB/s)\r", entry->d_name, size, (u32)(((u64)size * 1000) / ((u64)1024 * passedMs)));
			}
			console_printf(1, "Copied %s\n", entry->d_name);
			free(dataBuffer);
			fclose(writeFile);
			fclose(readFile);
		}

		free(srcPath);
		free(dstPath);
	}
	closedir(dir);

	return 1;
}

uint8_t copyFile(const char* src, const char* dst)
{
	FILE* sourceFile = fopen(src, "rb");
	if (sourceFile == NULL)
	{
		console_printf(1, "Error opening %s, src");
		return 0;
	}

	unsigned char* dataBuffer = memalign(0x40, BUFFER_SIZE);
	if (!dataBuffer)
	{
		console_printf(1, "Out of memory!");
		fclose(sourceFile);
		return 0;
	}
	
	FILE* destFile = fopen(dst, "wb");
	if (destFile == NULL)
	{
		console_printf(1, "Error creating %s, src");
		fclose(sourceFile);
		free(dataBuffer);
		return 0;
	}
	
	unsigned int size = 0;
	unsigned int ret;
	u32 passedMs = 1;
	u64 startTime = OSGetTime();

	console_printf(1, "\n");
	while ((ret = fread(dataBuffer, 0x1, BUFFER_SIZE, sourceFile)) > 0)
	{
		passedMs = (OSGetTime() - startTime) * 4000ULL / BUS_SPEED;
		if(passedMs == 0)
			passedMs = 1; // avoid 0 div

		// write buffer to file
		fwrite(dataBuffer, 0x01, ret, destFile);
		size += ret;
		console_printf(0, "0x%X (%i kB/s)\r", size, (u32)(((u64)size * 1000) / ((u64)1024 * passedMs)));
	}

	console_printf(1, "Copied %s\n", src);
	free(dataBuffer);
	fclose(sourceFile);
	fclose(destFile);

	return 1;
}