/*
 * fcntl.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Access constants for _open. Note that the permissions constants are
 * in sys/stat.h (ick).
 *
 */
#ifndef _FCNTL_H_
#define _FCNTL_H_

/*
 * It appears that fcntl.h should include io.h for compatibility...
 */
#include <io.h>

#include <fcntl.h> // include MSVC fcntl.h ... note that it's not sys/fcntl.h -.-

#ifndef	_NO_OLDNAMES

/* POSIX/Non-ANSI names for increased portability */
#define O_RDONLY        _O_RDONLY
#define O_WRONLY        _O_WRONLY
#define O_RDWR          _O_RDWR
#define O_APPEND        _O_APPEND
#define O_CREAT         _O_CREAT
#define O_TRUNC         _O_TRUNC
#define O_EXCL          _O_EXCL
#define O_TEXT          _O_TEXT
#define O_BINARY        _O_BINARY
#define O_RAW           _O_BINARY
#define O_TEMPORARY     _O_TEMPORARY
#define O_NOINHERIT     _O_NOINHERIT
#define O_SEQUENTIAL    _O_SEQUENTIAL
#define O_RANDOM        _O_RANDOM

#endif	/* Not _NO_OLDNAMES */

// flock constants!

#define LOCK_SH      1
#define LOCK_EX      2
#define LOCK_NB      4
#define LOCK_UN      8
#define LOCK_MAND    32
#define LOCK_READ    64
#define LOCK_WRITE   128
#define LOCK_RW      192
#define flock(a, b)  0
#define F_WRLCK      1
#define F_UNLCK      2

#endif	/* Not _FCNTL_H_ */