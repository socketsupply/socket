#include "../core.hh"
#include "../headers.hh"
#include "../json.hh"
#include "../resource.hh"
#include "../trace.hh"

#include "fs.hh"

namespace SSC {
  #define CONSTANT(c) { #c, (c) },
  static const std::map<String, int32_t> FS_CONSTANTS = {
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

  JSON::Object getStatsJSON (const String& source, uv_stat_t* stats) {
    return JSON::Object::Entries {
      {"source", source},
      {"data", JSON::Object::Entries {
        {"st_dev", std::to_string(stats->st_dev)},
        {"st_mode", std::to_string(stats->st_mode)},
        {"st_nlink", std::to_string(stats->st_nlink)},
        {"st_uid", std::to_string(stats->st_uid)},
        {"st_gid", std::to_string(stats->st_gid)},
        {"st_rdev", std::to_string(stats->st_rdev)},
        {"st_ino", std::to_string(stats->st_ino)},
        {"st_size", std::to_string(stats->st_size)},
        {"st_blksize", std::to_string(stats->st_blksize)},
        {"st_blocks", std::to_string(stats->st_blocks)},
        {"st_flags", std::to_string(stats->st_flags)},
        {"st_gen", std::to_string(stats->st_gen)},
        {"st_atim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_atim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_atim.tv_nsec)},
        }},
        {"st_mtim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_mtim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_mtim.tv_nsec)}
        }},
        {"st_ctim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_ctim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_ctim.tv_nsec)}
        }},
        {"st_birthtim", JSON::Object::Entries {
          {"tv_sec", std::to_string(stats->st_birthtim.tv_sec)},
          {"tv_nsec", std::to_string(stats->st_birthtim.tv_nsec)}
        }}
      }}
    };
  }

  void CoreFS::RequestContext::setBuffer (SharedPointer<char[]> base, uint32_t len) {
    this->buffer = base;
    this->buf.base = base.get();
    this->buf.len = len;
  }

  CoreFS::Descriptor::Descriptor (CoreFS* fs, ID id, const String& filename)
    : resource(filename, { false, fs->core }),
      fs(fs),
      id(id)
  {}

  bool CoreFS::Descriptor::isDirectory () const {
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    if (this->isAndroidAssetDirectory) {
      return true;
    }
  #endif
    return this->dir != nullptr;
  }

  bool CoreFS::Descriptor::isFile () const {
  #if SOCKET_RUNTIME_PLATFORM_ANDROID
    if (this->androidAsset != nullptr) {
      return true;
    }
  #endif
    return this->fd > 0 && this->dir == nullptr;
  }

  bool CoreFS::Descriptor::isRetained () const {
    return this->retained;
  }

  bool CoreFS::Descriptor::isStale () const {
    return this->stale;
  }

  SharedPointer<CoreFS::Descriptor> CoreFS::getDescriptor (ID id) {
    Lock lock(this->mutex);
    if (descriptors.find(id) != descriptors.end()) {
      return descriptors.at(id);
    }
    return nullptr;
  }

  SharedPointer<CoreFS::Descriptor> CoreFS::getDescriptor (ID id) const {
    if (descriptors.find(id) != descriptors.end()) {
      return descriptors.at(id);
    }
    return nullptr;
  }

  void CoreFS::removeDescriptor (ID id) {
    Lock lock(this->mutex);
    if (descriptors.find(id) != descriptors.end()) {
      descriptors.erase(id);
    }
  }

  bool CoreFS::hasDescriptor (ID id) const {
    return descriptors.find(id) != descriptors.end();
  }

  void CoreFS::retainOpenDescriptor (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    auto desc = getDescriptor(id);

    if (desc == nullptr) {
      auto json = JSON::Object::Entries {
        {"source", "fs.retainOpenDescriptor"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"code", "ENOTOPEN"},
          {"type", "NotFoundError"},
          {"message", "No file descriptor found with that id"}
        }}
      };

      return callback(seq, json, Post{});
    }

    desc->retained = true;
    auto json = JSON::Object::Entries {
      {"source", "fs.retainOpenDescriptor"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(desc->id)}
      }}
    };

    callback(seq, json, Post{});
  }

  void CoreFS::access (
    const String& seq,
    const String& path,
    int mode,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() mutable {
      auto loop = &this->core->eventLoop;
      auto desc = std::make_shared<Descriptor>(this, 0, path);

      if (desc->resource.url.scheme == "socket") {
        if (desc->resource.access(mode) == mode) {
          const auto json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"data", JSON::Object::Entries {
              {"mode", mode},
            }}
          };

          return callback(seq, json, Post{});
        }
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        else if (mode == R_OK || mode == F_OK) {
          auto name = desc->resource.name;

          if (name.starts_with("/")) {
            name = name.substr(1);
          } else if (name.starts_with("./")) {
            name = name.substr(2);
          }

          const auto attachment = Android::JNIEnvironmentAttachment(desc->fs->core->platform.jvm);
          const auto assetManager = CallObjectClassMethodFromAndroidEnvironment(
            attachment.env,
            this->core->platform.activity,
            "getAssetManager",
            "()Landroid/content/res/AssetManager;"
          );

          const auto entries = (jobjectArray) CallObjectClassMethodFromAndroidEnvironment(
            attachment.env,
            assetManager,
            "list",
            "(Ljava/lang/String;)[Ljava/lang/String;",
            attachment.env->NewStringUTF(name.c_str())
          );

          const auto length = attachment.env->GetArrayLength(entries);
          if (length > 0) {
            const auto json = JSON::Object::Entries {
              {"source", "fs.access"},
              {"data", JSON::Object::Entries {
                {"mode", mode},
              }}
            };

            return callback(seq, json, Post{});
          }
        }
        #endif
      }
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      else if (
        desc->resource.url.scheme == "content" ||
        desc->resource.url.scheme == "android.resource"
      ) {
        if (this->core->platform.contentResolver.hasAccess(desc->resource.url.str())) {
          const auto json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"data", JSON::Object::Entries {
              {"mode", mode},
            }}
          };

          return callback(seq, json, Post{});
        }
      }
    #endif

      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_access(loop, req, desc->resource.path.string().c_str(), mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
        #if SOCKET_RUNTIME_PLATFORM_ANDROID
          if (ctx->descriptor->resource.access(req->flags) == req->flags) {
            json = JSON::Object::Entries {
              {"source", "fs.access"},
              {"data", JSON::Object::Entries {
                {"mode", req->flags},
              }}
            };
          } else
        #endif
          {
            json = JSON::Object::Entries {
              {"source", "fs.access"},
              {"err", JSON::Object::Entries {
                {"code", req->result},
                {"message", String(uv_strerror((int) req->result))}
              }}
            };
          }
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.access"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::chmod (
    const String& seq,
    const String& path,
    int mode,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_chmod(loop, req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.chmod"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.chmod"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.chmod"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::chown (
    const String& seq,
    const String& path,
    uv_uid_t uid,
    uv_gid_t gid,
    const CoreModule::Callback& callback
  ) const {
    core->dispatchEventLoop([=, this]() {
      const auto ctx = new RequestContext(seq, callback);
      const auto err = uv_fs_chown(&core->eventLoop, &ctx->req, path.c_str(), uid, gid, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.chown"},
            {"err", JSON::Object::Entries {
              {"code", uv_fs_get_result(req)},
              {"message", String(uv_strerror(uv_fs_get_result(req)))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.chown"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.chown"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->callback(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::lchown (
    const String& seq,
    const String& path,
    uv_uid_t uid,
    uv_gid_t gid,
    const CoreModule::Callback& callback
  ) const {
    core->dispatchEventLoop([=, this]() {
      const auto ctx = new RequestContext(seq, callback);
      const auto err = uv_fs_lchown(&core->eventLoop, &ctx->req, path.c_str(), uid, gid, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.lchown"},
            {"err", JSON::Object::Entries {
              {"code", uv_fs_get_result(req)},
              {"message", String(uv_strerror(uv_fs_get_result(req)))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.lchown"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.lchown"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->callback(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::close (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->androidAsset != nullptr) {
        Lock lock(this->mutex);
        AAsset_close(desc->androidAsset);
        desc->androidAsset = nullptr;
        const auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"fd", desc->fd}
          }}
        };

        this->removeDescriptor(desc->id);
        return callback(seq, json, Post{});
      } else if (
        desc->resource.url.scheme == "content" ||
        desc->resource.url.scheme == "android.resource"
      ) {
        Lock lock(this->mutex);
        this->core->platform.contentResolver.closeFileDescriptor(
          desc->androidContent
        );

        desc->androidContent = nullptr;
        const auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"fd", desc->fd}
          }}
        };

        this->removeDescriptor(desc->id);
        return callback(seq, json, Post{});
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_close(loop, req, desc->fd, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.close"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.close"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"fd", desc->fd}
            }}
          };

          desc->fs->removeDescriptor(desc->id);
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.close"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::open (
    const String& seq,
    ID id,
    const String& path,
    int flags,
    int mode,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = std::make_shared<Descriptor>(this, id, path);

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->resource.url.scheme == "socket") {
        auto assetManager = FileResource::getSharedAndroidAssetManager();
        desc->androidAsset = AAssetManager_open(
          assetManager,
          desc->resource.name.c_str(),
          AASSET_MODE_RANDOM
        );

        desc->fd = AAsset_openFileDescriptor(
          desc->androidAsset,
          &desc->androidAssetOffset,
          &desc->androidAssetLength
        );

        const auto json = JSON::Object::Entries {
          {"source", "fs.open"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"fd", desc->fd}
          }}
        };

        // insert into `descriptors` map
        Lock lock(desc->fs->mutex);
        this->descriptors.insert_or_assign(desc->id, desc);
        return callback(seq, json, Post{});
      } else if (
        desc->resource.url.scheme == "content" ||
        desc->resource.url.scheme == "android.resource"
      ) {
        auto fileDescriptor = this->core->platform.contentResolver.openFileDescriptor(
          desc->resource.url.str(),
          &desc->androidContentOffset,
          &desc->androidContentLength
        );

        if (fileDescriptor == nullptr) {
          const auto json = JSON::Object::Entries {
            {"source", "fs.open"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"type", "NotFoundError"},
              {"message", "Content does not exist at given URI"}
            }}
          };

          return callback(seq, json, Post{});
        }

        desc->fd = this->core->platform.contentResolver.getFileDescriptorFD(
          fileDescriptor
        );

        desc->androidContent = fileDescriptor;

        const auto json = JSON::Object::Entries {
          {"source", "fs.open"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"fd", desc->fd}
          }}
        };

        // insert into `descriptors` map
        Lock lock(desc->fs->mutex);
        this->descriptors.insert_or_assign(desc->id, desc);
        return callback(seq, json, Post{});
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_open(loop, req, desc->resource.path.string().c_str(), flags, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
        #if SOCKET_RUNTIME_PLATFORM_ANDROID
          if (ctx->descriptor->resource.isAndroidLocalAsset()) {
            auto assetManager = FileResource::getSharedAndroidAssetManager();
            desc->androidAsset = AAssetManager_open(
              assetManager,
              ctx->descriptor->resource.name.c_str(),
              AASSET_MODE_RANDOM
            );

            desc->fd = AAsset_openFileDescriptor(
              desc->androidAsset,
              &desc->androidAssetOffset,
              &desc->androidAssetLength
            );
            json = JSON::Object::Entries {
              {"source", "fs.open"},
              {"data", JSON::Object::Entries {
                {"id", std::to_string(desc->id)},
                {"fd", desc->fd}
              }}
            };

            // insert into `descriptors` map
            Lock lock(desc->fs->mutex);
            desc->fs->descriptors.insert_or_assign(desc->id, desc);
          } else
        #endif
          {
            json = JSON::Object::Entries {
              {"source", "fs.open"},
              {"err", JSON::Object::Entries {
                {"id", std::to_string(desc->id)},
                {"code", req->result},
                {"message", String(uv_strerror((int) req->result))}
              }}
            };
          }
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.open"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"fd", (int) req->result}
            }}
          };

          desc->fd = (int) req->result;
          // insert into `descriptors` map
          Lock lock(desc->fs->mutex);
          desc->fs->descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.open"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::opendir (
    const String& seq,
    ID id,
    const String& path,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc =  std::make_shared<Descriptor>(this, id, path);

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->resource.url.scheme == "socket") {
        auto name = desc->resource.name;

        if (name.starts_with("/")) {
          name = name.substr(1);
        } else if (name.starts_with("./")) {
          name = name.substr(2);
        }

        const auto attachment = Android::JNIEnvironmentAttachment(desc->fs->core->platform.jvm);
        const auto assetManager = CallObjectClassMethodFromAndroidEnvironment(
          attachment.env,
          desc->fs->core->platform.activity,
          "getAssetManager",
          "()Landroid/content/res/AssetManager;"
        );

        const auto entries = (jobjectArray) CallObjectClassMethodFromAndroidEnvironment(
          attachment.env,
          assetManager,
          "list",
          "(Ljava/lang/String;)[Ljava/lang/String;",
          attachment.env->NewStringUTF(name.c_str())
        );

        const auto length = attachment.env->GetArrayLength(entries);

        for (int i = 0; i < length; ++i) {
          const auto entry = (jstring) attachment.env->GetObjectArrayElement(entries, i);
          const auto filename = attachment.env->GetStringUTFChars(entry, nullptr);
          if (filename != nullptr) {
            desc->androidAssetDirectoryEntries.push(filename);
            attachment.env->ReleaseStringUTFChars(entry, filename);
          }
        }

        if (length > 0) {
          desc->isAndroidAssetDirectory = true;
          const auto json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)}
            }}
          };

          // insert into `descriptors` map
          Lock lock(desc->fs->mutex);
          desc->fs->descriptors.insert_or_assign(desc->id, desc);
          return callback(seq, json, Post{});
        }
      } else if (
        desc->resource.url.scheme == "content" ||
        desc->resource.url.scheme == "android.resource"
      ) {
        const auto entries = this->core->platform.contentResolver.getPathnameEntriesFromContentURI(
          desc->resource.url.str()
        );

        for (const auto& entry : entries) {
          desc->androidContentDirectoryEntries.push(entry);
        }

        if (entries.size() > 0) {
          desc->isAndroidContentDirectory = true;
          const auto json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)}
            }}
          };

          // insert into `descriptors` map
          Lock lock(desc->fs->mutex);
          desc->fs->descriptors.insert_or_assign(desc->id, desc);
          return callback(seq, json, Post{});
        }
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_opendir(loop, req, desc->resource.path.string().c_str(), [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
        #if SOCKET_RUNTIME_PLATFORM_ANDROID
          auto name = ctx->descriptor->resource.name;

          if (name.starts_with("/")) {
            name = name.substr(1);
          } else if (name.starts_with("./")) {
            name = name.substr(2);
          }

          const auto attachment = Android::JNIEnvironmentAttachment(ctx->descriptor->fs->core->platform.jvm);
          const auto assetManager = CallObjectClassMethodFromAndroidEnvironment(
            attachment.env,
            ctx->descriptor->fs->core->platform.activity,
            "getAssetManager",
            "()Landroid/content/res/AssetManager;"
          );

          const auto entries = (jobjectArray) CallObjectClassMethodFromAndroidEnvironment(
            attachment.env,
            assetManager,
            "list",
            "(Ljava/lang/String;)[Ljava/lang/String;",
            attachment.env->NewStringUTF(name.c_str())
          );

          const auto length = attachment.env->GetArrayLength(entries);

          for (int i = 0; i < length; ++i) {
            const auto entry = (jstring) attachment.env->GetObjectArrayElement(entries, i);
            const auto filename = attachment.env->GetStringUTFChars(entry, nullptr);
            if (filename != nullptr) {
              desc->androidAssetDirectoryEntries.push(filename);
              attachment.env->ReleaseStringUTFChars(entry, filename);
            }
          }

          if (length > 0) {
            desc->isAndroidAssetDirectory = true;
          }

          if (desc->isAndroidAssetDirectory) {
            json = JSON::Object::Entries {
              {"source", "fs.opendir"},
              {"data", JSON::Object::Entries {
                {"id", std::to_string(desc->id)}
              }}
            };

            // insert into `descriptors` map
            Lock lock(desc->fs->mutex);
            desc->fs->descriptors.insert_or_assign(desc->id, desc);
          } else
        #endif
          {
            json = JSON::Object::Entries {
              {"source", "fs.opendir"},
              {"err", JSON::Object::Entries {
                {"id", std::to_string(desc->id)},
                {"code", req->result},
                {"message", String(uv_strerror((int) req->result))}
              }}
            };
          }
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)}
            }}
          };

          desc->dir = (uv_dir_t *) req->ptr;
          // insert into `descriptors` map
          Lock lock(desc->fs->mutex);
          desc->fs->descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.opendir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::readdir (
    const String& seq,
    ID id,
    size_t nentries,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->isAndroidAssetDirectory) {
        Vector<JSON::Any> entries;

        for (
          int i = 0;
          i < nentries && i < desc->androidAssetDirectoryEntries.size();
          ++i
        ) {
          const auto name = desc->androidAssetDirectoryEntries.front();
          const auto encodedName = name.ends_with("/")
            ? encodeURIComponent(name.substr(0, name.size() - 1))
            : encodeURIComponent(name);

          const auto entry = JSON::Object::Entries {
            {"type", name.ends_with("/") ? 2 : 1},
            {"name", encodedName}
          };

          entries.push_back(entry);
          desc->androidAssetDirectoryEntries.pop();
        }

        const auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"data", entries}
        };

        return callback(seq, json, Post{});
      } else if (desc->isAndroidContentDirectory) {
        Vector<JSON::Any> entries;

        for (
          int i = 0;
          i < nentries && i < desc->androidContentDirectoryEntries.size();
          ++i
        ) {
          const auto name = desc->androidContentDirectoryEntries.front();
          const auto encodedName = name.ends_with("/")
            ? encodeURIComponent(name.substr(0, name.size() - 1))
            : encodeURIComponent(name);

          const auto entry = JSON::Object::Entries {
            {"type", name.ends_with("/") ? 2 : 1},
            {"name", encodedName}
          };

          entries.push_back(entry);
          desc->androidContentDirectoryEntries.pop();
        }

        const auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"data", entries}
        };

        return callback(seq, json, Post{});
      }
    #endif

      if (!desc->isDirectory()) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

      Lock lock(desc->mutex);
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;

      desc->dir->dirents = ctx->dirents;
      desc->dir->nentries = nentries;

      auto err = uv_fs_readdir(loop, req, desc->dir, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.readdir"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          Vector<JSON::Any> entries;

          for (int i = 0; i < req->result; ++i) {
            auto entry = JSON::Object::Entries {
              {"type", desc->dir->dirents[i].type},
              {"name", encodeURIComponent(desc->dir->dirents[i].name)}
            };

            entries.push_back(entry);
          }

          json = JSON::Object::Entries {
            {"source", "fs.readdir"},
            {"data", entries}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::closedir (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->isAndroidAssetDirectory) {
        Lock lock(this->mutex);
        desc->isAndroidAssetDirectory = false;
        desc->androidAssetDirectoryEntries = {};
        const auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(desc->id)}
          }}
        };

        this->removeDescriptor(desc->id);
        return callback(seq, json, Post{});
      } else if (desc->isAndroidContentDirectory) {
        Lock lock(this->mutex);
        desc->isAndroidContentDirectory = false;
        desc->androidContentDirectoryEntries = {};
        const auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(desc->id)}
          }}
        };

        this->removeDescriptor(desc->id);
        return callback(seq, json, Post{});
      }
    #endif

      if (!desc->isDirectory()) {
        auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"message", "The descriptor found with was not a directory"}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_closedir(loop, req, desc->dir, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.closedir"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.closedir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"fd", desc->fd}
            }}
          };

          desc->fs->removeDescriptor(desc->id);
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::closeOpenDescriptor (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    auto desc = getDescriptor(id);

    if (desc == nullptr) {
      auto json = JSON::Object::Entries {
        {"source", "fs.closeOpenDescriptor"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(id)},
          {"code", "ENOTOPEN"},
          {"type", "NotFoundError"},
          {"message", "No descriptor found with that id"}
        }}
      };

      return callback(seq, json, Post{});
    }

    if (desc->isDirectory()) {
      this->closedir(seq, id, callback);
    } else if (desc->isFile()) {
      this->close(seq, id, callback);
    }
  }

  void CoreFS::closeOpenDescriptors (const String& seq, const CoreModule::Callback& callback) {
    return this->closeOpenDescriptors(seq, false, callback);
  }

  void CoreFS::closeOpenDescriptors (
    const String& seq,
    bool preserveRetained,
    const CoreModule::Callback& callback
  ) {
    Lock lock(this->mutex);

    auto pending = descriptors.size();
    int queued = 0;
    auto json = JSON::Object {};
    auto ids = Vector<uint64_t> {};

    for (auto const &tuple : descriptors) {
      ids.push_back(tuple.first);
    }

    for (auto const id : ids) {
      auto desc = descriptors[id];
      pending--;

      if (desc == nullptr) {
        descriptors.erase(id);
        continue;
      }

      if (preserveRetained && desc->isRetained()) {
        continue;
      }

      if (desc->isDirectory()) {
        queued++;
        this->closedir(seq, id, [pending, callback](auto seq, auto json, auto post) {
          if (pending == 0) {
            callback(seq, json, post);
          }
        });
      } else if (desc->isFile()) {
        queued++;
        this->close(seq, id, [pending, callback](auto seq, auto json, auto post) {
          if (pending == 0) {
            callback(seq, json, post);
          }
        });
      }
    }

    if (queued == 0) {
      callback(seq, json, Post{});
    }
  }

  void CoreFS::read (
    const String& seq,
    ID id,
    size_t size,
    size_t offset,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() mutable {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.read"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->androidAsset != nullptr) {
        const auto length = AAsset_getLength(desc->androidAsset);

        if (offset >= static_cast<size_t>(length)) {
          const auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", 0}
          }};

          Post post {0};
          post.id = rand64();
          post.body = std::make_shared<char[]>(size);
          post.length = 0;
          post.headers = headers.str();
          return callback(seq, JSON::Object{}, post);
        } else {
          if (size > length - offset) {
            size = length - offset;
          }

          offset += desc->androidAssetOffset;
        }
      } else if (desc->androidContent != nullptr) {
        const auto length = desc->androidContentLength;

        if (offset >= static_cast<size_t>(length)) {
          const auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", 0}
          }};

          Post post {0};
          post.id = rand64();
          post.body = std::make_shared<char[]>(size);
          post.length = 0;
          post.headers = headers.str();
          return callback(seq, JSON::Object{}, post);
        } else {
          if (size > length - offset) {
            size = length - offset;
          }

          offset += desc->androidContentOffset;
        }
      }
    #endif

      auto bytes = std::make_shared<char[]>(size);
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;

      ctx->setBuffer(bytes, size);

      auto err = uv_fs_read(loop, req, desc->fd, &ctx->buf, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};
        Post post = {0};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.read"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", req->result}
          }};

          post.id = rand64();
          post.body = ctx->buffer;
          post.length = (int) req->result;
          post.headers = headers.str();
        }

        ctx->callback(ctx->seq, json, post);
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.read"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::watch (
    const String& seq,
    ID id,
    const String& path,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      auto json = JSON::Object::Entries {
        {"source", "fs.watch"},
        {"err", JSON::Object::Entries {
          {"message", "Not supported"}
        }}
      };

      callback(seq, json, Post{});
      return;
    #else
      SharedPointer<FileSystemWatcher> watcher;
      {
        Lock lock(this->mutex);
        watcher = this->watchers[id];
      }

      if (watcher == nullptr) {
        watcher.reset(new FileSystemWatcher(path));
        watcher->core = this->core;
        const auto started = watcher->start([=, this](
          const auto& changed,
          const auto& events,
          const auto& context
        ) mutable {
          JSON::Array::Entries eventNames;

          if (std::find(events.begin(), events.end(), FileSystemWatcher::Event::RENAME) != events.end()) {
            eventNames.push_back("rename");
          }

          if (std::find(events.begin(), events.end(), FileSystemWatcher::Event::CHANGE) != events.end()) {
            eventNames.push_back("change");
          }

          auto json = JSON::Object::Entries {
            {"source", "fs.watch"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"events",eventNames},
              {"path", std::filesystem::relative(changed, path).string()}
            }}
          };

          callback("-1", json, Post{});
        });

        if (!started) {
          auto json = JSON::Object::Entries {
            {"source", "fs.watch"},
            {"err", JSON::Object::Entries {
              {"message", "Failed to start 'fs.Watcher'"}
            }}
          };

          callback(seq, json, Post{});
          return;
        }

        {
          Lock lock(this->mutex);
          this->watchers.insert_or_assign(id, watcher);
        }
      }

      auto json = JSON::Object::Entries {
        {"source", "fs.watch"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)}
        }}
      };

      callback(seq, json, Post{});
    #endif
    });
  }

  void CoreFS::write (
    const String& seq,
    ID id,
    SharedPointer<char[]> bytes,
    size_t size,
    size_t offset,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.write"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->androidAsset != nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.write"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "EPERM"},
            {"type", "NotAllowedError"},
            {"message", "Cannot write to an Android Asset file descriptor."}
          }}
        };

        return callback(seq, json, Post{});
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;

      ctx->setBuffer(bytes, size);
      auto err = uv_fs_write(loop, req, desc->fd, &ctx->buf, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.write"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.write"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"result", req->result}
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.write"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(desc->id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::stat (
    const String& seq,
    const String& path,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = std::make_shared<Descriptor>(this, 0, path);

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->resource.isAndroidLocalAsset()) {
        const auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"data", JSON::Object::Entries {
            {"st_mode", R_OK},
            {"st_size", desc->resource.size()}
          }}
        };

        return callback(seq, json, Post{});
      } else if (
        desc->resource.url.scheme == "content" ||
        desc->resource.url.scheme == "android.resource"
      ) {
        const auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"data", JSON::Object::Entries {
            {"st_mode", R_OK},
            {"st_size", desc->resource.size()}
          }}
        };

        return callback(seq, json, Post{});
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_stat(loop, req, desc->resource.path.string().c_str(), [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.stat"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = getStatsJSON("fs.stat", uv_fs_get_statbuf(req));
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::stopWatch (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      auto json = JSON::Object::Entries {
        {"source", "fs.stopWatch"},
        {"err", JSON::Object::Entries {
          {"message", "Not supported"}
        }}
      };

      callback(seq, json, Post{});
      return;
    #else
      auto watcher = this->watchers[id];
      if (watcher != nullptr) {
        watcher->stop();
        this->watchers.erase(id);
        auto json = JSON::Object::Entries {
          {"source", "fs.stopWatch"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
          }}
        };
        callback(seq, json, Post{});
      } else {
        auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "fs.Watcher does not exist"}
          }}
        };

        callback(seq, json, Post{});
      }
    #endif
    });
  }

  void CoreFS::fsync (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.fsync"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_fsync(loop, req, desc->fd, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.fsync"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.fsync"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.fsync"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::ftruncate (
    const String& seq,
    ID id,
    int64_t offset,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.ftruncate"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_ftruncate(loop, req, desc->fd, offset, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.ftruncate"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.ftruncate"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.ftruncate"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::fstat (
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = getDescriptor(id);

      if (desc == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "fs.fstat"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"type", "NotFoundError"},
            {"message", "No file descriptor found with that id"}
          }}
        };

        return callback(seq, json, Post{});
      }

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (desc->androidAsset != nullptr) {
        const auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"data", JSON::Object::Entries {
            {"st_mode", R_OK},
            {"st_size", desc->resource.size()}
          }}
        };

        return callback(seq, json, Post{});
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_fstat(loop, req, desc->fd, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->descriptor;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.fstat"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = getStatsJSON("fs.fstat", uv_fs_get_statbuf(req));
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.fstat"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::getOpenDescriptors (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    auto entries = Vector<JSON::Any> {};

    for (auto const &tuple : descriptors) {
      auto desc = tuple.second;

      if (!desc || (desc->isStale() && !desc->isRetained())) {
        continue;
      }

      auto entry = JSON::Object::Entries {
        {"id",  std::to_string(desc->id)},
        {"fd", std::to_string(desc->isDirectory() ? desc->id : desc->fd)},
        {"type", desc->dir ? "directory" : "file"}
      };

      entries.push_back(entry);
    }

    auto json = JSON::Object::Entries {
      {"source", "fs.getOpenDescriptors"},
      {"data", entries}
    };

    callback(seq, json, Post{});
  }

  void CoreFS::lstat (
    const String& seq,
    const String& path,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto desc = std::make_shared<Descriptor>(this, 0, path);
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_lstat(loop, req, desc->resource.path.string().c_str(), [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.stat"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = getStatsJSON("fs.lstat", uv_fs_get_statbuf(req));
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

	void CoreFS::link (
    const String& seq,
    const String& src,
    const String& dest,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, callback);
      auto err = uv_fs_link(&core->eventLoop, &ctx->req, src.c_str(), dest.c_str(), [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.link"},
            {"err", JSON::Object::Entries {
              {"code", uv_fs_get_result(req)},
              {"message", String(uv_strerror(uv_fs_get_result(req)))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.link"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.link"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->callback(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::symlink (
    const String& seq,
    const String& src,
    const String& dest,
    int flags,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, callback);
      auto err = uv_fs_symlink(&core->eventLoop, &ctx->req, src.c_str(), dest.c_str(), flags, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.symlink"},
            {"err", JSON::Object::Entries {
              {"code", uv_fs_get_result(req)},
              {"message", String(uv_strerror(uv_fs_get_result(req)))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.symlink"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.symlink"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->callback(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::unlink (
    const String& seq,
    const String& path,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_unlink(loop, req, filename, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.unlink"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.unlink"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.unlink"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

	void CoreFS::readlink (
    const String& seq,
    const String& path,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, callback);
      auto err = uv_fs_readlink(&core->eventLoop, &ctx->req, path.c_str(), [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.readlink"},
            {"err", JSON::Object::Entries {
              {"code", uv_fs_get_result(req)},
              {"message", String(uv_strerror(uv_fs_get_result(req)))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.readlink"},
            {"data", JSON::Object::Entries {
              {"path", String((char*) uv_fs_get_ptr(req))}
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readlink"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->callback(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::realpath (
    const String& seq,
    const String& path,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str(); auto desc = std::make_shared<Descriptor>(this, 0, filename);
      auto ctx = new RequestContext(desc, seq, callback);
      auto err = uv_fs_realpath(&core->eventLoop, &ctx->req, path.c_str(), [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
        #if SOCKET_RUNTIME_PLATFORM_ANDROID
          if (ctx->descriptor->resource.isAndroidLocalAsset()) {
            json = JSON::Object::Entries {
              {"source", "fs.realpath"},
              {"data", JSON::Object::Entries {
                {"path", ctx->descriptor->resource.path}
              }}
            };
          } else if (ctx->descriptor->resource.isAndroidContent()) {
            json = JSON::Object::Entries {
              {"source", "fs.realpath"},
              {"data", JSON::Object::Entries {
                {"path", ctx->descriptor->resource.url.str()}
              }}
            };
          } else
        #endif
          {
            json = JSON::Object::Entries {
              {"source", "fs.realpath"},
              {"err", JSON::Object::Entries {
                {"code", uv_fs_get_result(req)},
                {"message", String(uv_strerror(uv_fs_get_result(req)))}
              }}
            };
          }
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.realpath"},
            {"data", JSON::Object::Entries {
              {"path", String((char*) uv_fs_get_ptr(req))}
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post {});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.realpath"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->callback(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::rename (
    const String& seq,
    const String& pathA,
    const String& pathB,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto src = pathA.c_str();
      auto dst = pathB.c_str();
      auto err = uv_fs_rename(loop, req, src, dst, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.rename"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.rename"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.rename"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::copyFile (
    const String& seq,
    const String& pathA,
    const String& pathB,
    int flags,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto src = std::make_shared<Descriptor>(this, 0, pathA);
      auto dst = std::make_shared<Descriptor>(this, 0, pathB);

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      if (src->resource.isAndroidLocalAsset() || src->resource.isAndroidLocalAsset()) {
        auto bytes = src->resource.read();
        fs::remove(dst->resource.path.string());
        std::ofstream stream(dst->resource.path.string());
        stream << bytes;
        stream.close();

        auto json = JSON::Object::Entries {
          {"source", "fs.copyFile"},
          {"data", JSON::Object::Entries {
            {"result", 0}
          }}
        };

        return callback(seq, json, Post{});
      }
    #endif

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_copyfile(loop, req, src->resource.path.string().c_str(), dst->resource.path.string().c_str(), flags, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.copyFile"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.copyFile"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.copyFile"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::rmdir (
    const String& seq,
    const String& path,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, callback);
      auto req = &ctx->req;
      auto err = uv_fs_rmdir(loop, req, filename, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.rmdir"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.rmdir"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      });

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.rmdir"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::mkdir (
    const String& seq,
    const String& path,
    int mode,
    bool recursive,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this]() {
      int err = 0;
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, callback);
      ctx->recursive = recursive;
      auto req = &ctx->req;
      const auto callback = [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        int result = uv_fs_get_result(req);

        // Report recursive errors only when they are not EEXIST
        if (result < 0 && (!ctx->recursive || (result != -EEXIST && result != -4075))) {
          json = JSON::Object::Entries {
            {"source", "fs.mkdir"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.mkdir"},
            {"data", JSON::Object::Entries {
              {"result", req->result},
            }}
          };
        }

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      };

      if (!recursive) {
        err = uv_fs_mkdir(loop, req, filename, mode, callback);
      } else {
        const auto sep = String(1, std::filesystem::path::preferred_separator);
        const auto components = split(path, sep);
        auto queue = std::queue(std::deque(components.begin(), components.end()));
        auto currentComponents = Vector<String>();
        while (queue.size() > 0) {
          uv_fs_t req;
          const auto currentComponent = queue.front();
          queue.pop();
          currentComponents.push_back(currentComponent);
          const auto joinedComponents = join(currentComponents, sep);
          const auto currentPath = joinedComponents.empty() ? sep : joinedComponents;
          if (queue.size() == 0) {
            err = uv_fs_mkdir(loop, &ctx->req, currentPath.c_str(), mode, callback);
          } else {
            err = uv_fs_mkdir(loop, &req, currentPath.c_str(), mode, nullptr);
          }

          if (err == 0 || err == -EEXIST || err == -4075) { // '4075' is EEXIST on Windows
            err = 0;
            continue;
          } else {
            break;
          }
        }
      }

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.mkdir"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void CoreFS::constants (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    static const auto data = JSON::Object(FS_CONSTANTS);
    static const auto json = JSON::Object::Entries {
      {"source", "fs.constants"},
      {"data", data}
    };

    callback(seq, json, Post {});
  }
}
