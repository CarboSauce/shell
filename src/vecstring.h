#ifndef VECSTRING_H
#define VECSTRING_H
#include <stddef.h>

typedef struct vec {
	char* buf;
	size_t size;
	size_t cap;
} string;

string string_init();

void string_deinit(string* str);

void string_push(string* str, char c);

#endif
