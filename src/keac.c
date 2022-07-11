#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "keac.h"
#include "file.h"
#include "lexer.h"
#include "symbol_table.h"

char *keac_compile(const char *file_name)
{
	SymbolTable *table = st_create(8);

	char *src = file_read(file_name);

	Lexer *lexer = lexer_create(src, file_name, table);

	while (lexer_next_token(lexer)->type != TOKEN_EOF);
	putchar('\n');
	free(src);
	lexer_free(lexer);

	st_free(table);

	// After code generation
	// char *assembly;
	// return assembly;
	return NULL;
}

void keac_error(const char *file, uint64_t line, uint64_t column, const char *message, ...)
{
	fprintf(stderr, "%s:%lu:%lu: error: ", file, line, column);

	va_list args;
	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);
}