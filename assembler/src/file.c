#include "file.h"

#define _XOPEN_SOURCE	501
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct file read_file(const char *path)
{
	struct file f;

	struct stat stats;

	if (stat(path, &stats))
		return (struct file){ 0 };
	
	const int fd = open(path, O_RDONLY);
	if (fd == -1)
		return (struct file){ 0 };
	
	f.len = stats.st_size;
	f.dat = mmap(nullptr, f.len, PROT_READ, MAP_PRIVATE, fd, 0);

	close(fd);

	if (f.dat == MAP_FAILED)
		return (struct file){ 0 };
	
	return f;
}

int write_file(struct file *f, const char *path)
{
	const int fd = open(path, O_RDWR|O_TRUNC|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1)
		return 1;
	
	if (ftruncate(fd, f->len))
		return 1;
	
	uint8_t *dat = mmap(nullptr, f->len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	close(fd);

	if (dat == MAP_FAILED)
		return 1;
	
	memcpy(dat, f->dat, f->len);
	
	close_file(f);

	return 0;
}

void close_file(struct file *f)
{
	munmap(f->dat, f->len);
}
