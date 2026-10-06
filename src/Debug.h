#pragma once

#include <cstdarg>
#include <cstdio>

inline void
FatCatDebug(const char* format, ...)
{
	FILE* file = fopen("/tmp/fatcat-debug.log", "a");
	if (file == nullptr)
		return;
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fputc('\n', file);
	fclose(file);
}
