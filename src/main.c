#include <stdio.h>

#include "keac.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Error: No input files\n");
		return 1;
	}

	for (int i = 1; i < argc; ++i)
		keac_run(argv[i]);

	return 0;
}
