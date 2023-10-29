/**
 * `ok.h` - libok
 *
 * Copyright (C) 2014-2023 Joseph Werle <joseph.werle@gmail.com>
 */

#ifndef OK_H
#define OK_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * libok version
 */
#ifndef OK_VERSION
#define OK_VERSION "0.6.5"
#endif

/**
 * No-op/void `ok()` function
 */
#ifndef okx
#define okx(...) (void) (0);
#endif

/**
 * Allow for custom `printf` implementation.
 */
#if !defined(LIBOK_PRINTF)
#define LIBOK_PRINTF(...) (printf(__VA_ARGS__))
#endif

/**
 * Allow for custom `fprintf` implementation.
 */
#if !defined(LIBOK_FPRINTF)
#define LIBOK_FPRINTF(...) (fprintf(__VA_ARGS__))
#endif

/**
 * Configure the need for a newline ('\n') in a `printf` call.
 */
#if !defined(LIBOK_PRINTF_NEEDS_NEWLINE)
#define LIBOK_PRINTF_NEEDS_NEWLINE 1
#endif

/**
 * Configure the need for a newline ('\n') in a `fprintf` call.
 */
#if !defined(LIBOK_FPRINTF_NEEDS_NEWLINE)
#define LIBOK_FPRINTF_NEEDS_NEWLINE LIBOK_PRINTF_NEEDS_NEWLINE
#endif

/**
 * Increments ok count and
 * outputs a message to stdout
 */
#define ok(format, ...) ({                                                     \
  if (ok_count() == 0 && ok_failed() == 0) ok_begin(NULL);                     \
  int count = ok_count_inc() + ok_failed();                                    \
  LIBOK_PRINTF("ok %d - " format, count, ##__VA_ARGS__);                       \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
})

/**
 * Outputs a a "not ok"  message to stdout.
 * Increments not ok count.
 */
#define notok(format, ...) ({                                                  \
  if (ok_count() == 0 && ok_failed() == 0) ok_begin(NULL);                     \
  int count = ok_count() + ok_failed_inc();                                    \
  LIBOK_PRINTF("not ok %d - " format, count, ##__VA_ARGS__);                   \
                                                                               \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
                                                                               \
  LIBOK_PRINTF("   ---");                                                      \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
                                                                               \
  LIBOK_PRINTF("   severity: fail");                                           \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
                                                                               \
  LIBOK_PRINTF("   stack:    |-");                                             \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
                                                                               \
  LIBOK_PRINTF("          at %s (%s:%d)",                                      \
    __FUNCTION__,                                                              \
    __FILE__,                                                                  \
    __LINE__                                                                   \
  );                                                                           \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
                                                                               \
  LIBOK_PRINTF("   ...");                                                      \
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {                                            \
    LIBOK_PRINTF("\n");                                                        \
  }                                                                            \
})

/**
 * Called at the beginning of a test with an optional (NULL) label.
 */
void ok_begin (const char *label);

/**
 * Can be used to printf a comment.
 */
void ok_comment (const char *comment);

/**
 * Can be used to provide a multiline test explaination.
 */
void ok_explain (const char *explaination);

/**
 * Can be used to issue an emergency "Bail out!" statement with an
 * optional comment.
 */
void ok_bail (const char *comment);

/**
 * Completes tests and asserts that
 * the expected test count matches the
 * actual test count if the expected
 * count is greater than 0
 */
bool ok_done (void);

/**
 * Sets the expectation count
 */
void ok_expect (int);

/**
 * Returns the expected count
 */
int ok_expected (void);

/**
 * Returns the ok count
 */
int ok_count (void);
int ok_count_inc (void);

/**
 * Returns the not ok count
 */
int ok_failed (void);
int ok_failed_inc (void);

/**
 * Resets count and expected counters
 */
void ok_reset (void);

#if defined(LIBOK_INCLUDE_IMPLEMENTATION)

static int ok_count_;
static int ok_failed_;
static int ok_expected_;
static bool ok_begin_;
static bool ok_header_printed_;

void ok_comment (const char *comment) {
  LIBOK_PRINTF("# %s", comment);
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  }
}

void ok_explain (const char *explaination) {
  if (ok_count() == 0 && ok_failed() == 0) ok_begin(NULL);
  ok_comment("");
  int i = 0;
  while (explaination[i] != '\0') {
    const char* pointer = explaination + i;
    int j = i;

    while (explaination[i] != '\n' && explaination[i] != '0') {
      i++;
    }

    j = i - j;
    LIBOK_PRINTF("# %.*s", j, pointer);
    if (LIBOK_PRINTF_NEEDS_NEWLINE) {
      LIBOK_PRINTF("\n");
    }

    i++;
  }
  ok_comment("");
}

void ok_bail (const char *comment) {
  LIBOK_PRINTF("Bail out!");

  if (comment != NULL) {
    LIBOK_PRINTF(" %s", comment);
  }

  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  }
}

void ok_begin (const char *label) {
  if (ok_begin_) return;
  ok_begin_ = true;

  // print new line for new test
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  } else {
    LIBOK_PRINTF("");
  }

  if (!ok_header_printed_) {
    ok_header_printed_ = true;
    LIBOK_PRINTF("TAP version 14");
    if (LIBOK_PRINTF_NEEDS_NEWLINE) {
      LIBOK_PRINTF("\n");
    }
  }

  if (label != NULL) {
    LIBOK_PRINTF("# %s", label);
    if (LIBOK_PRINTF_NEEDS_NEWLINE) {
      LIBOK_PRINTF("\n");
    }
  }
}

bool ok_done (void) {
  if (ok_count() == 0 && ok_failed() == 0) ok_begin(NULL);

  const int expected = ok_expected();
  const int failed = ok_failed();
  const int count = ok_count();
  bool success = true;

  if (0 != expected && count != expected) {
    if (expected > count) {
      LIBOK_FPRINTF(stderr, "expected number of success conditions not met.");
      if (LIBOK_FPRINTF_NEEDS_NEWLINE) {
        LIBOK_FPRINTF(stderr, "\n");
      }
    } else {
      LIBOK_FPRINTF(stderr,
        "expected number of success conditions is less than the "
        "number of given success conditions.");
      if (LIBOK_FPRINTF_NEEDS_NEWLINE) {
        LIBOK_FPRINTF(stderr, "\n");
      }
    }

    success = false;
  }

  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  } else {
    LIBOK_PRINTF("");
  }

  if (expected == 0) {
    LIBOK_PRINTF("1..%d", count + failed);
    if (LIBOK_PRINTF_NEEDS_NEWLINE) {
      LIBOK_PRINTF("\n");
    }
  }

  LIBOK_PRINTF("# tests %d", count + failed);
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  }

  LIBOK_PRINTF("# pass  %d", count);
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  }

  if (failed > 0) {
    LIBOK_PRINTF("# fail  %d", failed);
    if (LIBOK_PRINTF_NEEDS_NEWLINE) {
      LIBOK_PRINTF("\n");
    }

    success = false;
  }

  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  } else {
    LIBOK_PRINTF("");
  }

  return success;
}

void ok_expect (int expected) {
  if (ok_count() == 0 && ok_failed() == 0) ok_begin(NULL);
  ok_expected_ = expected;
  LIBOK_PRINTF("1..%d", expected);
  if (LIBOK_PRINTF_NEEDS_NEWLINE) {
    LIBOK_PRINTF("\n");
  }
}

int ok_expected (void) {
  return ok_expected_;
}

int ok_count_inc (void) {
  return ++ok_count_;
}

int ok_count (void) {
  return ok_count_;
}

int ok_failed_inc (void) {
  return ++ok_failed_;
}

int ok_failed (void) {
  return ok_failed_;
}

void ok_reset (void) {
  ok_begin_ = false;
  ok_count_ = 0;
  ok_failed_ = 0;
  ok_expected_ = 0;
}

#endif
#ifdef __cplusplus
}
#endif
#endif
