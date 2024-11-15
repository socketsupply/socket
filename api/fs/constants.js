import { sendSync } from '../ipc.js'
import * as exports from './constants.js'

const constants = sendSync('fs.constants', {}, { cache: true })?.data || {}

/**
 * This flag can be used with uv_fs_copyfile() to return an error if the
 * destination already exists.
 * @type {number}
 */
export const COPYFILE_EXCL = 0x0001

/**
 * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
 * If copy-on-write is not supported, a fallback copy mechanism is used.
 * @type {number}
 */
export const COPYFILE_FICLONE = 0x0002

/**
 * This flag can be used with uv_fs_copyfile() to attempt to create a reflink.
 * If copy-on-write is not supported, an error is returned.
 * @type {number}
 */
export const COPYFILE_FICLONE_FORCE = 0x0004

/**
 * A constant representing a directory entry whose type is unknown.
 * It indicates that the type of the file or directory cannot be determined.
 * @type {number}
 */
export const UV_DIRENT_UNKNOWN = constants.UV_DIRENT_UNKNOWN || 0

/**
 * A constant representing a directory entry of type file.
 * It indicates that the entry is a regular file.
 * @type {number}
 */
export const UV_DIRENT_FILE = constants.UV_DIRENT_FILE || 1

/**
 * A constant epresenting a directory entry of type directory.
 * It indicates that the entry is a directory.
 * @type {number}
 */
export const UV_DIRENT_DIR = constants.UV_DIRENT_DIR || 2

/**
 * A constant representing a directory entry of type symbolic link.
 * @type {number}
 */
export const UV_DIRENT_LINK = constants.UV_DIRENT_LINK || 3

/**
 * A constant representing a directory entry of type FIFO (named pipe).
 * @type {number}
 */
export const UV_DIRENT_FIFO = constants.UV_DIRENT_FIFO || 4

/**
 * A constant representing a directory entry of type socket.
 * @type {number}
 */
export const UV_DIRENT_SOCKET = constants.UV_DIRENT_SOCKET || 5

/**
 * A constant representing a directory entry of type character device
 * @type {number}
 */
export const UV_DIRENT_CHAR = constants.UV_DIRENT_CHAR || 6

/**
 * A constant representing a directory entry of type block device.
 * @type {number}
 */
export const UV_DIRENT_BLOCK = constants.UV_DIRENT_BLOCK || 7

/**
 * A constant representing a symlink should target a directory.
 * @type {number}
 */
export const UV_FS_SYMLINK_DIR = constants.UV_FS_SYMLINK_DIR || 1

/**
 * A constant representing a symlink should be created as a Windows junction.
 * @type {number}
 */
export const UV_FS_SYMLINK_JUNCTION = constants.UV_FS_SYMLINK_JUNCTION || 2

/**
 * A constant representing an opened file for memory mapping on Windows systems.
 * @type {number}
 */
export const UV_FS_O_FILEMAP = constants.UV_FS_O_FILEMAP || 0

/**
 * Opens a file for read-only access.
 * @type {number}
 */
export const O_RDONLY = constants.O_RDONLY || 0

/**
 * Opens a file for write-only access.
 * @type {number}
 */
export const O_WRONLY = constants.O_WRONLY || 1

/**
 * Opens a file for both reading and writing.
 * @type {number}
 */
export const O_RDWR = constants.O_RDWR || 2

/**
 * Appends data to the file instead of overwriting.
 * @type {number}
 */
export const O_APPEND = constants.O_APPEND || 8

/**
 * Enables asynchronous I/O notifications.
 * @type {number}
 */
export const O_ASYNC = constants.O_ASYNC || 0

/**
 * Ensures file descriptors are closed on `exec()` calls.
 * @type {number}
 */
export const O_CLOEXEC = constants.O_CLOEXEC || 0

/**
 * Creates a new file if it does not exist.
 * @type {number}
 */
export const O_CREAT = constants.O_CREAT || 0

/**
 * Minimizes caching effects for file I/O.
 * @type {number}
 */
export const O_DIRECT = constants.O_DIRECT || 0

/**
 * Ensures the opened file is a directory.
 * @type {number}
 */
export const O_DIRECTORY = constants.O_DIRECTORY || 0

/**
 * Writes file data synchronously.
 * @type {number}
 */
export const O_DSYNC = constants.O_DSYNC || 0

/**
 * Fails the operation if the file already exists.
 * @type {number}
 */
export const O_EXCL = constants.O_EXCL || 0

/**
 * Enables handling of large files.
 * @type {number}
 */
export const O_LARGEFILE = constants.O_LARGEFILE || 0

/**
 * Prevents updating the file's last access time.
 * @type {number}
 */
export const O_NOATIME = constants.O_NOATIME || 0

/**
 * Prevents becoming the controlling terminal for the process.
 * @type {number}
 */
export const O_NOCTTY = constants.O_NOCTTY || 0

/**
 * Does not follow symbolic links.
 * @type {number}
 */
export const O_NOFOLLOW = constants.O_NOFOLLOW || 0

/**
 * Opens the file in non-blocking mode.
 * @type {number}
 */
export const O_NONBLOCK = constants.O_NONBLOCK || 0

/**
 * Alias for `O_NONBLOCK` on some systems.
 * @type {number}
 */
export const O_NDELAY = constants.O_NDELAY || 0

/**
 * Obtains a file descriptor for a file but does not open it.
 * @type {number}
 */
export const O_PATH = constants.O_PATH || 0

/**
 * Writes both file data and metadata synchronously.
 * @type {number}
 */
export const O_SYNC = constants.O_SYNC || 0

/**
 * Creates a temporary file that is not linked to a directory.
 * @type {number}
 */
export const O_TMPFILE = constants.O_TMPFILE || 0

/**
 * Truncates the file to zero length if it exists.
 * @type {number}
 */
export const O_TRUNC = constants.O_TRUNC || 0

/**
 * Bitmask for extracting the file type from a mode.
 * @type {number}
 */
export const S_IFMT = constants.S_IFMT || 0

/**
 * Indicates a regular file.
 * @type {number}
 */
export const S_IFREG = constants.S_IFREG || 0

/**
 * Indicates a directory.
 * @type {number}
 */
export const S_IFDIR = constants.S_IFDIR || 0

/**
 * Indicates a character device.
 * @type {number}
 */
export const S_IFCHR = constants.S_IFCHR || 0

/**
 * Indicates a block device.
 * @type {number}
 */
export const S_IFBLK = constants.S_IFBLK || 0

/**
 * Indicates a FIFO (named pipe).
 * @type {number}
 */
export const S_IFIFO = constants.S_IFIFO || 0

/**
 * Indicates a symbolic link.
 * @type {number}
 */
export const S_IFLNK = constants.S_IFLNK || 0

/**
 * Indicates a socket.
 * @type {number}
 */
export const S_IFSOCK = constants.S_IFSOCK || 0

/**
 * Grants read, write, and execute permissions for the file owner.
 * @type {number}
 */
export const S_IRWXU = constants.S_IRWXU || 0

/**
 * Grants read permission for the file owner.
 * @type {number}
 */
export const S_IRUSR = constants.S_IRUSR || 0

/**
 * Grants write permission for the file owner.
 * @type {number}
 */
export const S_IWUSR = constants.S_IWUSR || 0

/**
 * Grants execute permission for the file owner.
 * @type {number}
 */
export const S_IXUSR = constants.S_IXUSR || 0

/**
 * Grants read, write, and execute permissions for the group.
 * @type {number}
 */
export const S_IRWXG = constants.S_IRWXG || 0

/**
 * Grants read permission for the group.
 * @type {number}
 */
export const S_IRGRP = constants.S_IRGRP || 0

/**
 * Grants write permission for the group.
 * @type {number}
 */
export const S_IWGRP = constants.S_IWGRP || 0

/**
 * Grants execute permission for the group.
 * @type {number}
 */
export const S_IXGRP = constants.S_IXGRP || 0

/**
 * Grants read, write, and execute permissions for others.
 * @type {number}
 */
export const S_IRWXO = constants.S_IRWXO || 0

/**
 * Grants read permission for others.
 * @type {number}
 */
export const S_IROTH = constants.S_IROTH || 0

/**
 * Grants write permission for others.
 * @type {number}
 */
export const S_IWOTH = constants.S_IWOTH || 0

/**
 * Grants execute permission for others.
 * @type {number}
 */
export const S_IXOTH = constants.S_IXOTH || 0

/**
 * Checks for the existence of a file.
 * @type {number}
 */
export const F_OK = constants.F_OK || 0

/**
 * Checks for read permission on a file.
 * @type {number}
 */
export const R_OK = constants.R_OK || 4

/**
 * Checks for write permission on a file.
 * @type {number}
 */
export const W_OK = constants.W_OK || 2

/**
 * Checks for execute permission on a file.
 * @type {number}
 */
export const X_OK = constants.X_OK || 1

export default exports
