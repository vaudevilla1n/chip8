#pragma once

#include <stddef.h>

struct file {
	char *dat;
	size_t len;
};

struct file read_file(const char *path);
void close_file(struct file *f);
