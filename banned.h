/* SPDX-License-Identifier: ISC */
#ifndef BANNED_H
#define BANNED_H

/*
 * A number of libc functions have severe security implications or lack
 * bounds checking.  Ban them at compile time.
 */

/* no buffer-overrun fuel */
#define strcpy(a, b)        BANNED_strcpy(a, b)
#define strcat(a, b)        BANNED_strcat(a, b)
#define sprintf(...)        BANNED_sprintf(__VA_ARGS__)
#define vsprintf(...)       BANNED_vsprintf(__VA_ARGS__)

/* goto fail / double-free footguns */
#define gets(s)             BANNED_gets(s)
#define tmpnam(s)           BANNED_tmpnam(s)
#define mktemp(s)           BANNED_mktemp(s)
#define strtok(a, b)        BANNED_strtok(a, b)

/* deprecated, replace with strlcpy / mempcpy */
#define strncpy(a, b, c)    BANNED_strncpy(a, b, c)

#endif
