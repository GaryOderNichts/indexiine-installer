#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "fs/sd_fat_devoptab.h"
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include "utils/fsutils.h"
#include <sys/stat.h>
#include "exploit.h"
#include "../payload/wupserver_bin.h"

#define MAX_CONSOLE_LINES_TV    27
#define MAX_CONSOLE_LINES_DRC   18

static char* consoleArrayTv[MAX_CONSOLE_LINES_TV];
static char* consoleArrayDrc[MAX_CONSOLE_LINES_DRC];

Region region = Undetected;

static void console_print_pos(int x, int y, const char *format, ...)
{
	char* tmp = NULL;

	va_list va;
	va_start(va, format);
	if((vasprintf(&tmp, format, va) >= 0) && tmp)
	{
		if(strlen(tmp) > 79)
			tmp[79] = 0;

		OSScreenPutFontEx(0, x, y, tmp);
		OSScreenPutFontEx(1, x, y, tmp);

	}
	va_end(va);

	if(tmp)
		free(tmp);
}

void console_printf(int newline, const char *format, ...)
{
	char* tmp = NULL;

	va_list va;
	va_start(va, format);
	if((vasprintf(&tmp, format, va) >= 0) && tmp)
	{
		if(newline)
		{
			if(consoleArrayTv[0])
				free(consoleArrayTv[0]);
			if(consoleArrayDrc[0])
				free(consoleArrayDrc[0]);

			for(int i = 1; i < MAX_CONSOLE_LINES_TV; i++)
				consoleArrayTv[i-1] = consoleArrayTv[i];

			for(int i = 1; i < MAX_CONSOLE_LINES_DRC; i++)
				consoleArrayDrc[i-1] = consoleArrayDrc[i];
		}
		else
		{
			if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
				free(consoleArrayTv[MAX_CONSOLE_LINES_TV-1]);
			if(consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1])
				free(consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1]);

			consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = NULL;
			consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = NULL;
		}

		if(strlen(tmp) > 79)
			tmp[79] = 0;

		consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = (char*)malloc(strlen(tmp) + 1);
		if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
			strcpy(consoleArrayTv[MAX_CONSOLE_LINES_TV-1], tmp);

		consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = (tmp);
		
	}
	va_end(va);

	// Clear screens
	OSScreenClearBufferEx(0, 0);
	OSScreenClearBufferEx(1, 0);


	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
	{
		if(consoleArrayTv[i])
			OSScreenPutFontEx(0, 0, i, consoleArrayTv[i]);
	}

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
	{
		if(consoleArrayDrc[i])
			OSScreenPutFontEx(1, 0, i, consoleArrayDrc[i]);
	}

	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);
}

//just to be able to call async
void someFunc(void *arg)
{
	(void)arg;
}

static int mcp_hook_fd = -1;
int MCPHookOpen()
{
	//take over mcp thread
	mcp_hook_fd = MCP_Open();
	if(mcp_hook_fd < 0)
		return -1;
	IOS_IoctlAsync(mcp_hook_fd, 0x62, (void*)0, 0, (void*)0, 0, someFunc, (void*)0);
	//let wupserver start up
	sleep(1);
	if(IOSUHAX_Open("/dev/mcp") < 0)
		return -1;
	return 0;
}

void MCPHookClose()
{
	if(mcp_hook_fd < 0)
		return;
	//close down wupserver, return control to mcp
	IOSUHAX_Close();
	//wait for mcp to return
	sleep(1);
	MCP_Close(mcp_hook_fd);
	mcp_hook_fd = -1;
}

int Menu_Main(void)
{
	InitOSFunctionPointers();
	InitSocketFunctionPointers();
	InitFSFunctionPointers();
	InitVPadFunctionPointers();

	VPADInit();

	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
		consoleArrayTv[i] = NULL;

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
		consoleArrayDrc[i] = NULL;

	// Prepare screen
	int screen_buf0_size = 0;

	// Init screen and screen buffers
	OSScreenInit();
	screen_buf0_size = OSScreenGetBufferSizeEx(0);
	OSScreenSetBufferEx(0, (void *)0xF4000000);
	OSScreenSetBufferEx(1, (void *)(0xF4000000 + screen_buf0_size));

	OSScreenEnableEx(0, 1);
	OSScreenEnableEx(1, 1);

	// Clear screens
	OSScreenClearBufferEx(0, 0);
	OSScreenClearBufferEx(1, 0);

	// Flip buffers
	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);

	int mountsdres = mount_sd_fat("sd");
	if (mountsdres < 0)
	{
		console_printf(1, "Error mounting sd: %i\n", mountsdres);
		sleep(2);
		return 0;
	}

	int res = IOSUHAX_Open(NULL);
	if (res < 0)
		res = MCPHookOpen();
	if (res < 0)
	{
		console_printf(1, "Doing IOSU Exploit...");
		*(volatile unsigned int*)0xF5E70000 = wupserver_bin_len;
		memcpy((void*)0xF5E70020, &wupserver_bin, wupserver_bin_len);
		DCStoreRange((void*)0xF5E70000, wupserver_bin_len + 0x40);
		IOSUExploit();
		//done with iosu exploit, take over mcp
		if(MCPHookOpen() < 0)
		{
			console_printf(1, "MCP hook could not be opened!");
			goto exit;
		}
		console_printf(1, "Done doing IOSU Exploit!");
	}

	int fsaFd = IOSUHAX_FSA_Open();
	if(fsaFd < 0)
	{
		console_printf(1, "IOSUHAX_FSA_Open failed: %i\n", fsaFd);
		sleep(2);
		return 0;
	}

	int mountres = mount_fs("dev", fsaFd, NULL, MLC_MOUNT_PATH);
	if(mountres < 0)
	{
		console_printf(1, "%i Mount of %s failed", mountres, MLC_MOUNT_PATH);
		sleep(5);
		return 0;
	}

	// detect region
	if (dirExists(BROWSER_PATH_EUR))
		region = EUR;
	else if (dirExists(BROWSER_PATH_USA))
		region = USA;
	else if (dirExists(BROWSER_PATH_JPN))
		region = JPN;

	if (region == Undetected)
	{
		console_printf(1, "Error detecting Region!");
		sleep(2);
		return 0;
	}

	const char* regionStrings[] = {"Undetected", "EUR", "USA", "JPN"};

	// Clear screens
	OSScreenClearBufferEx(0, 0);
	OSScreenClearBufferEx(1, 0);

	console_print_pos(0, 0, "-----------------------------------------");
	console_print_pos(0, 1, "Indexiine Installer v1 by GaryOderNichts");
	console_print_pos(0, 2, "Indexiine by Jonhyjp");
	console_print_pos(0, 3, "Detected Region: %s", regionStrings[region]);
	console_print_pos(0, 4, "-----------------------------------------");

	console_print_pos(0, 6, "Press A to backup and replace index.html");
	console_print_pos(0, 7, "Press B to restore index.html");

	console_print_pos(0, 9, "Press HOME to exit");

	// Flip buffers
	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);

	int vpadError = -1;
	VPADData vpad;

	char* indexpath;
	switch (region)
	{
	case EUR:
		indexpath = BROWSER_PATH_EUR INDEX_PATH;
		break;
	case USA:
		indexpath = BROWSER_PATH_USA INDEX_PATH;
		break;
	case JPN:
		indexpath = BROWSER_PATH_JPN INDEX_PATH;
		break;
	default:
		return 0;
		break;
	}

	while (1)
	{
		VPADRead(0, &vpad, 1, &vpadError);

		// Install
		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_A))
		{
			// Backup the file if it exists and there is no backup
			if (fileExists(indexpath) && !fileExists(INDEX_BACKUP_PATH))
			{
				copyFile(indexpath, INDEX_BACKUP_PATH);
			}
			
			if (copyFile(INDEXIINE_INDEX_PATH, indexpath))
			{
				// chmod the file
				chmodSingle(fsaFd, indexpath, INDEX_MODE);	
			} 
			else
			{
				console_printf(1, "Error copying index.html to %s", indexpath);
				sleep(4);
				break;
			}
			
			console_printf(1, "Successfully installed Indexiine!");
			sleep(4);
			break;
		}

		// Restore
		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B))
		{
			if (copyFile(INDEX_BACKUP_PATH, indexpath))
			{
				// chmod the file
				chmodSingle(fsaFd, indexpath, INDEX_MODE);

				console_printf(1, "Successfully restored index.html");
			}

			sleep(4);
			break;
		}

		// Exit
		if(vpadError == 0 && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_HOME))
		{
			break;
		}
	}
	exit: 
	
	console_printf(1, "Exiting...");

	for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
	{
		if(consoleArrayTv[i])
			free(consoleArrayTv[i]);
	}

	for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
	{
		if(consoleArrayDrc[i])
			free(consoleArrayDrc[i]);
	}

	unmount_fs("dev");

	unmount_sd_fat("sd");

	IOSUHAX_FSA_Close(fsaFd);

	if(mcp_hook_fd >= 0)
		MCPHookClose();
	else
		IOSUHAX_Close();

	sleep(1);

	return 0;
}

