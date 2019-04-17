#pragma once

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define FORMAT_NORMAL		1

#define LOG_FORCE			0
#define LOG_ERROR			1
#define LOG_WARNING			2
#define LOG_NOTICE			3
#define LOG_INFO			4
#define LOG_DEBUG			5

#define LOG_LEVEL			2

typedef struct _ERROR_TYPE_DESC
{
	unsigned int	type;
	const char		*desc;
}ErrorTypeDesc, *PErrorTypeDesc;

extern ErrorTypeDesc ErrorTypeTable[];

void debugLog(const char *mess,...);

#if !FORMAT_NORMAL
#define LogEx(f, l, e, ...) \
{ \
	fprintf(stdout, "[%s] %s:%d: ", ErrorTypeTable[e].desc, strrchr(f, '\\') + 1, l); \
	fprintf(stdout, __VA_ARGS__); \
}
#else
#define LogEx(f, l, e, ...) \
	debugLog(__VA_ARGS__)
#endif

#define Log(dwErrType, ...) \
	if(dwErrType <= LOG_LEVEL) \
		LogEx(__FILE__, __LINE__, dwErrType, __VA_ARGS__);
