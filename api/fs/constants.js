import { sendSync } from '../ipc.js'
import * as exports from './constants.js'

const constants = sendSync('fs.constants', {}, { cache: true })?.data || {}

/**
 * This flag can be used with uv_fs_copyfile() to return an error if the
 * destination already exists.
 */
export const COPYFILE_EXCL = 0x0001

/**
 * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
 * If copy-on-write is not supported, a fallback copy mechanism is used.
 */
export const COPYFILE_FICLONE = 0x0002

/**
 * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
 * If copy-on-write is not supported, an error is returned.
 */
export const COPYFILE_FICLONE_FORCE = 0x0004

export const UV_DIRENT_UNKNOWN = constants.UV_DIRENT_UNKNOWN || 0
export const UV_DIRENT_FILE = constants.UV_DIRENT_FILE || 1
export const UV_DIRENT_DIR = constants.UV_DIRENT_DIR || 2
export const UV_DIRENT_LINK = constants.UV_DIRENT_LINK || 3
export const UV_DIRENT_FIFO = constants.UV_DIRENT_FIFO || 4
export const UV_DIRENT_SOCKET = constants.UV_DIRENT_SOCKET || 5
export const UV_DIRENT_CHAR = constants.UV_DIRENT_CHAR || 6
export const UV_DIRENT_BLOCK = constants.UV_DIRENT_BLOCK || 7

export const O_RDONLY = constants.O_RDONLY || 0
export const O_WRONLY = constants.O_WRONLY || 1
export const O_RDWR = constants.O_RDWR || 2
export const O_APPEND = constants.O_APPEND || 8
export const O_ASYNC = constants.O_ASYNC || 0
export const O_CLOEXEC = constants.O_CLOEXEC || 0
export const O_CREAT = constants.O_CREAT || 0
export const O_DIRECT = constants.O_DIRECT || 0
export const O_DIRECTORY = constants.O_DIRECTORY || 0
export const O_DSYNC = constants.O_DSYNC || 0
export const O_EXCL = constants.O_EXCL || 0
export const O_LARGEFILE = constants.O_LARGEFILE || 0
export const O_NOATIME = constants.O_NOATIME || 0
export const O_NOCTTY = constants.O_NOCTTY || 0
export const O_NOFOLLOW = constants.O_NOFOLLOW || 0
export const O_NONBLOCK = constants.O_NONBLOCK || 0
export const O_NDELAY = constants.O_NDELAY || 0
export const O_PATH = constants.O_PATH || 0
export const O_SYNC = constants.O_SYNC || 0
export const O_TMPFILE = constants.O_TMPFILE || 0
export const O_TRUNC = constants.O_TRUNC || 0

export const S_IFMT = constants.S_IFMT || 0
export const S_IFREG = constants.S_IFREG || 0
export const S_IFDIR = constants.S_IFDIR || 0
export const S_IFCHR = constants.S_IFCHR || 0
export const S_IFBLK = constants.S_IFBLK || 0
export const S_IFIFO = constants.S_IFIFO || 0
export const S_IFLNK = constants.S_IFLNK || 0
export const S_IFSOCK = constants.S_IFSOCK || 0
export const S_IRWXU = constants.S_IRWXU || 0
export const S_IRUSR = constants.S_IRUSR || 0
export const S_IWUSR = constants.S_IWUSR || 0
export const S_IXUSR = constants.S_IXUSR || 0
export const S_IRWXG = constants.S_IRWXG || 0
export const S_IRGRP = constants.S_IRGRP || 0
export const S_IWGRP = constants.S_IWGRP || 0
export const S_IXGRP = constants.S_IXGRP || 0
export const S_IRWXO = constants.S_IRWXO || 0
export const S_IROTH = constants.S_IROTH || 0
export const S_IWOTH = constants.S_IWOTH || 0
export const S_IXOTH = constants.S_IXOTH || 0

export const F_OK = constants.F_OK || 0
export const R_OK = constants.R_OK || 4
export const W_OK = constants.W_OK || 2
export const X_OK = constants.X_OK || 1

export default exports
