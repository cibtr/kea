#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

char *file_read(const char *file_name)
{
	FILE *file = fopen(file_name, "r");
	if (file == NULL) {
		fprintf(stderr, "error: %s: file not found\n", file_name);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = malloc(file_size + 1);
	fread(buffer, sizeof(char), file_size, file);

	fclose(file);

	return buffer;
}

void file_write(const char *file_name, const char *source)
{
	FILE *file = fopen(file_name, "w+");
	fwrite(source, sizeof(char), strlen(source), file);

	fclose(file);
}
