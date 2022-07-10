#ifndef KEAC_H
#define KEAC_H

#include <stdint.h>

void keac_run(const char *file_name);
void keac_error(const char *file, uint64_t line, uint64_t column, const char *message, ...);

#endif // KEAC_H
