#ifndef FILE_H
#define FILE_H

#include <stdbool.h>

char *file_read(const char *file_name);
void file_write(const char *file_name, const char *src);
char *file_asm_name(const char *file_name); // Replaces the file extension to .asm and if it doesn't exist it adds it

#endif // FILE_H
