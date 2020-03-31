#ifndef _FSUTILS_H_
#define _FSUTILS_H_

#include <stdint.h>

uint8_t removeRecursive(const char* path);
uint8_t dirExists(const char* path);
uint8_t fileExists(const char* path);
uint8_t chmodSingle(int fsaFd, char *pPath, int mode);
uint8_t copyDir(const char* src, const char* dst);
uint8_t copyFile(const char* src, const char* dst);

#endif