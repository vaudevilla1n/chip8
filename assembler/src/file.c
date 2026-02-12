#include "file.h"

#include <fcntl.h>
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

void close_file(struct file *f)
{
	munmap(f->dat, f->len);
}
