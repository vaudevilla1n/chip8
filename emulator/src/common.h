#pragma once

#include <stdio.h>
#include <stdlib.h>

#define unused(x)	(void)(x)
#define todo(x)		do { fprintf(stderr, "todo: \'%s\'\n", (x)); abort(); } while (0)

#define warn(fmt, ...)	do { fprintf(stderr, fmt __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(fmt, ...)	do { warn(fmt __VA_OPT__(,)__VA_ARGS__); exit(1); } while (0)

#define countof(arr)	(sizeof(arr) / sizeof((arr)[0]))

