#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STREQ(s, t)	(!strcmp((s), (t)))

#define	unused(x)	(void)(x)
#define	todo(x)		do { fprintf(stderr, "todo: \'%s\'\n", (x)); abort(); } while (0)

#define assert_mem(p)	\
({ \
	void *tmp = (p); \
	if (!tmp) abort(); \
	(p) \
})

#define xcalloc(n, size)	assert_mem(calloc((n), (size)))
