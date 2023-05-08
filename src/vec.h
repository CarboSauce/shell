#ifndef VEC_H
#define VEC_H
#include <stddef.h>
#include <string.h>

#define VEC_DECLARE(name, type)                                  \
	typedef struct name {                                        \
		type* buf;                                               \
		size_t size;                                             \
		size_t cap;                                              \
	} name;                                                      \
                                                                 \
	static name name##_init()                                    \
	{                                                            \
		name out;                                                \
		out.buf = malloc(8 * sizeof(type));                      \
		if (out.buf == NULL) {                                   \
			fprintf(stderr, "Critical error: Malloc failure\n"); \
			exit(1);                                             \
		}                                                        \
		out.size = 0;                                            \
		out.cap = 8;                                             \
		return out;                                              \
	}                                                            \
                                                                 \
	static void name##_deinit(name* vec)                         \
	{                                                            \
		free(vec->buf);                                          \
		vec->cap = 0;                                            \
		vec->size = 0;                                           \
	}                                                            \
                                                                 \
	static void name##_push(name* vec, type* data)               \
	{                                                            \
		if (vec->size + 1 == vec->cap) {                         \
			const size_t newcap = vec->cap + vec->cap / 2;       \
			vec->buf = realloc(vec->buf, newcap * sizeof(type)); \
		}                                                        \
		memcpy(&vec->buf[vec->size], data, sizeof(type));        \
		vec->size++;                                             \
	}

#endif
