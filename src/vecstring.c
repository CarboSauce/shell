#include "vecstring.h"
#include "stdio.h"
#include "stdlib.h"

string string_init()
{
	string out;
	out.buf = malloc(16);
	if (out.buf == NULL) {
		fprintf(stderr, "Critical error: Malloc failure\n");
		exit(1);
	}
	out.size = 0;
	out.cap  = 16;
	return out;
}

void string_deinit(string* str)
{
	free(str->buf);
	str->cap  = 0;
	str->size = 0;
}

void string_push(string* str, char c)
{
	if (str->size + 2 == str->cap) {
		const size_t newcap = str->cap + str->cap / 2;
		str->buf            = realloc(str->buf, newcap);
	}
	str->buf[str->size++] = c;
	str->buf[str->size]   = '\0';
}
