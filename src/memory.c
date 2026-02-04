#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "logger.h"


Memory* memory_create() {
	Memory *memory = malloc(sizeof(Memory));
	assert(memory);
	memset(memory, 0, sizeof(Memory));
	return memory;
}

void memory_destroy(Memory *memory) { free(memory); }

