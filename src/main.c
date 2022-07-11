#include <stdio.h>
// #include <stdlib.h>

#include "keac.h"
// #include "file.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "error: no input files\n");
		return 1;
	}

	for (int i = 1; i < argc; ++i) {
		keac_compile(argv[i]);
		// char *assembly = keac_compile(argv[i]);
		// char *asm_file_name = file_asm_name(argv[i]);

		// file_write(asm_file_name, assembly);
		// free(assembly);
		// free(asm_file_name);
	}

	return 0;
}
