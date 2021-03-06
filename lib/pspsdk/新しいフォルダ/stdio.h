/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * stdio.h
 *
 * Copyright (c) 2002-2004 PS2DEV
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 * Copyright (c) 2005 James Forshaw <tyranid@gmail.com>
 * Copyright (c) 2005 John Kelley <ps2dev@kelley.ca>
 *
 * $Id: stdio.h 1095 2005-09-27 21:02:16Z jim $
 */

#ifndef __STDIO_H__
#define __STDIO_H__

#include <pspkernel.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Some aliases for the unix 'unistd' functions */
static __inline__ int open(const char *fname, int flags, ...) { return sceIoOpen(fname, flags, 0777); }
static __inline__ int close(int handle) { return sceIoClose(handle); }
static __inline__ ssize_t read(int handle, void * buffer, size_t size) { return sceIoRead(handle, buffer, size); }
static __inline__ ssize_t write(int handle, const void * buffer, size_t size) { return sceIoWrite(handle, buffer, size); }
static __inline__ off_t lseek(int handle, off_t position, int wheel) { return sceIoLseek(handle, position, wheel); }
static __inline__ off_t tell(int handle) { return sceIoLseek(handle, 0, 1); }

/* Some win32 equivalents... baaah */
#define _open open
#define _close close
#define _read read
#define _write write
#define _lseek lseek

#define _O_APPEND O_APPEND
#define _O_BINARY O_BINARY
#define _O_CREAT  O_CREAT
#define _O_RDONKY O_RDONKY
#define _O_RDWR   O_RDWR
#define _O_TEXT   O_TEXT
#define _O_TRUNC  O_TRUNC
#define _O_WRONLY O_WRONLY

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif


#define BUFSIZ                         1024

#define _NFILE                         16


#define _IOFBF                         0x0000
#define _IOLBF                         0x0100
#define _IONBF                         0x0004
#define _IOEOF                         0x0020
#define _IOERR                         0x0040

#define _IOREAD                        0x0001
#define _IOWRT                         0x0002
#define _IORW                          0x0200
#define _IOMYBUF                       0x0010


/* ensure EOF is defined. */
#ifndef EOF
#define EOF                            (-1)
#endif


#define FOPEN_MAX                      _NFILE
#define FILENAME_MAX                   1024


#define SEEK_SET                       0
#define SEEK_CUR                       1
#define SEEK_END                       2


/* ensure fpos_t is defined. */
#ifndef __FPOS_T_DEFINED
#define __FPOS_T_DEFINED
typedef long fpos_t;
#endif // __FPOS_T_DEFINED


/* ensure FILE is defined. */
#ifndef __FILE_DEFINED
#define __FILE_DEFINED
typedef struct {
  int  type;
  int  fd;
  int  cnt;
  int  flag;
  int  has_putback;
  u8   putback;
} FILE;
#endif // __FILE_DEFINED


extern FILE __iob[_NFILE];


#define stdin                          (&__iob[0])
#define stdout                         (&__iob[1])
#define stderr                         (&__iob[2])


/* function declarations. */
void   clearerr(FILE *);
int    feof(FILE *);
int    ferror(FILE *);
FILE   *fopen(const char *, const char *);
FILE   *fdopen(int, const char *);
int    fclose(FILE *);
int    fflush(FILE *);
int    fgetc(FILE *);
int    fgetpos(FILE *, fpos_t *);
char   *fgets(char *, int, FILE *);
int    fileno(FILE *);
int    fputc(int, FILE *);
int    fputs(const char *, FILE *);
size_t fread(void *, size_t, size_t, FILE *);
FILE   *freopen(const char *, const char *, FILE *);
int    fscanf(FILE *, const char *, ...);
int    fseek(FILE *, long, int);
int    fsetpos(FILE *, const fpos_t *);
long   ftell(FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int    getc(FILE *);
int    getchar(void);
char   *gets(char *);
void   perror(const char *);
int    putc(int, FILE *);
int    puts(const char *);
int    remove(const char *);
int    rename(const char *, const char *);
void   rewind(FILE *);
int    scanf(const char *, ...);
int    setbuf(FILE *, char *);
int    setvbuf(FILE *, char *, int, size_t);
int    sscanf(const char *, const char *, ...);
FILE   *tmpfile(void);
char   *tmpnam(char *);
int    vfscanf(FILE *, const char *, va_list);
int    vscanf(const char *, va_list);
int    vsscanf(const char *, const char *, va_list);
int    vxscanf(int (*xgetc)(void **), void (*xungetc)(int, void **), void *stream, const char *, va_list);
int    xscanf(int (*xgetc)(void **), void (*xungetc)(int, void **), void *stream, const char *, ...);

int    ungetc(int, FILE *);

int    _fcloseall(void);
int    _fflushall(void);

int    chdir(const char *path);


/* from xprintf */
int vxprintf(void (*func)(char*, int, void *), void * arg, const char * format, va_list ap);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
char *vmprintf(const char *zFormat, va_list ap);
int vfprintf(FILE *pOut, const char *zFormat, va_list ap);
int vprintf(const char *format, va_list ap);
int vasprintf(char **strp, const char *format, va_list ap);

int xprintf(void (*func)(char*, int, void *), void * arg, const char * format, ...)
    __attribute__((format(printf,3,4)));
int snprintf(char *str, size_t sz, const char *format, ...)
    __attribute__((format(printf,3,4)));
int sprintf(char *buf, const char *fmt, ...)
    __attribute__((format(printf,2,3)));
char *mprintf(const char *zFormat, ...)
    __attribute__((format(printf,1,2)));
int fprintf(FILE *pOut, const char *zFormat, ...)
    __attribute__((format(printf,2,3)));
int printf(const char *format, ...)
    __attribute__((format(printf,1,2)));
int asprintf(char **strp, const char *format, ...)
    __attribute__((format(printf,2,3)));

int putchar(int);

int npmPuts(const char *buf);
int nprintf(const char *format, ...);
int vnprintf(const char *format, va_list args);
int sio_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif


#endif // __STDIO_H__
