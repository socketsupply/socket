#ifndef SOCKET_RUNTIME_CORE_SERVICES_FS_H
#define SOCKET_RUNTIME_CORE_SERVICES_FS_H

#include "../../filesystem.hh"
#include "../../debug.hh"
#include "../../core.hh"
#include "../../ipc.hh"

namespace ssc::runtime::core::services {
  class FS : public core::Service {
    public:
      using ID = uint64_t;

      struct Descriptor {
        ID id;
        Atomic<bool> retained = false;
        Atomic<bool> stale = false;
        filesystem::Resource resource;
        Mutex mutex;
        uv_dir_t *dir = nullptr;
        uv_file fd = 0;
        FS* fs = nullptr;

      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        // asset state
        Android::Asset* androidAsset = nullptr;
        Queue<String> androidAssetDirectoryEntries;
        Queue<String> androidContentDirectoryEntries;
        Android::ContentResolver::FileDescriptor androidContent = nullptr;
        // type predicates
        bool isAndroidAssetDirectory = false;
        bool isAndroidContentDirectory = false;
        bool isAndroidContent = false;
        // descriptor offsets
        off_t androidAssetOffset = 0;
        off_t androidAssetLength = 0;
        off_t androidContentOffset = 0;
        off_t androidContentLength = 0;
      #endif

        Descriptor (FS* fs, ID, const String& filename);
        bool isDirectory () const;
        bool isFile () const;
        bool isRetained () const;
        bool isStale () const;
      };

      struct RequestContext : core::Service::RequestContext {
        ID id;
        SharedPointer<Descriptor> descriptor = nullptr;
        SharedPointer<char[]> buffer = nullptr;
        debug::Tracer tracer;
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

        RequestContext (const ipc::Message::Seq& seq, const Callback callback)
          : RequestContext(nullptr, seq, callback)
        {}

        RequestContext (
          SharedPointer<Descriptor> descriptor,
          const ipc::Message::Seq&,
          const Callback
        ) : tracer("FS::RequestContext")
        {
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

      Map<ID, SharedPointer<filesystem::Watcher>> watchers;
      Map<ID, SharedPointer<Descriptor>> descriptors;
      Mutex mutex;

      FS (const Options& options)
        : core::Service(options)
      {}

      SharedPointer<Descriptor> getDescriptor (ID) const;
      SharedPointer<Descriptor> getDescriptor (ID);
      void removeDescriptor (ID);
      bool hasDescriptor (ID) const;

      void constants (const ipc::Message::Seq&, const Callback) const;
      void access (const ipc::Message::Seq&, const String&, int, const Callback);
      void chmod (const ipc::Message::Seq&, const String&, int, const Callback) const;
      void chown (const ipc::Message::Seq&, const String&, uv_uid_t, uv_gid_t, const Callback) const;
      void lchown (const ipc::Message::Seq&, const String&, uv_uid_t, uv_gid_t, const Callback) const;

      void close (const ipc::Message::Seq&, ID, const Callback);
      void copyFile (const ipc::Message::Seq&, const String&, const String&, int, const Callback);
      void closedir (const ipc::Message::Seq&, ID, const Callback);

      void closeOpenDescriptor (const ipc::Message::Seq&, ID, const Callback);
      void closeOpenDescriptors (const ipc::Message::Seq&, const Callback);
      void closeOpenDescriptors (const ipc::Message::Seq&, bool, const Callback);
      void fstat (const ipc::Message::Seq&, ID, const Callback);
      void fsync (const ipc::Message::Seq&, ID, const Callback) const;
      void ftruncate (const ipc::Message::Seq&, ID, int64_t, const Callback) const;
      void getOpenDescriptors (const ipc::Message::Seq&, const Callback) const;
      void lstat (const ipc::Message::Seq&, const String&, const Callback);
      void link (const ipc::Message::Seq&, const String&, const String&, const Callback) const;
      void symlink (const ipc::Message::Seq&, const String&, const String&, int , const Callback) const;
      void mkdir (const ipc::Message::Seq&, const String&, int, bool, const Callback) const;
      void readlink (const ipc::Message::Seq&, const String&, const Callback) const;
      void realpath (const ipc::Message::Seq&, const String&, const Callback);
      void open (const ipc::Message::Seq&, ID, const String&, int, int, const Callback);
      void opendir (const ipc::Message::Seq&, ID, const String&, const Callback);
      void read (const ipc::Message::Seq&, ID, size_t, size_t, const Callback) const;
      void readdir (const ipc::Message::Seq&, ID, size_t, const Callback) const;
      void retainOpenDescriptor (const ipc::Message::Seq&, ID, const Callback);
      void rename (const ipc::Message::Seq&, const String&, const String&, const Callback) const;
      void rmdir (const ipc::Message::Seq&, const String&, const Callback) const;
      void stat (const ipc::Message::Seq&, const String&, const Callback);
      void stopWatch (const ipc::Message::Seq&, ID, const Callback);
      void unlink (const ipc::Message::Seq&, const String&, const Callback) const;
      void watch (const ipc::Message::Seq&, ID, const String&, const Callback);
      void write (const ipc::Message::Seq&, ID, SharedPointer<char[]>, size_t, size_t, const Callback) const;
  };
}
#endif
