#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "extension.hh"

static void initialize_libc_stdlib_tests () {
  test(atoi("123") == 123);
  test(atoi("abc") == 0);
  test(atof("123.456") == 123.456);
  test(atof("def") == 0.0);
  test(atol("123456") == 123456L);
  test(atol("abc") == 0L);

  test(getenv("FOO") == NULL);
  test(getenv(NULL) == NULL);

  test(setenv("FOO", "bar", 0) == 0);
  test(getenv("FOO") != NULL);

  auto d = div(10, 3);
  test(d.quot == 3 && d.rem == 1);

  auto ptr = malloc(32);
  test(ptr != NULL);
}

static void initialize_libc_string_tests () {
  {
    char dst[BUFSIZ] = {};
    const char* src = "a string to copy";
    test(memcpy(dst, src, 16) == dst);
    test(memcmp(dst, src, 16) == 0);
  }

  {
    char dst[BUFSIZ] = {};
    const char* src = "a string to copy";
    test(memmove(dst, src, 16) == dst);
    test(memcmp(dst, src, 16) == 0);
  }

  {
    char dst[BUFSIZ] = {};
    char expected[] = "cccccccccccccccc";
    test(memset(dst, 'c', 16) == dst);
    test(memcmp(dst, expected, 16) == 0);
  }

  {
    char expected[] = "def";
    const char* string = "abcdef";
    const char* chr = NULL;

    test((chr = (const char*) memchr(string, 'd', 6)) != 0);
    test(memcmp(chr, expected, 3) == 0);
  }

  {
    const char* src = "abcdef";
    char dst[BUFSIZ];
    test(strcpy(dst, src) == dst);
    test(memcmp(dst, src, 6) == 0);
  }

  {
    const char* src = "abcdefghi";
    char dst[BUFSIZ];
    test(strncpy(dst, src, 6) == dst);
    test(memcmp(dst, src, 6) == 0);
  }

  {
    const char* src = "abcdefghi";
    char dst[BUFSIZ];
    test(strcat(dst, src) == dst);
    test(memcmp(dst, src, 9) == 0);
  }

  {
    const char* src = "abcdefghi";
    char dst[BUFSIZ];
    test(strncat(dst, src, 6) == dst);
    test(memcmp(dst, src, 6) == 0);
  }

  {
    const char* src = "abcdefghi";
    char dst[BUFSIZ];
    test(strcat(dst, src) == dst);
    test(strcmp(dst, src) == 0);
  }

  {
    const char* src = "abcdefghi";
    char dst[BUFSIZ];
    test(strcat(dst, src) == dst);
    test(strncmp(dst, src, 2) == 0);
    test(strncmp(dst, src, 4) == 0);
    test(strncmp(dst, src, 6) == 0);
    test(strncmp(dst + 2, src + 2, 6) == 0);
    test(strncmp(dst + 4, src, 6) > 0);
    test(strncmp(dst, src + 4, 6) < 0);
  }
}

static void initialize_libc_time_tests () {
  {
    time_t timeValue = 0;;
    const auto t1 = time(NULL);
    const auto t2 = time(&timeValue);
    test(t1 == t2);
    test(t1 == timeValue);
    test(t2 == timeValue);
  }

  {
    time_t t1 = time(NULL);
    time_t t2 = t1 + 1024;
    test(difftime(t1, t2) == 1024.0);;
  }
}

void initialize_libc_tests () {
  initialize_libc_stdlib_tests();
  initialize_libc_string_tests();
  initialize_libc_time_tests();
}
