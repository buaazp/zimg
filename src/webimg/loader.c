#include <stdlib.h>
#include <string.h>

#include "internal.h"

extern struct loader *loaders[];

struct loader * choose_loader(void *blob, size_t len)
{
	struct loader **ld = loaders;

	while (*ld != NULL && (*ld)->suitable != NULL) {
		if ((*ld)->suitable(blob, len)) {
			break;
		}
		ld++;
	}

	return *ld;
}

struct loader * choose_saver(const char *fmt)
{
	struct loader **ld = loaders;

	while (*ld != NULL && (*ld)->save != NULL) {
		if (strcmp(fmt, (*ld)->name) == 0) {
			break;
		}
		ld++;
	}

	return *ld;
}
