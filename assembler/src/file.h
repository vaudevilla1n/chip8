#pragma once

#include <stddef.h>

struct file {
	char *dat;
	size_t len;
};

struct file read_file(const char *path);
int write_file(struct file *f, const char *path);
void close_file(struct file *f);
