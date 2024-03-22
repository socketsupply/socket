import { errno as constants } from './os/constants.js'

/**
 * @typedef {import('./os/constants.js').errno} errno
 */

export const E2BIG = constants.E2BIG
export const EACCES = constants.EACCES
export const EADDRINUSE = constants.EADDRINUSE
export const EADDRNOTAVAIL = constants.EADDRNOTAVAIL
export const EAFNOSUPPORT = constants.EAFNOSUPPORT
export const EAGAIN = constants.EAGAIN
export const EALREADY = constants.EALREADY
export const EBADF = constants.EBADF
export const EBADMSG = constants.EBADMSG
export const EBUSY = constants.EBUSY
export const ECANCELED = constants.ECANCELED
export const ECHILD = constants.ECHILD
export const ECONNABORTED = constants.ECONNABORTED
export const ECONNREFUSED = constants.ECONNREFUSED
export const ECONNRESET = constants.ECONNRESET
export const EDEADLK = constants.EDEADLK
export const EDESTADDRREQ = constants.EDESTADDRREQ
export const EDOM = constants.EDOM
export const EDQUOT = constants.EDQUOT
export const EEXIST = constants.EEXIST
export const EFAULT = constants.EFAULT
export const EFBIG = constants.EFBIG
export const EHOSTUNREACH = constants.EHOSTUNREACH
export const EIDRM = constants.EIDRM
export const EILSEQ = constants.EILSEQ
export const EINPROGRESS = constants.EINPROGRESS
export const EINTR = constants.EINTR
export const EINVAL = constants.EINVAL
export const EIO = constants.EIO
export const EISCONN = constants.EISCONN
export const EISDIR = constants.EISDIR
export const ELOOP = constants.ELOOP
export const EMFILE = constants.EMFILE
export const EMLINK = constants.EMLINK
export const EMSGSIZE = constants.EMSGSIZE
export const EMULTIHOP = constants.EMULTIHOP
export const ENAMETOOLONG = constants.ENAMETOOLONG
export const ENETDOWN = constants.ENETDOWN
export const ENETRESET = constants.ENETRESET
export const ENETUNREACH = constants.ENETUNREACH
export const ENFILE = constants.ENFILE
export const ENOBUFS = constants.ENOBUFS
export const ENODATA = constants.ENODATA
export const ENODEV = constants.ENODEV
export const ENOENT = constants.ENOENT
export const ENOEXEC = constants.ENOEXEC
export const ENOLCK = constants.ENOLCK
export const ENOLINK = constants.ENOLINK
export const ENOMEM = constants.ENOMEM
export const ENOMSG = constants.ENOMSG
export const ENOPROTOOPT = constants.ENOPROTOOPT
export const ENOSPC = constants.ENOSPC
export const ENOSR = constants.ENOSR
export const ENOSTR = constants.ENOSTR
export const ENOSYS = constants.ENOSYS
export const ENOTCONN = constants.ENOTCONN
export const ENOTDIR = constants.ENOTDIR
export const ENOTEMPTY = constants.ENOTEMPTY
export const ENOTSOCK = constants.ENOTSOCK
export const ENOTSUP = constants.ENOTSUP
export const ENOTTY = constants.ENOTTY
export const ENXIO = constants.ENXIO
export const EOPNOTSUPP = constants.EOPNOTSUPP
export const EOVERFLOW = constants.EOVERFLOW
export const EPERM = constants.EPERM
export const EPIPE = constants.EPIPE
export const EPROTO = constants.EPROTO
export const EPROTONOSUPPORT = constants.EPROTONOSUPPORT
export const EPROTOTYPE = constants.EPROTOTYPE
export const ERANGE = constants.ERANGE
export const EROFS = constants.EROFS
export const ESPIPE = constants.ESPIPE
export const ESRCH = constants.ESRCH
export const ESTALE = constants.ESTALE
export const ETIME = constants.ETIME
export const ETIMEDOUT = constants.ETIMEDOUT
export const ETXTBSY = constants.ETXTBSY
export const EWOULDBLOCK = constants.EWOULDBLOCK
export const EXDEV = constants.EXDEV

export const strings = Object.assign(Object.create(null), {
  [E2BIG]: 'Arg list too long',
  [EACCES]: 'Permission denied',
  [EADDRINUSE]: 'Address already in use',
  [EADDRNOTAVAIL]: 'Cannot assign requested address',
  [EAFNOSUPPORT]: 'Address family not supported by protocol family',
  [EAGAIN]: 'Resource temporarily unavailabl',
  [EALREADY]: 'Operation already in progress',
  [EBADF]: 'Bad file descriptor',
  [EBADMSG]: 'Bad message',
  [EBUSY]: 'Resource busy',
  [ECANCELED]: 'Operation canceled',
  [ECHILD]: 'No child processes',
  [ECONNABORTED]: 'Software caused connection abort',
  [ECONNREFUSED]: 'Connection refused',
  [ECONNRESET]: 'Connection reset by peer',
  [EDEADLK]: 'Resource deadlock avoided',
  [EDESTADDRREQ]: 'Destination address required',
  [EDOM]: 'Numerical argument out of domain',
  [EDQUOT]: 'Disc quota exceeded',
  [EEXIST]: 'File exists',
  [EFAULT]: 'Bad address',
  [EFBIG]: 'File too large',
  [EHOSTUNREACH]: 'No route to host',
  [EIDRM]: 'Identifier removed',
  [EILSEQ]: 'Illegal byte sequence',
  [EINPROGRESS]: 'Operation now in progress',
  [EINTR]: 'Interrupted function call',
  [EINVAL]: 'Invalid argument',
  [EIO]: 'Input/output error',
  [EISCONN]: 'Socket is already connected',
  [EISDIR]: 'Is a directory',
  [ELOOP]: 'Too many levels of symbolic links',
  [EMFILE]: 'Too many open files',
  [EMLINK]: 'Too many links',
  [EMSGSIZE]: 'Message too long',
  [EMULTIHOP]: '',
  [ENAMETOOLONG]: 'File name too long',
  [ENETDOWN]: 'Network is down',
  [ENETRESET]: 'Network dropped connection on reset',
  [ENETUNREACH]: 'Network is unreachable',
  [ENFILE]: 'Too many open files in system',
  [ENOBUFS]: 'No buffer space available',
  [ENODATA]: 'No message available',
  [ENODEV]: 'Operation not supported by device',
  [ENOENT]: 'No such file or directory',
  [ENOEXEC]: 'Exec format error',
  [ENOLCK]: 'No locks available',
  [ENOLINK]: '',
  [ENOMEM]: 'Cannot allocate memory',
  [ENOMSG]: 'No message of desired type',
  [ENOPROTOOPT]: 'Protocol not available',
  [ENOSPC]: 'Device out of space',
  [ENOSR]: 'No STREAM resources',
  [ENOSTR]: 'Not a STREAM',
  [ENOSYS]: 'Function not implemented',
  [ENOTCONN]: 'Socket is not connected',
  [ENOTDIR]: 'Not a directory',
  [ENOTEMPTY]: 'Directory not empty',
  [ENOTSOCK]: 'Socket operation on non-socket',
  [ENOTSUP]: 'Not supported',
  [ENOTTY]: 'Inappropriate ioctl for device',
  [ENXIO]: 'No such device or address',
  [EOPNOTSUPP]: 'Operation not supported on socket',
  [EOVERFLOW]: 'Value too large to be stored in data type',
  [EPERM]: 'Operation not permitted',
  [EPIPE]: 'Broken pipe',
  [EPROTO]: 'Protocol error',
  [EPROTONOSUPPORT]: 'Protocol not supported',
  [EPROTOTYPE]: 'Protocol wrong type for socket',
  [ERANGE]: 'Numerical result out of range',
  [EROFS]: 'Read-only file system',
  [ESPIPE]: 'Illegal seek',
  [ESRCH]: 'No such process',
  [ESTALE]: 'Stale NFS file handle',
  [ETIME]: 'STREAM ioctl() timeout',
  [ETIMEDOUT]: 'Operation timed out',
  [ETXTBSY]: 'Text file busy',
  [EWOULDBLOCK]: 'Operation would block',
  [EXDEV]: 'Improper link'
})

/**
 * Converts an `errno` code to its corresponding string message.
 * @param {import('./os/constants.js').errno} {code}
 * @return {string}
 */
export function toString (code) {
  code = Math.abs(code)
  return strings[code] ?? ''
}

/**
 * Gets the code for a given 'errno' name.
 * @param {string|number} name
 * @return {errno}
 */
export function getCode (name) {
  if (typeof name !== 'string') {
    name = name.toString()
  }

  name = name.toUpperCase()
  for (const key in constants) {
    if (name === key) {
      return constants[key]
    }
  }

  return 0
}

/**
 * Gets the name for a given 'errno' code
 * @return {string}
 * @param {string|number} code
 */
export function getName (code) {
  code = getCode(code)
  for (const key in constants) {
    const value = constants[key]
    if (value === code) {
      return key
    }
  }

  return ''
}

/**
 * Gets the message for a 'errno' code.
 * @param {number|string} code
 * @return {string}
 */
export function getMessage (code) {
  if (typeof code === 'string') {
    code = getCode(code)
  }

  code = Math.abs(code)
  return toString(code)
}

export { constants }
export default {
  constants,
  strings,
  toString,
  getCode,
  getMessage
}
