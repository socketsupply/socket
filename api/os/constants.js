import { sendSync } from '../ipc.js'

const constants = sendSync('os.constants', {}, { cache: true })?.data || {}

/**
 * @typedef {number} errno
 * @typedef {number} signal
 */

/**
 * A container for all known "errno" constant values.
 * Unsupported values have a default value of `0`.
 */
export const errno = Object.assign(Object.create(null), {
  /**
   * "Arg list too long"
   * The number of bytes used for the argument and environment list of the
   * new process exceeded the limit NCARGS (specified in ⟨sys/param.h⟩).
   * @type {errno}
   */
  E2BIG: constants.E2BIG ?? 0,

  /**
   * "Permission denied"
   * An attempt was made to access a file in a way forbidden by
   * its file access permissions.
   * @type {errno}
   */
  EACCES: constants.EACCES ?? 0,

  /**
   * "Address already in use"
   * Only one usage of each address is normally permitted.
   * @type {errno}
   */
  EADDRINUSE: constants.EADDRINUSE ?? 0,

  /**
   * "Cannot assign requested address"
   * Normally results from an attempt to create a socket with an
   * address not on this machine.
   * @type {errno}
   */
  EADDRNOTAVAIL: constants.EADDRNOTAVAIL ?? 0,

  /**
   * "Address family not supported by protocol family"
   * An address incompatible with the requested protocol was used.
   * For example, you shouldn't necessarily expect to be able to use
   * NS addresses with ARPA Internet protocols.
   * @type {errno}
   */
  EAFNOSUPPORT: constants.EAFNOSUPPORT ?? 0,

  /**
   * "Resource temporarily unavailable"
   * This is a temporary condition and later calls to the
   * same routine may complete normally.
   * @type {errno}
   */
  EAGAIN: constants.EAGAIN ?? 0,

  /**
   * "Operation already in progress"
   * An operation was attempted on a non-blocking object that
   * already had an operation in progress.
   * @type {errno}
   */
  EALREADY: constants.EALREADY ?? 0,

  /**
   * "Bad file descriptor"
   * A file descriptor argument was out of range, referred to no open file,
   * or a read (write) request was made to a file that was only open
   * for writing (reading).
   * @type {errno}
   */
  EBADF: constants.EBADF ?? 0,

  /**
   * "Bad message"
   * The message to be received is inapprorpiate for the operation
   * being attempted.
   * @type {errno}
   */
  EBADMSG: constants.EBADMSG ?? 0,

  /**
   * "Resource busy"
   * An attempt to use a system resource which was in use at the time
   * in a manner which would have conflicted with the request.
   * @type {errno}
   */
  EBUSY: constants.EBUSY ?? 0,

  /**
   * "Operation canceled"
   * The scheduled operation was canceled.
   * @type {errno}
   */
  ECANCELED: constants.ECANCELED ?? 0,

  /**
   * "No child processes"
   * A wait or waitpid function was executed by a process that had no existing
   * or unwaited-for child processes.
   * @type {errno}
   */
  ECHILD: constants.ECHILD ?? 0,

  /**
   * "Software caused connection abort"
   * A connection abort was caused internal to your host machine.
   * @type {errno}
   */
  ECONNABORTED: constants.ECONNABORTED ?? 0,

  /**
   * "Connection refused"
   * No connection could be made because the target machine actively refused it.
   * This usually results from trying to connect to a service that is inactive
   * on the foreign host.
   * @type {errno}
   */
  ECONNREFUSED: constants.ECONNREFUSED ?? 0,

  /**
   * "Connection reset by peer"
   * A connection was forcibly closed by a peer.
   * This normally results from a loss of the connection on the remote socket
   * due to a timeout or a reboot.
   * @type {errno}
   */
  ECONNRESET: constants.ECONNRESET ?? 0,

  /**
   * "Resource deadlock avoided"
   * An attempt was made to lock a system resource that would have resulted in
   * a deadlock situation.
   * @type {errno}
   */
  EDEADLK: constants.EDEADLK ?? 0,

  /**
   * "Destination address required"
   * A required address was omitted from an operation on a socket.
   * @type {errno}
   */
  EDESTADDRREQ: constants.EDESTADDRREQ ?? 0,

  /**
   * "Numerical argument out of domain"
   * A numerical input argument was outside the defined domain of the
   * mathematical function.
   * @type {errno}
   */
  EDOM: constants.EDOM ?? 0,

  /**
   * "Disc quota exceeded"
   * A write to an ordinary file, the creation of a directory or symbolic link,
   * or the creation of a directory entry failed because the user's quota of
   * disk blocks was exhausted, or the allocation of an inode for a newly
   * created file failed because the user's quota of inodes was exhausted.
   * @type {errno}
   */
  EDQUOT: constants.EDQUOT ?? 0,

  /**
   * "File exists"
   * An existing file was mentioned in an inappropriate context, for instance,
   * as the new link name in a link function.
   * @type {errno}
   */
  EEXIST: constants.EEXIST ?? 0,

  /**
   * "Bad address"
   * The system detected an invalid address in attempting to use an
   * argument of a call.
   * @type {errno}
   */
  EFAULT: constants.EFAULT ?? 0,

  /**
   * "File too large"
   * The size of a file exceeded the maximum.
   * @type {errno}
   */
  EFBIG: constants.EFBIG ?? 0,

  /**
   * "No route to host"
   * A socket operation was attempted to an unreachable host.
   * @type {errno}
   */
  EHOSTUNREACH: constants.EHOSTUNREACH ?? 0,

  /**
   * "Identifier removed"
   * An IPC identifier was removed while the current process was waiting on it.
   * @type {errno}
   */
  EIDRM: constants.EIDRM ?? 0,

  /**
   * "Illegal byte sequence"
   * While decoding a multibyte character the function came along an invalid
   * or an incomplete sequence of bytes or the given wide character is invalid.
   * @type {errno}
   */
  EILSEQ: constants.EILSEQ ?? 0,

  /**
   * "Operation now in progress"
   * An operation that takes a long time to complete.
   * @type {errno}
   */
  EINPROGRESS: constants.EINPROGRESS ?? 0,

  /**
   * "Interrupted function call"
   * An asynchronous signal (such as SIGINT or SIGQUIT) was caught by the
   * process during the execution of an interruptible function. If the signal
   * handler performs a normal return, the interrupted function call will seem
   * to have returned the error condition.
   * @type {errno}
   */
  EINTR: constants.EINTR ?? 0,

  /**
   * "Invalid argument"
   * Some invalid argument was supplied.
   * (For example, specifying an undefined signal to a signal or kill function).
   * @type {errno}
   */
  EINVAL: constants.EINVAL ?? 0,

  /**
   * "Input/output error"
   * Some physical input or output error occurred.
   * This error will not be reported until a subsequent operation on the same
   * file descriptor and may be lost (over written) by any subsequent errors.
   * @type {errno}
   */
  EIO: constants.EIO ?? 0,

  /**
   * "Socket is already connected"
   * A connect or connectx request was made on an already connected socket;
   * or, a sendto or sendmsg request on a connected socket specified a
   * destination when already connected.
   * @type {errno}
   */
  EISCONN: constants.EISCONN ?? 0,

  /**
   * "Is a directory"
   * An attempt was made to open a directory with write mode specified.
   * @type {errno}
   */
  EISDIR: constants.EISDIR ?? 0,

  /**
   * "Too many levels of symbolic links"
   * A path name lookup involved more than 8 symbolic links.
   * @type {errno}
   */
  ELOOP: constants.ELOOP ?? 0,

  /**
   * "Too many open files"
   * @type {errno}
   */
  EMFILE: constants.EMFILE ?? 0,

  /**
   * "Too many links"
   * Maximum allowable hard links to a single file
   * has been exceeded (limit of 32767 hard links per file).
   * @type {errno}
   */
  EMLINK: constants.EMLINK ?? 0,

  /**
   * "Message too long"
   * A message sent on a socket was larger than the internal message
   * buffer or some other network limit.
   * @type {errno}
   */
  EMSGSIZE: constants.EMSGSIZE ?? 0,

  /**
   * "Reserved"
   * This error is reserved for future use.
   * @type {errno}
   */
  EMULTIHOP: constants.EMULTIHOP ?? 0,

  /**
   * "File name too long"
   * A component of a path name exceeded 255 (MAXNAMELEN) characters,
   * or an entire path name exceeded 1023 (MAXPATHLEN-1) characters.
   * @type {errno}
   */
  ENAMETOOLONG: constants.ENAMETOOLONG ?? 0,

  /**
   * "Network is down"
   * A socket operation encountered a dead network.
   * @type {errno}
   */
  ENETDOWN: constants.ENETDOWN ?? 0,

  /**
   * "Network dropped connection on reset"
   * The host you were connected to crashed and rebooted.
   * @type {errno}
   */
  ENETRESET: constants.ENETRESET ?? 0,

  /**
   * "Network is unreachable"
   * A socket operation was attempted to an unreachable network.
   * @type {errno}
   */
  ENETUNREACH: constants.ENETUNREACH ?? 0,

  /**
   * "Too many open files in system"
   * Maximum number of file descriptors allowable on the system has been reached
   * and a requests for an open cannot be satisfied until at least one has
   * been closed.
   * @type {errno}
   */
  ENFILE: constants.ENFILE ?? 0,

  /**
   * "No buffer space available"
   * An operation on a socket or pipe was not performed because the system
   * lacked sufficient buffer space or because a queue was full.
   * @type {errno}
   */
  ENOBUFS: constants.ENOBUFS ?? 0,

  /**
   * "No message available"
   * No message was available to be received by the requested operation.
   * @type {errno}
   */
  ENODATA: constants.ENODATA ?? 0,

  /**
   * "Operation not supported by device"
   * An attempt was made to apply an inappropriate function to a device,
   * for example, trying to read a write-only device such as a printer.
   * @type {errno}
   */
  ENODEV: constants.ENODEV ?? 0,

  /**
   * "No such file or directory"
   * A component of a specified pathname did not exist,
   * or the pathname was an empty string.
   * @type {errno}
   */
  ENOENT: constants.ENOENT ?? 0,

  /**
   * "Exec format error"
   * A request was made to execute a file that,
   * although it has the appropriate permissions,
   * was not in the format required for an executable file.
   * @type {errno}
   */
  ENOEXEC: constants.ENOEXEC ?? 0,

  /**
   * "No locks available"
   * A system-imposed limit on the number of simultaneous
   * file locks was reached.
   * @type {errno}
   */
  ENOLCK: constants.ENOLCK ?? 0,

  /**
   * "Reserved"
   * This error is reserved for future use.
   * @type {errno}
   */
  ENOLINK: constants.ENOLINK ?? 0,

  /**
   * "Cannot allocate memory"
   * The new process image required more memory than was allowed by the hardware
   * or by system-imposed memory management constraints.A lack of swap space is
   * normally temporary; however, a lack of core is not.
   * Soft limits may be increased to their corresponding hard limits.
   * @type {errno}
   */
  ENOMEM: constants.ENOMEM ?? 0,

  /**
   * "No message of desired type"
   * An IPC message queue does not contain a message of the desired type,
   * or a message catalog does not contain the requested message.
   * @type {errno}
   */
  ENOMSG: constants.ENOMSG ?? 0,

  /**
   * "Protocol not available"
   * A bad option or level was specified in a `getsockopt(2)` or
   * `setsockopt(2)` call.
   * @type {errno}
   */
  ENOPROTOOPT: constants.ENOPROTOOPT ?? 0,

  /**
   * "Device out of space"
   * A write to an ordinary file, the creation of a directory or symbolic link,
   * or the creation of a directory entry failed because no more disk blocks
   * were available on the file system, or the allocation of an inode for a
   * newly created file failed because no more inodes were available on
   * the file system.
   * @type {errno}
   */
  ENOSPC: constants.ENOSPC ?? 0,

  /**
   * "No STREAM resources"
   * This error is reserved for future use.
   * @type {errno}
   */
  ENOSR: constants.ENOSR ?? 0,

  /**
   * "Not a STREAM"
   * This error is reserved for future use.
   * @type {errno}
   */
  ENOSTR: constants.ENOSTR ?? 0,

  /**
   * "Function not implemented"
   * Attempted a system call that is not available on this system.
   * @type {errno}
   */
  ENOSYS: constants.ENOSYS ?? 0,

  /**
   * "Socket is not connected"
   * An request to send or receive data was disallowed because the socket was
   * not connected and (when sending on a datagram socket) no address was
   * supplied.
   * @type {errno}
   */
  ENOTCONN: constants.ENOTCONN ?? 0,

  /**
   * "Not a directory"
   * A component of the specified pathname existed, but it was not a directory,
   * when a directory was expected.
   * @type {errno}
   */
  ENOTDIR: constants.ENOTDIR ?? 0,

  /**
   * "Directory not empty"
   * A directory with entries other than ‘.’ and ‘..’ was supplied to a remove
   * directory or rename call.
   * @type {errno}
   */
  ENOTEMPTY: constants.ENOTEMPTY ?? 0,

  /**
   * "Socket operation on non-socket"
   * Self-explanatory.
   * @type {errno}
   */
  ENOTSOCK: constants.ENOTSOCK ?? 0,

  /**
   * "Not supported"
   * The attempted operation is not supported for the type of object referenced.
   * @type {errno}
   */
  ENOTSUP: constants.ENOTSUP ?? 0,

  /**
   * "Inappropriate ioctl for device"
   * A control function (see `ioctl(2)`) was attempted for a file or special
   * device for which the operation was inappropriate.
   * @type {errno}
   */
  ENOTTY: constants.ENOTTY ?? 0,

  /**
   * "No such device or address"
   * Input or output on a special file referred to a device that did not exist,
   * or made a request beyond the limits of the device.
   * This error may also occur when, for example, a tape drive is not online or
   * no disk pack is loaded on a drive.
   * @type {errno}
   */
  ENXIO: constants.ENXIO ?? 0,

  /**
   * "Operation not supported on socket"
   * The attempted operation is not supported for the type of socket referenced;
   * for example, trying to accept a connection on a datagram socket.
   * @type {errno}
   */
  EOPNOTSUPP: constants.EOPNOTSUPP ?? 0,

  /**
   * "Value too large to be stored in data type"
   * A numerical result of the function was too large to be stored
   * in the caller provided space.
   * @type {errno}
   */
  EOVERFLOW: constants.EOVERFLOW ?? 0,

  /**
   * "Operation not permitted"
   * An attempt was made to perform an operation limited to processes with
   * appropriate privileges or to the owner of a file or other resources.
   * @type {errno}
   */
  EPERM: constants.EPERM ?? 0,

  /**
   * "Broken pipe"
   * A write on a pipe, socket or FIFO for which there is no process to
   * read the data.
   * @type {errno}
   */
  EPIPE: constants.EPIPE ?? 0,

  /**
   * "Protocol error"
   * Some protocol error occurred.
   * This error is device-specific, but is generally not related to a
   * hardware failure.
   * @type {errno}
   */
  EPROTO: constants.EPROTO ?? 0,

  /**
   * "Protocol not supported"
   * The protocol has not been configured into the system or
   * no implementation for it exists.
   * @type {errno}
   */
  EPROTONOSUPPORT: constants.EPROTONOSUPPORT ?? 0,

  /**
   * "Protocol wrong type for socket"
   * A protocol was specified that does not support the semantics of the socket
   * type requested. For example, you cannot use the ARPA Internet UDP protocol
   * with type SOCK_STREAM.
   * @type {errno}
   */
  EPROTOTYPE: constants.EPROTOTYPE ?? 0,

  /**
   * "Numerical result out of range"
   * A numerical result of the function was too large to fit in the available
   * space (perhaps exceeded precision).
   * @type {errno}
   */
  ERANGE: constants.ERANGE ?? 0,

  /**
   * "Read-only file system"
   * An attempt was made to modify a file or directory was made on a file
   * system that was read-only at the time.
   * @type {errno}
   */
  EROFS: constants.EROFS ?? 0,

  /**
   * "Illegal seek"
   * An lseek function was issued on a socket, pipe or FIFO.
   * @type {errno}
   */
  ESPIPE: constants.ESPIPE ?? 0,

  /**
   * "No such process"
   * No process could be found corresponding to that specified
   * by the given process ID.
   * @type {errno}
   */
  ESRCH: constants.ESRCH ?? 0,

  /**
   * "Stale NFS file handle"
   * An attempt was made to access an open file (on an NFS filesystem)
   * which is now unavailable as referenced by the file descriptor.
   * This may indicate the file was deleted on the NFS server or some other
   * catastrophic event occurred.
   * @type {errno}
   */
  ESTALE: constants.ESTALE ?? 0,

  /**
   * "STREAM ioctl() timeout"
   * This error is reserved for future use.
   * @type {errno}
   */
  ETIME: constants.ETIME ?? 0,

  /**
   * "Operation timed out"
   * A connect, connectx or send request failed because the connected party did
   * not properly respond after a period of time.
   * (The timeout period is dependent on the communication protocol.)
   * @type {errno}
   */
  ETIMEDOUT: constants.ETIMEDOUT ?? 0,

  /**
   * "Text file busy"
   * The new process was a pure procedure (shared text) file which was open for
   * writing by another process, or while the pure procedure file was being
   * executed an open call requested write access.
   * @type {errno}
   */
  ETXTBSY: constants.ETXTBSY ?? 0,

  /**
   * "Operation would block"
   * (may be same value as EAGAIN) (POSIX.1-2001).
   * @type {errno}
   */
  EWOULDBLOCK: constants.EWOULDBLOCK ?? constants.EAGAIN ?? 0,

  /**
   * "Improper link"
   * A hard link to a file on another file system was attempted.
   * @type {errno}
   */
  EXDEV: constants.EXDEV ?? 0
})

/**
 * A container for all known "signal" constant values.
 * Unsupported values have a default value of `0`.
 */
export const signal = Object.assign(Object.create(null), {
  /**
   * Terminal line hangup.
   * @type {signal}
   */
  SIGHUP: constants.SIGHUP ?? 0,

  /**
   * Interrupt program.
   * @type {signal}
   */
  SIGINT: constants.SIGINT ?? 0,

  /**
   * Quit program.
   * @type {signal}
   */
  SIGQUIT: constants.SIGQUIT ?? 0,

  /**
   * Illegal instruction.
   * @type {signal}
   */
  SIGILL: constants.SIGILL ?? 0,

  /**
   * Trace trap.
   * @type {signal}
   */
  SIGTRAP: constants.SIGTRAP ?? 0,

  /**
   * Abort program.
   * @type {signal}
   */
  SIGABRT: constants.SIGABRT ?? 0,

  /**
   * An alias to `SIGABRT`
   * @type {signal}
   */
  SIGIOT: constants.SIGIOT ?? constants.SIGABRT ?? 0,

  /**
   * Bus error.
   * @type {signal}
   */
  SIGBUS: constants.SIGBUS ?? 0,

  /**
   * Floating-point exception.
   * @type {signal}
   */
  SIGFPE: constants.SIGFPE ?? 0,

  /**
   * Kill program.
   * @type {signal}
   */
  SIGKILL: constants.SIGKILL ?? 0,

  /**
   * User defined signal 1.
   * @type {signal}
   */
  SIGUSR1: constants.SIGUSR1 ?? 0,

  /**
   * Segmentation violation.
   * @type {signal}
   */
  SIGSEGV: constants.SIGSEGV ?? 0,

  /**
   * User defined signal 2.
   * @type {signal}
   */
  SIGUSR2: constants.SIGUSR2 ?? 0,

  /**
   * Write on a pipe with no reader.
   * @type {signal}
   */
  SIGPIPE: constants.SIGPIPE ?? 0,

  /**
   * Real-time timer expired.
   * @type {signal}
   */
  SIGALRM: constants.SIGALRM ?? 0,

  /**
   * Software termination signal.
   * @type {signal}
   */
  SIGTERM: constants.SIGTERM ?? 0,

  /**
   * Child status has changed.
   * @type {signal}
   */
  SIGCHLD: constants.SIGCHLD ?? 0,

  /**
   * Continue after stop.
   * @type {signal}
   */
  SIGCONT: constants.SIGCONT ?? 0,

  /**
   * Stop signal (cannot be caught or ignored).
   * @type {signal}
   */
  SIGSTOP: constants.SIGSTOP ?? 0,

  /**
   * Stop signal generated from keyboard.
   * @type {signal}
   */
  SIGTSTP: constants.SIGTSTP ?? 0,

  /**
   * Background read attempted from control terminal.
   * @type {signal}
   */
  SIGTTIN: constants.SIGTTIN ?? 0,

  /**
   * Background write attempted to control terminal.
   * @type {signal}
   */
  SIGTTOU: constants.SIGTTOU ?? 0,

  /**
   * Urgent condition present on socket.
   * @type {signal}
   */
  SIGURG: constants.SIGURG ?? 0,

  /**
   * CPU time limit exceeded (see `setrlimit(2)`)
   * @type {signal}
   */
  SIGXCPU: constants.SIGXCPU ?? 0,

  /**
   * File size limit exceeded (see `setrlimit(2)`).
   * @type {signal}
   */
  SIGXFSZ: constants.SIGXFSZ ?? 0,

  /**
   * Virtual time alarm (see `setitimer(2)`).
   * @type {signal}
   */
  SIGVTALRM: constants.SIGVTALRM ?? 0,

  /**
   * Profiling timer alarm (see `setitimer(2)`).
   * @type {signal}
   */
  SIGPROF: constants.SIGPROF ?? 0,

  /**
   * Window size change.
   * @type {signal}
   */
  SIGWINCH: constants.SIGWINCH ?? 0,

  /**
   * I/O is possible on a descriptor (see `fcntl(2)`).
   * @type {signal}
   */
  SIGIO: constants.SIGIO ?? 0,

  /**
   * Status request from keyboard.
   * @type {signal}
   */
  SIGINFO: constants.SIGINFO ?? 0,

  /**
   * Non-existent system call invoked.
   * @type {signal}
   */
  SIGSYS: constants.SIGSYS ?? 0
})

export default {
  errno,
  signal
}
