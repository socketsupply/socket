#include "../os.hh"

namespace ssc::runtime::os {
  #define CONSTANT(c) { #c, (c) },
  static const Map<String, int32_t> CONSTANTS = {
    #if defined(E2BIG)
    CONSTANT(E2BIG)
    #endif
    #if defined(EACCES)
    CONSTANT(EACCES)
    #endif
    #if defined(EADDRINUSE)
    CONSTANT(EADDRINUSE)
    #endif
    #if defined(EADDRNOTAVAIL)
    CONSTANT(EADDRNOTAVAIL)
    #endif
    #if defined(EAFNOSUPPORT)
    CONSTANT(EAFNOSUPPORT)
    #endif
    #if defined(EAGAIN)
    CONSTANT(EAGAIN)
    #endif
    #if defined(EALREADY)
    CONSTANT(EALREADY)
    #endif
    #if defined(EBADF)
    CONSTANT(EBADF)
    #endif
    #if defined(EBADMSG)
    CONSTANT(EBADMSG)
    #endif
    #if defined(EBUSY)
    CONSTANT(EBUSY)
    #endif
    #if defined(ECANCELED)
    CONSTANT(ECANCELED)
    #endif
    #if defined(ECHILD)
    CONSTANT(ECHILD)
    #endif
    #if defined(ECONNABORTED)
    CONSTANT(ECONNABORTED)
    #endif
    #if defined(ECONNREFUSED)
    CONSTANT(ECONNREFUSED)
    #endif
    #if defined(ECONNRESET)
    CONSTANT(ECONNRESET)
    #endif
    #if defined(EDEADLK)
    CONSTANT(EDEADLK)
    #endif
    #if defined(EDESTADDRREQ)
    CONSTANT(EDESTADDRREQ)
    #endif
    #if defined(EDOM)
    CONSTANT(EDOM)
    #endif
    #if defined(EDQUOT)
    CONSTANT(EDQUOT)
    #endif
    #if defined(EEXIST)
    CONSTANT(EEXIST)
    #endif
    #if defined(EFAULT)
    CONSTANT(EFAULT)
    #endif
    #if defined(EFBIG)
    CONSTANT(EFBIG)
    #endif
    #if defined(EHOSTUNREACH)
    CONSTANT(EHOSTUNREACH)
    #endif
    #if defined(EIDRM)
    CONSTANT(EIDRM)
    #endif
    #if defined(EILSEQ)
    CONSTANT(EILSEQ)
    #endif
    #if defined(EINPROGRESS)
    CONSTANT(EINPROGRESS)
    #endif
    #if defined(EINTR)
    CONSTANT(EINTR)
    #endif
    #if defined(EINVAL)
    CONSTANT(EINVAL)
    #endif
    #if defined(EIO)
    CONSTANT(EIO)
    #endif
    #if defined(EISCONN)
    CONSTANT(EISCONN)
    #endif
    #if defined(EISDIR)
    CONSTANT(EISDIR)
    #endif
    #if defined(ELOOP)
    CONSTANT(ELOOP)
    #endif
    #if defined(EMFILE)
    CONSTANT(EMFILE)
    #endif
    #if defined(EMLINK)
    CONSTANT(EMLINK)
    #endif
    #if defined(EMSGSIZE)
    CONSTANT(EMSGSIZE)
    #endif
    #if defined(EMULTIHOP)
    CONSTANT(EMULTIHOP)
    #endif
    #if defined(ENAMETOOLONG)
    CONSTANT(ENAMETOOLONG)
    #endif
    #if defined(ENETDOWN)
    CONSTANT(ENETDOWN)
    #endif
    #if defined(ENETRESET)
    CONSTANT(ENETRESET)
    #endif
    #if defined(ENETUNREACH)
    CONSTANT(ENETUNREACH)
    #endif
    #if defined(ENFILE)
    CONSTANT(ENFILE)
    #endif
    #if defined(ENOBUFS)
    CONSTANT(ENOBUFS)
    #endif
    #if defined(ENODATA)
    CONSTANT(ENODATA)
    #endif
    #if defined(ENODEV)
    CONSTANT(ENODEV)
    #endif
    #if defined(ENOENT)
    CONSTANT(ENOENT)
    #endif
    #if defined(ENOEXEC)
    CONSTANT(ENOEXEC)
    #endif
    #if defined(ENOLCK)
    CONSTANT(ENOLCK)
    #endif
    #if defined(ENOLINK)
    CONSTANT(ENOLINK)
    #endif
    #if defined(ENOMEM)
    CONSTANT(ENOMEM)
    #endif
    #if defined(ENOMSG)
    CONSTANT(ENOMSG)
    #endif
    #if defined(ENOPROTOOPT)
    CONSTANT(ENOPROTOOPT)
    #endif
    #if defined(ENOSPC)
    CONSTANT(ENOSPC)
    #endif
    #if defined(ENOSR)
    CONSTANT(ENOSR)
    #endif
    #if defined(ENOSTR)
    CONSTANT(ENOSTR)
    #endif
    #if defined(ENOSYS)
    CONSTANT(ENOSYS)
    #endif
    #if defined(ENOTCONN)
    CONSTANT(ENOTCONN)
    #endif
    #if defined(ENOTDIR)
    CONSTANT(ENOTDIR)
    #endif
    #if defined(ENOTEMPTY)
    CONSTANT(ENOTEMPTY)
    #endif
    #if defined(ENOTSOCK)
    CONSTANT(ENOTSOCK)
    #endif
    #if defined(ENOTSUP)
    CONSTANT(ENOTSUP)
    #endif
    #if defined(ENOTTY)
    CONSTANT(ENOTTY)
    #endif
    #if defined(ENXIO)
    CONSTANT(ENXIO)
    #endif
    #if defined(EOPNOTSUPP)
    CONSTANT(EOPNOTSUPP)
    #endif
    #if defined(EOVERFLOW)
    CONSTANT(EOVERFLOW)
    #endif
    #if defined(EPERM)
    CONSTANT(EPERM)
    #endif
    #if defined(EPIPE)
    CONSTANT(EPIPE)
    #endif
    #if defined(EPROTO)
    CONSTANT(EPROTO)
    #endif
    #if defined(EPROTONOSUPPORT)
    CONSTANT(EPROTONOSUPPORT)
    #endif
    #if defined(EPROTOTYPE)
    CONSTANT(EPROTOTYPE)
    #endif
    #if defined(ERANGE)
    CONSTANT(ERANGE)
    #endif
    #if defined(EROFS)
    CONSTANT(EROFS)
    #endif
    #if defined(ESPIPE)
    CONSTANT(ESPIPE)
    #endif
    #if defined(ESRCH)
    CONSTANT(ESRCH)
    #endif
    #if defined(ESTALE)
    CONSTANT(ESTALE)
    #endif
    #if defined(ETIME)
    CONSTANT(ETIME)
    #endif
    #if defined(ETIMEDOUT)
    CONSTANT(ETIMEDOUT)
    #endif
    #if defined(ETXTBSY)
    CONSTANT(ETXTBSY)
    #endif
    #if defined(EWOULDBLOCK)
    CONSTANT(EWOULDBLOCK)
    #endif
    #if defined(EXDEV)
    CONSTANT(EXDEV)
    #endif

    #if defined(SIGHUP)
    CONSTANT(SIGHUP)
    #endif
    #if defined(SIGINT)
    CONSTANT(SIGINT)
    #endif
    #if defined(SIGQUIT)
    CONSTANT(SIGQUIT)
    #endif
    #if defined(SIGILL)
    CONSTANT(SIGILL)
    #endif
    #if defined(SIGTRAP)
    CONSTANT(SIGTRAP)
    #endif
    #if defined(SIGABRT)
    CONSTANT(SIGABRT)
    #endif
    #if defined(SIGIOT)
    CONSTANT(SIGIOT)
    #endif
    #if defined(SIGBUS)
    CONSTANT(SIGBUS)
    #endif
    #if defined(SIGFPE)
    CONSTANT(SIGFPE)
    #endif
    #if defined(SIGKILL)
    CONSTANT(SIGKILL)
    #endif
    #if defined(SIGUSR1)
    CONSTANT(SIGUSR1)
    #endif
    #if defined(SIGSEGV)
    CONSTANT(SIGSEGV)
    #endif
    #if defined(SIGUSR2)
    CONSTANT(SIGUSR2)
    #endif
    #if defined(SIGPIPE)
    CONSTANT(SIGPIPE)
    #endif
    #if defined(SIGALRM)
    CONSTANT(SIGALRM)
    #endif
    #if defined(SIGTERM)
    CONSTANT(SIGTERM)
    #endif
    #if defined(SIGCHLD)
    CONSTANT(SIGCHLD)
    #endif
    #if defined(SIGCONT)
    CONSTANT(SIGCONT)
    #endif
    #if defined(SIGSTOP)
    CONSTANT(SIGSTOP)
    #endif
    #if defined(SIGTSTP)
    CONSTANT(SIGTSTP)
    #endif
    #if defined(SIGTTIN)
    CONSTANT(SIGTTIN)
    #endif
    #if defined(SIGTTOU)
    CONSTANT(SIGTTOU)
    #endif
    #if defined(SIGURG)
    CONSTANT(SIGURG)
    #endif
    #if defined(SIGXCPU)
    CONSTANT(SIGXCPU)
    #endif
    #if defined(SIGXFSZ)
    CONSTANT(SIGXFSZ)
    #endif
    #if defined(SIGVTALRM)
    CONSTANT(SIGVTALRM)
    #endif
    #if defined(SIGPROF)
    CONSTANT(SIGPROF)
    #endif
    #if defined(SIGWINCH)
    CONSTANT(SIGWINCH)
    #endif
    #if defined(SIGIO)
    CONSTANT(SIGIO)
    #endif
    #if defined(SIGINFO)
    CONSTANT(SIGINFO)
    #endif
    #if defined(SIGSYS)
    CONSTANT(SIGSYS)
    #endif
  };
  #undef CONSTANT

  const Map<String, int32_t>& constants () {
    return CONSTANTS;
  }
}
