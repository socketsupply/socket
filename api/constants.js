import fs from './fs/constants.js'
import window from './window/constants.js'
import os from './os/constants.js'
export * from './fs/constants.js'
export * from './window/constants.js'

export const E2BIG = os.errno.E2BIG
export const EACCES = os.errno.EACCES
export const EADDRINUSE = os.errno.EADDRINUSE
export const EADDRNOTAVAIL = os.errno.EADDRNOTAVAIL
export const EAFNOSUPPORT = os.errno.EAFNOSUPPORT
export const EAGAIN = os.errno.EAGAIN
export const EALREADY = os.errno.EALREADY
export const EBADF = os.errno.EBADF
export const EBADMSG = os.errno.EBADMSG
export const EBUSY = os.errno.EBUSY
export const ECANCELED = os.errno.ECANCELED
export const ECHILD = os.errno.ECHILD
export const ECONNABORTED = os.errno.ECONNABORTED
export const ECONNREFUSED = os.errno.ECONNREFUSED
export const ECONNRESET = os.errno.ECONNRESET
export const EDEADLK = os.errno.EDEADLK
export const EDESTADDRREQ = os.errno.EDESTADDRREQ
export const EDOM = os.errno.EDOM
export const EDQUOT = os.errno.EDQUOT
export const EEXIST = os.errno.EEXIST
export const EFAULT = os.errno.EFAULT
export const EFBIG = os.errno.EFBIG
export const EHOSTUNREACH = os.errno.EHOSTUNREACH
export const EIDRM = os.errno.EIDRM
export const EILSEQ = os.errno.EILSEQ
export const EINPROGRESS = os.errno.EINPROGRESS
export const EINTR = os.errno.EINTR
export const EINVAL = os.errno.EINVAL
export const EIO = os.errno.EIO
export const EISCONN = os.errno.EISCONN
export const EISDIR = os.errno.EISDIR
export const ELOOP = os.errno.ELOOP
export const EMFILE = os.errno.EMFILE
export const EMLINK = os.errno.EMLINK
export const EMSGSIZE = os.errno.EMSGSIZE
export const EMULTIHOP = os.errno.EMULTIHOP
export const ENAMETOOLONG = os.errno.ENAMETOOLONG
export const ENETDOWN = os.errno.ENETDOWN
export const ENETRESET = os.errno.ENETRESET
export const ENETUNREACH = os.errno.ENETUNREACH
export const ENFILE = os.errno.ENFILE
export const ENOBUFS = os.errno.ENOBUFS
export const ENODATA = os.errno.ENODATA
export const ENODEV = os.errno.ENODEV
export const ENOENT = os.errno.ENOENT
export const ENOEXEC = os.errno.ENOEXEC
export const ENOLCK = os.errno.ENOLCK
export const ENOLINK = os.errno.ENOLINK
export const ENOMEM = os.errno.ENOMEM
export const ENOMSG = os.errno.ENOMSG
export const ENOPROTOOPT = os.errno.ENOPROTOOPT
export const ENOSPC = os.errno.ENOSPC
export const ENOSR = os.errno.ENOSR
export const ENOSTR = os.errno.ENOSTR
export const ENOSYS = os.errno.ENOSYS
export const ENOTCONN = os.errno.ENOTCONN
export const ENOTDIR = os.errno.ENOTDIR
export const ENOTEMPTY = os.errno.ENOTEMPTY
export const ENOTSOCK = os.errno.ENOTSOCK
export const ENOTSUP = os.errno.ENOTSUP
export const ENOTTY = os.errno.ENOTTY
export const ENXIO = os.errno.ENXIO
export const EOPNOTSUPP = os.errno.EOPNOTSUPP
export const EOVERFLOW = os.errno.EOVERFLOW
export const EPERM = os.errno.EPERM
export const EPIPE = os.errno.EPIPE
export const EPROTO = os.errno.EPROTO
export const EPROTONOSUPPORT = os.errno.EPROTONOSUPPORT
export const EPROTOTYPE = os.errno.EPROTOTYPE
export const ERANGE = os.errno.ERANGE
export const EROFS = os.errno.EROFS
export const ESPIPE = os.errno.ESPIPE
export const ESRCH = os.errno.ESRCH
export const ESTALE = os.errno.ESTALE
export const ETIME = os.errno.ETIME
export const ETIMEDOUT = os.errno.ETIMEDOUT
export const ETXTBSY = os.errno.ETXTBSY
export const EWOULDBLOCK = os.errno.EWOULDBLOCK
export const EXDEV = os.errno.EXDEV

export const SIGHUP = os.signal.SIGHUP
export const SIGINT = os.signal.SIGINT
export const SIGQUIT = os.signal.SIGQUIT
export const SIGILL = os.signal.SIGILL
export const SIGTRAP = os.signal.SIGTRAP
export const SIGABRT = os.signal.SIGABRT
export const SIGIOT = os.signal.SIGIOT
export const SIGBUS = os.signal.SIGBUS
export const SIGFPE = os.signal.SIGFPE
export const SIGKILL = os.signal.SIGKILL
export const SIGUSR1 = os.signal.SIGUSR1
export const SIGSEGV = os.signal.SIGSEGV
export const SIGUSR2 = os.signal.SIGUSR2
export const SIGPIPE = os.signal.SIGPIPE
export const SIGALRM = os.signal.SIGALRM
export const SIGTERM = os.signal.SIGTERM
export const SIGCHLD = os.signal.SIGCHLD
export const SIGCONT = os.signal.SIGCONT
export const SIGSTOP = os.signal.SIGSTOP
export const SIGTSTP = os.signal.SIGTSTP
export const SIGTTIN = os.signal.SIGTTIN
export const SIGTTOU = os.signal.SIGTTOU
export const SIGURG = os.signal.SIGURG
export const SIGXCPU = os.signal.SIGXCPU
export const SIGXFSZ = os.signal.SIGXFSZ
export const SIGVTALRM = os.signal.SIGVTALRM
export const SIGPROF = os.signal.SIGPROF
export const SIGWINCH = os.signal.SIGWINCH
export const SIGIO = os.signal.SIGIO
export const SIGINFO = os.signal.SIGINFO
export const SIGSYS = os.signal.SIGSYS

export default {
  ...fs,
  ...window,
  ...os.errno,
  ...os.signal
}
