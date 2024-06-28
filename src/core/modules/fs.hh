#ifndef SOCKET_RUNTIME_CORE_MODULE_FS_H
#define SOCKET_RUNTIME_CORE_MODULE_FS_H

#include "../file_system_watcher.hh"
#include "../resource.hh"
#include "../module.hh"
#include "../trace.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../../platform/android.hh"
#endif

namespace SSC {
  class Core;
  class CoreFS : public CoreModule {
    public:
      using ID = uint64_t;

      struct Descriptor {
        ID id;
        Atomic<bool> retained = false;
        Atomic<bool> stale = false;
        FileResource resource;
        Mutex mutex;
        uv_dir_t *dir = nullptr;
        uv_file fd = 0;
        CoreFS* fs = nullptr;

        Descriptor (CoreFS* fs, ID id, const String& filename);
        bool isDirectory () const;
        bool isFile () const;
        bool isRetained () const;
        bool isStale () const;
      };

      struct RequestContext : CoreModule::RequestContext {
        ID id;
        SharedPointer<Descriptor> descriptor = nullptr;
        SharedPointer<char[]> buffer = nullptr;
        Tracer tracer;
        uv_fs_t req;
        uv_buf_t buf;
        // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
        uv_dirent_t dirents[256];
        int offset = 0;
        int result = 0;
        bool recursive;

        RequestContext () = delete;
        RequestContext (SharedPointer<Descriptor> descriptor)
          : RequestContext(descriptor, "", nullptr)
        {}

        RequestContext (const String& seq, const Callback& callback)
          : RequestContext(nullptr, seq, callback)
        {}

        RequestContext (
          SharedPointer<Descriptor> descriptor,
          const String& seq,
          const Callback& callback
        ) : tracer("CoreFS::RequestContext") {
          this->id = rand64();
          this->seq = seq;
          this->req.data = (void*) this;
          this->callback = callback;
          this->descriptor = descriptor;
          this->recursive = false;
          this->req.loop = nullptr;
        }

        ~RequestContext () {
          if (this->req.loop) {
            uv_fs_req_cleanup(&this->req);
          }
        }

        void setBuffer (SharedPointer<char[]> base, uint32_t size);
      };

      std::map<ID, SharedPointer<FileSystemWatcher>> watchers;
      std::map<ID, SharedPointer<Descriptor>> descriptors;
      Mutex mutex;

      CoreFS (Core* core)
        : CoreModule(core)
      {}

      SharedPointer<Descriptor> getDescriptor (ID id) const;
      SharedPointer<Descriptor> getDescriptor (ID id);
      void removeDescriptor (ID id);
      bool hasDescriptor (ID id) const;

      void constants (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void access (
        const String& seq,
        const String& path,
        int mode,
        const CoreModule::Callback& callback
      ) const;

      void chmod (
        const String& seq,
        const String& path,
        int mode,
        const CoreModule::Callback& callback
      ) const;

      void chown (
        const String& seq,
        const String& path,
        uv_uid_t uid,
        uv_gid_t gid,
        const CoreModule::Callback& callback
      ) const;

      void lchown (
        const String& seq,
        const String& path,
        uv_uid_t uid,
        uv_gid_t gid,
        const CoreModule::Callback& callback
      ) const;

      void close (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void copyFile (
        const String& seq,
        const String& src,
        const String& dst,
        int flags,
        const CoreModule::Callback& callback
      ) const;

      void closedir (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void closeOpenDescriptor (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void closeOpenDescriptors (
        const String& seq,
        const CoreModule::Callback& callback
      );

      void closeOpenDescriptors (
        const String& seq,
        bool preserveRetained,
        const CoreModule::Callback& callback
      );

      void fstat (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      ) const;

      void fsync (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      ) const;

      void ftruncate (
        const String& seq,
        ID id,
        int64_t offset,
        const CoreModule::Callback& callback
      ) const;

      void getOpenDescriptors (
        const String& seq,
        const CoreModule::Callback& callback
      ) const;

      void lstat (
        const String& seq,
        const String& path,
        const CoreModule::Callback& callback
      ) const;

      void link (
        const String& seq,
        const String& src,
        const String& dest,
        const CoreModule::Callback& callback
      ) const;

      void symlink (
        const String& seq,
        const String& src,
        const String& dest,
        int flags,
        const CoreModule::Callback& callback
      ) const;

      void mkdir (
        const String& seq,
        const String& path,
        int mode,
        bool recursive,
        const CoreModule::Callback& callback
      ) const;

      void readlink (
        const String& seq,
        const String& path,
        const CoreModule::Callback& callback
      ) const;

      void realpath (
        const String& seq,
        const String& path,
        const CoreModule::Callback& callback
      ) const;

      void open (
        const String& seq,
        ID id,
        const String& path,
        int flags,
        int mode,
        const CoreModule::Callback& callback
      );

      void opendir (
        const String& seq,
        ID id,
        const String& path,
        const CoreModule::Callback& callback
      );

      void read (
        const String& seq,
        ID id,
        size_t len,
        size_t offset,
        const CoreModule::Callback& callback
      ) const;

      void readdir (
        const String& seq,
        ID id,
        size_t entries,
        const CoreModule::Callback& callback
      ) const;

      void retainOpenDescriptor (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void rename (
        const String& seq,
        const String& src,
        const String& dst,
        const CoreModule::Callback& callback
      ) const;

      void rmdir (
        const String& seq,
        const String& path,
        const CoreModule::Callback& callback
      ) const;

      void stat (
        const String& seq,
        const String& path,
        const CoreModule::Callback& callback
      ) const;

      void stopWatch (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void unlink (
        const String& seq,
        const String& path,
        const CoreModule::Callback& callback
      ) const;

      void watch (
        const String& seq,
        ID id,
        const String& path,
        const CoreModule::Callback& callback
      );

      void write (
        const String& seq,
        ID id,
        SharedPointer<char[]> bytes,
        size_t size,
        size_t offset,
        const CoreModule::Callback& callback
      ) const;
  };
}
#endif
