#pragma once

#include <stdio.h>
#include <stdlib.h>

#define	unused(x)	(void)(x)
#define	todo(x)		do { fprintf(stderr, "todo: \'%s\'\n", (x)); abort(); } while (0)
