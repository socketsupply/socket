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

static void initialize_libc_errno_tests () {
  #define TEST_ERRNO_VALUE(code, message) ({     \
    errno = code;                                \
    test(strcmp(strerror(errno), message) == 0); \
    errno = 0;                                   \
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
}

void initialize_libc_tests () {
  initialize_libc_stdlib_tests();
  initialize_libc_string_tests();
  initialize_libc_time_tests();
  initialize_libc_errno_tests();
}
