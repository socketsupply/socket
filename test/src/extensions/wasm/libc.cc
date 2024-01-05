#include <float.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "extension.hh"

static bool fequal (double a, double b) {
  auto c = fabs(a - b);
  return c == 0.0f || c < FLT_EPSILON;
}

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

static void initialize_libc_errno_tests () {
  #define TEST_ERRNO_VALUE(code, message) ({                                   \
    errno = code;                                                              \
    test(strcmp(strerror(errno), message) == 0);                               \
    errno = 0;                                                                 \
  })

  TEST_ERRNO_VALUE(0, "Undefined error: 0");
  TEST_ERRNO_VALUE(EPERM, "Operation not permitted");
  TEST_ERRNO_VALUE(ENOENT, "No such file or directory");
  TEST_ERRNO_VALUE(ESRCH, "No such process");
  TEST_ERRNO_VALUE(EINTR, "Interrupted system call");
  TEST_ERRNO_VALUE(EIO, "Input/output error");
  TEST_ERRNO_VALUE(ENXIO, "Device not configured");
  TEST_ERRNO_VALUE(E2BIG, "Argument list too long");
  TEST_ERRNO_VALUE(ENOEXEC, "Exec format error");
  TEST_ERRNO_VALUE(EBADF, "Bad file descriptor");
  TEST_ERRNO_VALUE(ECHILD, "No child processes");
  TEST_ERRNO_VALUE(EAGAIN, "Try again");
  TEST_ERRNO_VALUE(ENOMEM, "Out of memory");
  TEST_ERRNO_VALUE(EACCES, "Permission denied");
  TEST_ERRNO_VALUE(EFAULT, "Bad address");
  TEST_ERRNO_VALUE(ENOTBLK, "Block device required");
  TEST_ERRNO_VALUE(EBUSY, "Device or resource busy");
  TEST_ERRNO_VALUE(EEXIST, "File exists");
  TEST_ERRNO_VALUE(EXDEV, "Cross-device link");
  TEST_ERRNO_VALUE(ENODEV, "No such device");
  TEST_ERRNO_VALUE(ENOTDIR, "Not a directory");
  TEST_ERRNO_VALUE(EISDIR, "Is a directory");
  TEST_ERRNO_VALUE(EINVAL, "Invalid argument");
  TEST_ERRNO_VALUE(ENFILE, "File table overflow");
  TEST_ERRNO_VALUE(EMFILE, "Too many open files");
  TEST_ERRNO_VALUE(ENOTTY, "Not a typewriter");
  TEST_ERRNO_VALUE(ETXTBSY, "Text file busy");
  TEST_ERRNO_VALUE(EFBIG, "File too large");
  TEST_ERRNO_VALUE(ENOSPC, "No space left on device");
  TEST_ERRNO_VALUE(ESPIPE, "Illegal seek");
  TEST_ERRNO_VALUE(EROFS, "Read-only file system ");
  TEST_ERRNO_VALUE(EMLINK, "Too many links");
  TEST_ERRNO_VALUE(EPIPE, "Broken pipe");
  TEST_ERRNO_VALUE(EDOM, " Numerical argument out of domain");
  TEST_ERRNO_VALUE(ERANGE, "umerical result out of range");
  TEST_ERRNO_VALUE(EDEADLK, "Resource deadlock avoided");
  TEST_ERRNO_VALUE(ENAMETOOLONG, "File name too long");
  TEST_ERRNO_VALUE(ENOLCK, "No locks available");
  TEST_ERRNO_VALUE(ENOSYS, "Function not implemented");
  TEST_ERRNO_VALUE(ENOTEMPTY, "Directory not empty");
  TEST_ERRNO_VALUE(ELOOP, "Too many levels of symbolic links");
  TEST_ERRNO_VALUE(EWOULDBLOCK, "Try again");
  TEST_ERRNO_VALUE(ENOMSG, "No message of desired type");
  TEST_ERRNO_VALUE(EIDRM, "Identifier removed");
  TEST_ERRNO_VALUE(ECHRNG, "Character range error");
  TEST_ERRNO_VALUE(EL2NSYNC, "Level 2 not synchronized");
  TEST_ERRNO_VALUE(EL3HLT, "Level 3 halted");
  TEST_ERRNO_VALUE(EL3RST, "EL3RST");
  TEST_ERRNO_VALUE(ELNRNG, "ELNRNG");
  TEST_ERRNO_VALUE(EUNATCH, "No such device");
  TEST_ERRNO_VALUE(ENOCSI, "No CSI structure available");
  TEST_ERRNO_VALUE(EL2HLT, "Level 2 halted");
  TEST_ERRNO_VALUE(EBADE, "Invalid exchange");
  TEST_ERRNO_VALUE(EBADR, "Invalid request descriptor");
  TEST_ERRNO_VALUE(EXFULL, "Exchange full");
  TEST_ERRNO_VALUE(ENOANO, "No anode");
  TEST_ERRNO_VALUE(EBADRQC, "Invalid request code");
  TEST_ERRNO_VALUE(EBADSLT, "Invalid slot");
  TEST_ERRNO_VALUE(EDEADLOCK, "Resource deadlock avoided");
  TEST_ERRNO_VALUE(EBFONT, "Bad font file format");
  TEST_ERRNO_VALUE(ENOSTR, "Device not a stream");
  TEST_ERRNO_VALUE(ENODATA, "No data available");
  TEST_ERRNO_VALUE(ETIME, "Timer expired");
  TEST_ERRNO_VALUE(ENOSR, "Out of streams resources");
  TEST_ERRNO_VALUE(ENONET, "Machine is not on the network");
  TEST_ERRNO_VALUE(ENOPKG, "Package not installed");
  TEST_ERRNO_VALUE(EREMOTE, "Object is remote");
  TEST_ERRNO_VALUE(ENOLINK, "Link has been severed");
  TEST_ERRNO_VALUE(EADV, "Advertise error");
  TEST_ERRNO_VALUE(ESRMNT, "Srmount error");
  TEST_ERRNO_VALUE(ECOMM, "Communication error on send");
  TEST_ERRNO_VALUE(EPROTO, "Protocol error");
  TEST_ERRNO_VALUE(EMULTIHOP, "Multihop attempted");
  TEST_ERRNO_VALUE(EDOTDOT, "RFS specific error");
  TEST_ERRNO_VALUE(EBADMSG, "Not a data message");
  TEST_ERRNO_VALUE(EOVERFLOW, "Value too large for defined data type");
  TEST_ERRNO_VALUE(ENOTUNIQ, "Name not unique on network");
  TEST_ERRNO_VALUE(EBADFD, "File descriptor in bad state");
  TEST_ERRNO_VALUE(EREMCHG, "Remote address changed");
  TEST_ERRNO_VALUE(ELIBACC, "Can not access a needed shared library");
  TEST_ERRNO_VALUE(ELIBBAD, "Accessing a corrupted shared library");
  TEST_ERRNO_VALUE(ELIBSCN, ".lib section in a.out corrupted");
  TEST_ERRNO_VALUE(ELIBMAX, "Attempting to link in too many shared libraries");
  TEST_ERRNO_VALUE(ELIBEXEC, "Cannot exec a shared library directly");
  TEST_ERRNO_VALUE(EILSEQ, "Illegal byte sequence");
  TEST_ERRNO_VALUE(ERESTART, "Interrupted system call should be restarted");
  TEST_ERRNO_VALUE(ESTRPIPE, "Streams pipe error");
  TEST_ERRNO_VALUE(EUSERS, "Too many users");
  TEST_ERRNO_VALUE(ENOTSOCK, "Socket operation on non-socket");
  TEST_ERRNO_VALUE(EDESTADDRREQ, "Destination address required");
  TEST_ERRNO_VALUE(EMSGSIZE, "Message too long");
  TEST_ERRNO_VALUE(EPROTOTYPE, "Protocol wrong type for socket");
  TEST_ERRNO_VALUE(ENOPROTOOPT, "Protocol not available");
  TEST_ERRNO_VALUE(EPROTONOSUPPORT, "Protocol not supported");
  TEST_ERRNO_VALUE(ESOCKTNOSUPPORT, "Socket type not supported");
  TEST_ERRNO_VALUE(EOPNOTSUPP, "Operation not supported");
  TEST_ERRNO_VALUE(ENOTSUP, "Operation not supported");
  TEST_ERRNO_VALUE(EPFNOSUPPORT, "Protocol family not supported");
  TEST_ERRNO_VALUE(EAFNOSUPPORT, "Address family not supported by protocol");
  TEST_ERRNO_VALUE(EADDRINUSE, "Address already in use");
  TEST_ERRNO_VALUE(EADDRNOTAVAIL, "Cannot assign requested address");
  TEST_ERRNO_VALUE(ENETDOWN, "Network is down");
  TEST_ERRNO_VALUE(ENETUNREACH, "Network is unreachable");
  TEST_ERRNO_VALUE(ENETRESET, "Network dropped connection because of reset");
  TEST_ERRNO_VALUE(ECONNABORTED, "Software caused connection abort");
  TEST_ERRNO_VALUE(ECONNRESET, "Connection reset by peer");
  TEST_ERRNO_VALUE(ENOBUFS, "No buffer space available");
  TEST_ERRNO_VALUE(EISCONN, "Transport endpoint is already connected");
  TEST_ERRNO_VALUE(ENOTCONN, "Transport endpoint is not connected");
  TEST_ERRNO_VALUE(ESHUTDOWN, "Cannot send after transport endpoint shutdown");
  TEST_ERRNO_VALUE(ETOOMANYREFS, "Too many references: cannot splice");
  TEST_ERRNO_VALUE(ETIMEDOUT, "Connection timed out");
  TEST_ERRNO_VALUE(ECONNREFUSED, "Connection refused");
  TEST_ERRNO_VALUE(EHOSTDOWN, "Host is down");
  TEST_ERRNO_VALUE(EHOSTUNREACH, "No route to host");
  TEST_ERRNO_VALUE(EALREADY, "Operation already in progress");
  TEST_ERRNO_VALUE(EINPROGRESS, "Operation now in progress");
  TEST_ERRNO_VALUE(ESTALE, "Stale NFS file handle");
  TEST_ERRNO_VALUE(EUCLEAN, "Structure needs cleaning");
  TEST_ERRNO_VALUE(ENOTNAM, "Not a XENIX named type file");
  TEST_ERRNO_VALUE(ENAVAIL, "No XENIX semaphores available");
  TEST_ERRNO_VALUE(EISNAM, "Is a named type file");
  TEST_ERRNO_VALUE(EREMOTEIO, "Remote I/O error");
  TEST_ERRNO_VALUE(EDQUOT, "Quota exceeded");
  TEST_ERRNO_VALUE(ENOMEDIUM, "No medium found");
  TEST_ERRNO_VALUE(EMEDIUMTYPE, "Wrong medium type");
  TEST_ERRNO_VALUE(ECANCELED, "Operation canceled");
  TEST_ERRNO_VALUE(ENOKEY, "Required key not available");
  TEST_ERRNO_VALUE(EKEYEXPIRED, "Key has expired");
  TEST_ERRNO_VALUE(EKEYREVOKED, "Key has been revoked");
  TEST_ERRNO_VALUE(EKEYREJECTED, "Key was rejected by service");
  TEST_ERRNO_VALUE(EOWNERDEAD, "Owner died");
  TEST_ERRNO_VALUE(ENOTRECOVERABLE, "State not recoverable");
  TEST_ERRNO_VALUE(ERFKILL, "Operation not possible due to RF-kill");
  TEST_ERRNO_VALUE(EHWPOISON, "Memory page has hardware error");

  #undef TEST_ERRNO_VALUE
}

static void variadic_string (char* output, int count, ...) {
  va_list args;
  size_t offset = 0;
  va_start(args, count);
  for (int i = 0; i < count; ++i) {
    char* arg = va_arg(args, char*);
    size_t size = strlen(arg);
    for (int j = 0; j < size; ++j) {
      output[offset + j] = arg[j];
    }

    offset += size;
  }
}

static void initialize_libc_variadic_tests () {
  char output[BUFSIZ] = {0};

  variadic_string(output, 3, "abc", "def", "ghi");
  test(strcmp(output, "abcdefghi") == 0);
}

static void initialize_libc_libgen_tests () {
  test(strcmp(basename("/"), "/") == 0);
  test(strcmp(basename("."), ".") == 0);
  test(strcmp(basename("/usr"), "usr") == 0);
  test(strcmp(basename("/usr/"), "usr") == 0);
  test(strcmp(basename("/usr/local"), "local") == 0);
  test(strcmp(basename("/usr/local/"), "local") == 0);
  test(strcmp(basename("/usr/local/bin"), "bin") == 0);
  test(strcmp(basename("/usr/local/bin/program.exe"), "program.exe") == 0);

  test(strcmp(dirname("/"), "/") == 0);
  test(strcmp(dirname("."), ".") == 0);
  test(strcmp(dirname("/usr"), "/") == 0);
  test(strcmp(dirname("/usr/"), "/") == 0);
  test(strcmp(dirname("/usr/local"), "/usr") == 0);
  test(strcmp(dirname("/usr/local/"), "/usr") == 0);
  test(strcmp(dirname("/usr/local/bin"), "/usr/local") == 0);
  test(strcmp(dirname("/usr/local/bin/program.exe"), "/usr/local/bin") == 0);

  char pathWithTooBigBasename[PATH_MAX + 2] = {0};

  pathWithTooBigBasename[0] = '/';

  for (int i = 1; i < PATH_MAX + 2; ++i) {
    pathWithTooBigBasename[i] = 'a';
  }

  char pathWithTooBigDirname[PATH_MAX + 4] = {0};
  pathWithTooBigDirname[0] = '/';
  pathWithTooBigDirname[1] = 'a';
  pathWithTooBigDirname[2] = '/';

  for (int i = 3; i < PATH_MAX + 2; ++i) {
    pathWithTooBigDirname[i] = 'b';
  }

  pathWithTooBigDirname[PATH_MAX + 2] = '/';
  pathWithTooBigDirname[PATH_MAX + 3] = 'c';

  test(basename((char*) pathWithTooBigBasename) == NULL && errno == ENAMETOOLONG);
  test(dirname((char*) pathWithTooBigDirname) == NULL && errno == ENAMETOOLONG);
}

static void initialize_libc_regex_tests () {
  regex_t regex;
  char input[] = "abbbc abc def 000 abbbbbc";
  char pattern[] = "ab+c";
  const size_t nmatch = 3;
  char* expected[] = {
    (char*) "abbbc",
    (char*) "abc",
    (char*) "abbbbbc"
  };

  regmatch_t pmatch[nmatch] = {0};

  test(regcomp(&regex, pattern, 0) == 0);
  test(regexec(&regex, input, nmatch, pmatch, 0) == 0);
  test(strncmp(expected[0], input + pmatch[0].rm_so, 5) == 0);
  test(strncmp(expected[1], input + pmatch[1].rm_so, 3) == 0);
  test(strncmp(expected[2], input + pmatch[2].rm_so, 7) == 0);
  regfree(&regex);
}

static void initialize_libc_math_tests () {
  test(fequal(acos(0.1), 1.4706289056333368));
  test(fequal(acosf(0.1234), 1.4470809809523457));
  test(fequal(acosl(0.123), 1.4474840516030247));
  test(fequal(acosh(12345.0), 10.114153580698794));
  test(fequal(acoshf(12345.6789), 10.114208221435547));
  test(fequal(acoshl(2.3456), 1.4967962513609694));

  test(fequal(asin(0.33333333), 0.339836905918588));
  test(fequal(asinf(0.98765), 1.4134716987609863));
  test(fequal(asinl(0.29384756), 0.2982496274073672));
  test(fequal(asinh(123.2938475), 5.507734136828022));
  test(fequal(asinhf(123.98765),  5.513345241546631));
  test(fequal(asinhl(888.999888777), 7.483244607290699));

  test(fequal(atan(123.456), 1.5626964520979927));
  test(fequal(atanf(456.789), 1.5686071357187537));
  test(fequal(atanl(987.654), 1.5697838268118243));

  test(fequal(atan2(123.456, 987.654), 0.12435424685414087));
  test(fequal(atan2f(987.654, 123.456), 1.4464420799407558));
  test(fequal(atan2l(0.0, 0.0), 0.0));

  test(fequal(atanh(0.998877), 3.7421685630428145));
  test(fequal(atanhf(0.112233), 0.11270783203029852));
  test(fequal(atanhl(0.123456789), 0.12408981359692224));

  test(fequal(cbrt(1.0), 1.0));
  test(fequal(cbrtf(0.123456789), 0.49793385921817446));
  test(fequal(cbrtl(1.23456789), 1.0727659796410873));

  test(fequal(ceil(1.45), 2.0));
  test(fequal(ceilf(0.45), 1));
  test(fequal(ceill(0.0), 0.0));

  test(fequal(copysign(-123.456, -32.64), -123.456));
  test(fequal(copysignf(-123.456, 0.0), 123.456f));
  test(fequal(copysignl(987.654, 1), 987.654));

  test(fequal(cos(123.456), -0.5947139710921574));
  test(fequal(cosf(987.654), 0.3680378496646881));
  test(fequal(cosl(0.11223344556), 0.9937084352371074));

  test(fequal(cosh(0.11223344556), 1.0063047870924915));
  test(fequal(coshf(0.987654321), 1.5286892059778199));
  test(fequal(coshl(0.123456789), 1.0076304736991977));

  test(fequal(erf(0.0), 1.000000082740371e-9));
  test(fequal(erf(2.0), 0.9953221395812188));
  test(fequal(erff(123.0), 1));
  test(fequal(erfl(-INFINITY), -1));

  test(fequal(erfcf(INFINITY), 0));
  test(fequal(erfcf(-INFINITY), 2));
  test(fequal(erfcf(123.456), 0));
  test(fequal(erfcl(-0.456), 0.5187334966466077));

  test(fequal(exp(0.1234), 1.131336865198686));
  test(fequal(expf(0.4321), 1.5404891563745755));
  test(fequal(expl(1.234), 3.43494186080076));

  test(fequal(exp2(1.234), 1073741824));
  test(fequal(exp2f(4.321), 1078984704));
  test(fequal(exp2l(32.0), 1120927744));

  test(fequal(expm1(1.23), 2.4212295362896734));
  test(fequal(expm1f(4.56), 94.5834732055664));
  test(fequal(expm1l(0.87654321), 1.4025801420211792));

  test(fequal(fabs(-1.234), 1.234));
  test(fequal(fabsf(-99.87654f), 99.87654f));
  test(fequal(fabsl(12345.6789), 12345.6789));

  test(fequal(fdim(2.34, 1.23), 1.1099999999999999));
  test(fequal(fdimf(1.23, 2.34), 0.0));
  test(fequal(fdiml(99.876, 88.7654), 11.110600000000005));

  test(fequal(floor(1.23), 1.0));
  test(fequal(floorf(0.0), 0));
  test(fequal(floorl(2.99), 2));

  test(fequal(fma(1.2, 3.4, 5.6), 1.2 * 3.4 + 5.6));
  test(fequal(fmaf(5.6, 3.4, 1.2), 20.239999771118164));
  test(fequal(fmal(99.88, 77.66, 55.44), 99.88 * 77.66 + 55.44));

  test(fequal(fmax(0.0, 1.0), 1.0));
  test(fequal(fmaxf(987.654, 123.456), 987.654f));
  test(fequal(fmaxl(9000.888, 1000.1234), 9000.888));

  test(fequal(fmin(123.456, 987.654), 123.456));
  test(fequal(fminf(9.87654321, 1.23456789), 1.23456788));
  test(fequal(fminl(0.000123456789, 0.0000123456789), 0.0000123456789));

  test(fequal(fmod(0.1234, -0.5678), 0.1234));
  test(fequal(fmodf(-0.001234, 0.9987), -0.0012339999666437507));
  test(fequal(fmodl(0.00000051, -1.23450001), -1.2344995));

  int exp = 0;
  test(fequal(frexp(123.456, &exp), 1.929) && exp == 6);
  test(fequal(frexpf(0.987654321, &exp), 3.9506173133850098) && exp == -2);
  test(fequal(frexpl(64.321684, &exp), 1.0050263125) && exp == 6);

  test(fequal(hypot(32, 16), 35.77708763999664));
  test(fequal(hypotf(128, 16), 128.9961239727768f));
  test(fequal(hypotl(0.321987, 0.123456), 0.3448434602903178));

  test(fequal(ilogb(123.456), 6.0));
  test(fequal(ilogbf(0.123456), -4.0));
  test(fequal(ilogbl(123456), 16));

  test(fequal(ldexp(123.456, 987), 1.6147969556736795e+299L));
  test(fequal(ldexpf(0.123, 2), 0.492f));
  test(fequal(ldexpl(999.888, 456), 1.8604987349977237e+140L));

  test(fequal(lgamma(123), 462.6081785268749L));
  test(fequal(lgammaf(123.9987), 467.4059753417969F));
  test(fequal(lgammal(99887766.5544332211L), 1740000694.2205663L));

  test(fequal(tgamma(0.7531), 0.4642265214333058F));
  test(fequal(tgammaf(0.1357), 17.936216354370117F));
  test(fequal(tgammal(0.13571357L), 17.934361673027155L));

  test(fequal(rint(0.88246), 1.0));
  test(fequal(rintf(42.682462), 43.0));
  test(fequal(rintl(1357.6824629998L), 1358));
  test(lrint(8662289) == 8662289);
  test(fequal(lrintf(0.8662289), 0));
  test(lrintl(0.3268) == 0);
  test(fequal(llrint(32.64), 33.0));
  test(llrintf(-2.3264L) == -2L);
  test(fequal(llrintl(42.99876L), 43.0));

  test(fequal(lround(-2.32), -2.0));
  // test(lroundf());
  // test(lroundl());
  // test(llround());
  // test(llroundf());
  // test(llroundl());

  // test(log());
  // test(logf());
  // test(logl());
  // test(log10());
  // test(log10f());
  // test(log10l());
  // test(log1p());
  // test(log1pf());
  // test(log1pl());
  // test(log2());
  // test(log2f());
  // test(log2l());
  // test(logb());
  // test(logbf());
  // test(logbl());

  // test(modf());
  // test(modff());
  // test(modfl());

  // test(nearbyint());
  // test(nearbyintf());
  // test(nearbyintl());

  // test(nextafter());
  // test(nextafterf());
  // test(nextafterl());

  // test(nexttoward());
  // test(nexttowardf());
  // test(nexttowardl());

  // test(pow());
  // test(powf());
  // test(powl());

  // test(remainder());
  // test(remainderf());
  // test(remainderl());

  // test(remquo());
  // test(remquof());
  // test(remquol());

  // test(round());
  // test(roundf());
  // test(roundl());

  // test(scalbln());
  // test(scalblnf());
  // test(scalblnl());
  // test(scalbn());
  // test(scalbnf());
  // test(scalbnl());

  // test(sin());
  // test(sinf());
  // test(sinl());
  // test(sinh());
  // test(sinhf());
  // test(sinhl());

  // test(sqrt());
  // test(sqrtf());
  // test(sqrtl());

  // test(tan());
  // test(tanf());
  // test(tanl());
  // test(tanh());
  // test(tanhf());
  // test(tanhl());

  // test(trunc());
  // test(truncf());
  // test(truncl());
}

void initialize_libc_tests () {
  initialize_libc_stdlib_tests();
  initialize_libc_string_tests();
  initialize_libc_time_tests();
  initialize_libc_errno_tests();
  initialize_libc_regex_tests();
  initialize_libc_variadic_tests();
  initialize_libc_libgen_tests();
  initialize_libc_math_tests();
}
