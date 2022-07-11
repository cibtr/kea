#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "file.h"

static uint32_t get_extension_index(const char *file_name);

char *file_read(const char *file_name)
{
	FILE *file = fopen(file_name, "r");
	if (file == NULL) {
		fprintf(stderr, "error: %s: cannot open file\n", file_name);
		exit(EXIT_FAILURE);
	}

	uint32_t extension_index = get_extension_index(file_name);
	if (extension_index != 0 && strcmp(file_name + extension_index, ".ke") != 0) {
		fprintf(stderr, "error: %s: unrecognised file format\n", file_name);
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

void file_write(const char *file_name, const char *contents)
{
	FILE *file = fopen(file_name, "w+");
	fwrite(contents, sizeof(char), strlen(contents), file);

	fclose(file);
}

char *file_asm_name(const char *file_name)
{
	char *asm_file_name;

	uint32_t extension_index = get_extension_index(file_name);
	if (extension_index == 0) { // No file extension
		size_t asm_file_name_len = strlen(file_name) + strlen(".asm");

		asm_file_name = strcpy(malloc(asm_file_name_len + 1), file_name);
		sprintf(asm_file_name + strlen(file_name), ".asm");
	}
	else {
		// .ke has 2 characters and .asm has 3 so .asm = .ke + 1
		size_t asm_file_name_len = strlen(file_name) + 1; 

		asm_file_name = strcpy(malloc(asm_file_name_len + 1), file_name);
		sprintf(asm_file_name + extension_index, ".asm");
	}

	return asm_file_name;
}

uint32_t get_extension_index(const char *file_name)
{
	uint32_t i;
	for (i = strlen(file_name) - 1; i != 0 && file_name[i] != '.'; --i);
	return i;
}
