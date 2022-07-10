#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stddef.h>
#include <stdint.h>

typedef struct symbol {
	char *id;
	size_t id_length;

	struct symbol *next;
} Symbol;

typedef struct symbol_table {
	size_t table_size;
	Symbol *table;
	uint32_t entry_count;
} SymbolTable;

SymbolTable *st_create(size_t initial_size);
void st_free(SymbolTable *table);

Symbol *st_get_symbol(SymbolTable *table, const char *value);
Symbol *st_add_symbol(SymbolTable *table, const char *value);
void st_remove_symbol(SymbolTable *table, const char *value);
void sb_print(SymbolTable *table);

#endif // SYMBOL_TABLE_H
