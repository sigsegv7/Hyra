/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#define __need_NULL
#define __need_size_t
#include <stddef.h>
#define __need_va_list
#include <stdarg.h>

#if __STDC_VERSION__ >= 202311L
#define __STDC_VERSION_STDIO_H__ 202311L
#endif

/* Buffering modes */
#define _IOFBF 0 /* Fully buffered */
#define _IOLBF 1 /* Line buffered */
#define _IONBF 2 /* Unbuffered */

/* Default buffer size */
#define BUFSIZ 256

/* End-Of-File indicator */
#define EOF (-1)

/* Spec says these should be defined as macros */
#define stdin  stdin
#define stdout stdout
#define stderr stderr

/* File structure */
typedef struct _IO_FILE {
    int fd;
    int buf_mode;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define putc(c, stream) fputc((c), (stream))
#define getc(stream)    fgetc((stream))

__BEGIN_DECLS

size_t fread(void *__restrict ptr, size_t size, size_t n, FILE *__restrict stream);
size_t fwrite(const void *__restrict ptr, size_t size, size_t n, FILE *__restrict stream);

long ftell(FILE *stream);
char *fgets(char *__restrict s, int size, FILE *__restrict stream);

FILE *fopen(const char *__restrict path, const char *__restrict mode);
int fclose(FILE *stream);

int vsnprintf(char *s, size_t size, const char *fmt, va_list ap);
int snprintf(char *s, size_t size, const char *fmt, ...);

int printf(const char *__restrict fmt, ...);
int fileno(FILE *stream);
int fputc(int c, FILE *stream);

int putchar(int c);
int fgetc(FILE *stream);
int getchar(void);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
int fputs(const char *__restrict s, FILE *__restrict stream);
#else
int fputs(const char *s, FILE *stream);
#endif
int puts(const char *s);

__END_DECLS

#endif /* !_STDIO_H */
