#include "../filesystem.hh"

namespace ssc::runtime::filesystem {
  #define CONSTANT(c) { #c, (c) },
  static const Map<String, int32_t> CONSTANTS = {
    #if defined(UV_DIRENT_UNKNOWN)
    CONSTANT(UV_DIRENT_UNKNOWN)
    #endif
    #if defined(UV_DIRENT_FILE)
    CONSTANT(UV_DIRENT_FILE)
    #endif
    #if defined(UV_DIRENT_DIR)
    CONSTANT(UV_DIRENT_DIR)
    #endif
    #if defined(UV_DIRENT_LINK)
    CONSTANT(UV_DIRENT_LINK)
    #endif
    #if defined(UV_DIRENT_FIFO)
    CONSTANT(UV_DIRENT_FIFO)
    #endif
    #if defined(UV_DIRENT_SOCKET)
    CONSTANT(UV_DIRENT_SOCKET)
    #endif
    #if defined(UV_DIRENT_CHAR)
    CONSTANT(UV_DIRENT_CHAR)
    #endif
    #if defined(UV_DIRENT_BLOCK)
    CONSTANT(UV_DIRENT_BLOCK)
    #endif
    #if defined(UV_FS_O_FILEMAP)
    CONSTANT(UV_FS_O_FILEMAP)
    #endif
    #if defined(O_RDONLY)
    CONSTANT(O_RDONLY)
    #endif
    #if defined(O_WRONLY)
    CONSTANT(O_WRONLY)
    #endif
    #if defined(O_RDWR)
    CONSTANT(O_RDWR)
    #endif
    #if defined(O_APPEND)
    CONSTANT(O_APPEND)
    #endif
    #if defined(O_ASYNC)
    CONSTANT(O_ASYNC)
    #endif
    #if defined(O_CLOEXEC)
    CONSTANT(O_CLOEXEC)
    #endif
    #if defined(O_CREAT)
    CONSTANT(O_CREAT)
    #endif
    #if defined(O_DIRECT)
    CONSTANT(O_DIRECT)
    #endif
    #if defined(O_DIRECTORY)
    CONSTANT(O_DIRECTORY)
    #endif
    #if defined(O_DSYNC)
    CONSTANT(O_DSYNC)
    #endif
    #if defined(O_EXCL)
    CONSTANT(O_EXCL)
    #endif
    #if defined(O_LARGEFILE)
    CONSTANT(O_LARGEFILE)
    #endif
    #if defined(O_NOATIME)
    CONSTANT(O_NOATIME)
    #endif
    #if defined(O_NOCTTY)
    CONSTANT(O_NOCTTY)
    #endif
    #if defined(O_NOFOLLOW)
    CONSTANT(O_NOFOLLOW)
    #endif
    #if defined(O_NONBLOCK)
    CONSTANT(O_NONBLOCK)
    #endif
    #if defined(O_NDELAY)
    CONSTANT(O_NDELAY)
    #endif
    #if defined(O_PATH)
    CONSTANT(O_PATH)
    #endif
    #if defined(O_SYNC)
    CONSTANT(O_SYNC)
    #endif
    #if defined(O_TMPFILE)
    CONSTANT(O_TMPFILE)
    #endif
    #if defined(O_TRUNC)
    CONSTANT(O_TRUNC)
    #endif
    #if defined(S_IFMT)
    CONSTANT(S_IFMT)
    #endif
    #if defined(S_IFREG)
    CONSTANT(S_IFREG)
    #endif
    #if defined(S_IFDIR)
    CONSTANT(S_IFDIR)
    #endif
    #if defined(S_IFCHR)
    CONSTANT(S_IFCHR)
    #endif
    #if defined(S_IFBLK)
    CONSTANT(S_IFBLK)
    #endif
    #if defined(S_IFIFO)
    CONSTANT(S_IFIFO)
    #endif
    #if defined(S_IFLNK)
    CONSTANT(S_IFLNK)
    #endif
    #if defined(S_IFSOCK)
    CONSTANT(S_IFSOCK)
    #endif
    #if defined(S_IRWXU)
    CONSTANT(S_IRWXU)
    #endif
    #if defined(S_IRUSR)
    CONSTANT(S_IRUSR)
    #endif
    #if defined(S_IWUSR)
    CONSTANT(S_IWUSR)
    #endif
    #if defined(S_IXUSR)
    CONSTANT(S_IXUSR)
    #endif
    #if defined(S_IRWXG)
    CONSTANT(S_IRWXG)
    #endif
    #if defined(S_IRGRP)
    CONSTANT(S_IRGRP)
    #endif
    #if defined(S_IWGRP)
    CONSTANT(S_IWGRP)
    #endif
    #if defined(S_IXGRP)
    CONSTANT(S_IXGRP)
    #endif
    #if defined(S_IRWXO)
    CONSTANT(S_IRWXO)
    #endif
    #if defined(S_IROTH)
    CONSTANT(S_IROTH)
    #endif
    #if defined(S_IWOTH)
    CONSTANT(S_IWOTH)
    #endif
    #if defined(S_IXOTH)
    CONSTANT(S_IXOTH)
    #endif
    #if defined(F_OK)
    CONSTANT(F_OK)
    #endif
    #if defined(R_OK)
    CONSTANT(R_OK)
    #endif
    #if defined(W_OK)
    CONSTANT(W_OK)
    #endif
    #if defined(X_OK)
    CONSTANT(X_OK)
    #endif
  };
  #undef CONSTANT

  const Map<String, int32_t>& constants () {
    return CONSTANTS;
  }
}
