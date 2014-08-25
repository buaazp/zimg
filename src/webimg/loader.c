#include <stdlib.h>
#include <string.h>

#include "internal.h"

extern struct loader *loaders[];

struct loader * choose_loader(void *blob, size_t len)
{
	struct loader **ld = loaders;

	while (*ld != NULL) {
		if ((*ld)->suitable != NULL && (*ld)->suitable(blob, len)) {
			break;
		}
		ld++;
	}

	return *ld;
}

struct loader * choose_saver(const char *fmt)
{
	struct loader **ld = loaders;

	while (*ld != NULL) {
		if ((*ld)->save != NULL && strcmp(fmt, (*ld)->name) == 0) {
			break;
		}
		ld++;
	}

	return *ld;
}
