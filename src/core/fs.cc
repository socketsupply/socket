#include "core.hh"
namespace SSC {

  #define SET_CONSTANT(c) constants[#c] = (c);
  static std::map<String, int32_t> getFSConstantsMap () {
    std::map<String, int32_t> constants;

    #if defined(UV_DIRENT_UNKNOWN)
    SET_CONSTANT(UV_DIRENT_UNKNOWN)
    #endif
    #if defined(UV_DIRENT_FILE)
    SET_CONSTANT(UV_DIRENT_FILE)
    #endif
    #if defined(UV_DIRENT_DIR)
    SET_CONSTANT(UV_DIRENT_DIR)
    #endif
    #if defined(UV_DIRENT_LINK)
    SET_CONSTANT(UV_DIRENT_LINK)
    #endif
    #if defined(UV_DIRENT_FIFO)
    SET_CONSTANT(UV_DIRENT_FIFO)
    #endif
    #if defined(UV_DIRENT_SOCKET)
    SET_CONSTANT(UV_DIRENT_SOCKET)
    #endif
    #if defined(UV_DIRENT_CHAR)
    SET_CONSTANT(UV_DIRENT_CHAR)
    #endif
    #if defined(UV_DIRENT_BLOCK)
    SET_CONSTANT(UV_DIRENT_BLOCK)
    #endif
    #if defined(O_RDONLY)
    SET_CONSTANT(O_RDONLY);
    #endif
    #if defined(O_WRONLY)
    SET_CONSTANT(O_WRONLY);
    #endif
    #if defined(O_RDWR)
    SET_CONSTANT(O_RDWR);
    #endif
    #if defined(O_APPEND)
    SET_CONSTANT(O_APPEND);
    #endif
    #if defined(O_ASYNC)
    SET_CONSTANT(O_ASYNC);
    #endif
    #if defined(O_CLOEXEC)
    SET_CONSTANT(O_CLOEXEC);
    #endif
    #if defined(O_CREAT)
    SET_CONSTANT(O_CREAT);
    #endif
    #if defined(O_DIRECT)
    SET_CONSTANT(O_DIRECT);
    #endif
    #if defined(O_DIRECTORY)
    SET_CONSTANT(O_DIRECTORY);
    #endif
    #if defined(O_DSYNC)
    SET_CONSTANT(O_DSYNC);
    #endif
    #if defined(O_EXCL)
    SET_CONSTANT(O_EXCL);
    #endif
    #if defined(O_LARGEFILE)
    SET_CONSTANT(O_LARGEFILE);
    #endif
    #if defined(O_NOATIME)
    SET_CONSTANT(O_NOATIME);
    #endif
    #if defined(O_NOCTTY)
    SET_CONSTANT(O_NOCTTY);
    #endif
    #if defined(O_NOFOLLOW)
    SET_CONSTANT(O_NOFOLLOW);
    #endif
    #if defined(O_NONBLOCK)
    SET_CONSTANT(O_NONBLOCK);
    #endif
    #if defined(O_NDELAY)
    SET_CONSTANT(O_NDELAY);
    #endif
    #if defined(O_PATH)
    SET_CONSTANT(O_PATH);
    #endif
    #if defined(O_SYNC)
    SET_CONSTANT(O_SYNC);
    #endif
    #if defined(O_TMPFILE)
    SET_CONSTANT(O_TMPFILE);
    #endif
    #if defined(O_TRUNC)
    SET_CONSTANT(O_TRUNC);
    #endif
    #if defined(S_IFMT)
    SET_CONSTANT(S_IFMT);
    #endif
    #if defined(S_IFREG)
    SET_CONSTANT(S_IFREG);
    #endif
    #if defined(S_IFDIR)
    SET_CONSTANT(S_IFDIR);
    #endif
    #if defined(S_IFCHR)
    SET_CONSTANT(S_IFCHR);
    #endif
    #if defined(S_IFBLK)
    SET_CONSTANT(S_IFBLK);
    #endif
    #if defined(S_IFIFO)
    SET_CONSTANT(S_IFIFO);
    #endif
    #if defined(S_IFLNK)
    SET_CONSTANT(S_IFLNK);
    #endif
    #if defined(S_IFSOCK)
    SET_CONSTANT(S_IFSOCK);
    #endif
    #if defined(S_IRWXU)
    SET_CONSTANT(S_IRWXU);
    #endif
    #if defined(S_IRUSR)
    SET_CONSTANT(S_IRUSR);
    #endif
    #if defined(S_IWUSR)
    SET_CONSTANT(S_IWUSR);
    #endif
    #if defined(S_IXUSR)
    SET_CONSTANT(S_IXUSR);
    #endif
    #if defined(S_IRWXG)
    SET_CONSTANT(S_IRWXG);
    #endif
    #if defined(S_IRGRP)
    SET_CONSTANT(S_IRGRP);
    #endif
    #if defined(S_IWGRP)
    SET_CONSTANT(S_IWGRP);
    #endif
    #if defined(S_IXGRP)
    SET_CONSTANT(S_IXGRP);
    #endif
    #if defined(S_IRWXO)
    SET_CONSTANT(S_IRWXO);
    #endif
    #if defined(S_IROTH)
    SET_CONSTANT(S_IROTH);
    #endif
    #if defined(S_IWOTH)
    SET_CONSTANT(S_IWOTH);
    #endif
    #if defined(S_IXOTH)
    SET_CONSTANT(S_IXOTH);
    #endif
    #if defined(F_OK)
    SET_CONSTANT(F_OK);
    #endif
    #if defined(R_OK)
    SET_CONSTANT(R_OK);
    #endif
    #if defined(W_OK)
    SET_CONSTANT(W_OK);
    #endif
    #if defined(X_OK)
    SET_CONSTANT(X_OK);
    #endif

    return constants;
  }
  #undef SET_CONSTANT

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

	void Core::FS::RequestContext::setBuffer(char* base, uint32_t len) {
		this->buf.base = base;
		this->buf.len = len;
  }

  void Core::FS::RequestContext::freeBuffer() {
    delete[] static_cast<char*>(this->buf.base);
    this->buf.base = nullptr;
    this->buf.len = 0;
  }

  char* Core::FS::RequestContext::getBuffer () {
    return this->buf.base;
  }

  uint32_t Core::FS::RequestContext::getBufferSize () {
    return this->buf.len;
  }

  Core::FS::Descriptor::Descriptor (Core *core, uint64_t id) {
    this->core = core;
    this->id = id;
  }

  bool Core::FS::Descriptor::isDirectory () {
    Lock lock(this->mutex);
    return this->dir != nullptr;
  }

  bool Core::FS::Descriptor::isFile () {
    Lock lock(this->mutex);
    return this->fd > 0 && this->dir == nullptr;
  }

  bool Core::FS::Descriptor::isRetained () {
    return this->retained;
  }

  bool Core::FS::Descriptor::isStale () {
    return this->stale;
  }

  Core::FS::Descriptor * Core::FS::getDescriptor (uint64_t id) {
    Lock lock(this->mutex);
    if (descriptors.find(id) != descriptors.end()) {
      return descriptors.at(id);
    }
    return nullptr;
  }

  void Core::FS::removeDescriptor (uint64_t id) {
    Lock lock(this->mutex);
    if (descriptors.find(id) != descriptors.end()) {
      descriptors.erase(id);
    }
  }

  bool Core::FS::hasDescriptor (uint64_t id) {
    Lock lock(this->mutex);
    return descriptors.find(id) != descriptors.end();
  }

  void Core::FS::retainOpenDescriptor (
    const String seq,
    uint64_t id,
    Module::Callback cb
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

      return cb(seq, json, Post{});
    }

    desc->retained = true;
    auto json = JSON::Object::Entries {
      {"source", "fs.retainOpenDescriptor"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(desc->id)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::FS::access (
    const String seq,
    const String path,
    int mode,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_access(loop, req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"err", JSON::Object::Entries {
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.access"},
            {"data", JSON::Object::Entries {
              {"mode", req->flags},
            }}
          };
        }

        ctx->cb(ctx->seq, json, Post {});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::chmod (
    const String seq,
    const String path,
    int mode,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
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

        ctx->cb(ctx->seq, json, Post {});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

	void Core::FS::chown (
    const String seq,
    const String path,
    uv_uid_t uid,
    uv_gid_t gid,
    Module::Callback cb
  ) {
    core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, cb);
      auto uv_cb = [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post {});
        delete ctx;
      };

      auto err = uv_fs_chown(
          &core->eventLoop, &ctx->req, path.c_str(), uid, gid, uv_cb);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.chown"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->cb(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::lchown (
		const String seq,
    const String path,
    uv_uid_t uid,
    uv_gid_t gid,
    Module::Callback cb
  ) {
    core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, cb);
      auto uv_cb = [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post {});
        delete ctx;
      };

      auto err = uv_fs_lchown(
          &core->eventLoop, &ctx->req, path.c_str(), uid, gid, uv_cb);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.lchown"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->cb(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::close (
    const String seq,
    uint64_t id,
    Module::Callback cb
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

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_close(loop, req, desc->fd, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
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

          desc->core->fs.removeDescriptor(desc->id);
          delete desc;
        }

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::open (
    const String seq,
    uint64_t id,
    const String path,
    int flags,
    int mode,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto desc = new Descriptor(this->core, id);
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_open(loop, req, filename, flags, mode, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.open"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };

          delete desc;
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
          Lock lock(desc->core->fs.mutex);
          desc->core->fs.descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete desc;
        delete ctx;
      }
    });
  }

  void Core::FS::opendir (
    const String seq,
    uint64_t id,
    const String path,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto desc =  new Descriptor(this->core, id);
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_opendir(loop, req, filename, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(desc->id)},
              {"code", req->result},
              {"message", String(uv_strerror((int) req->result))}
            }}
          };

          delete desc;
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.opendir"},
            {"data", JSON::Object::Entries {
              {"id", std::to_string(desc->id)}
            }}
          };

          desc->dir = (uv_dir_t *) req->ptr;
          // insert into `descriptors` map
          Lock lock(desc->core->fs.mutex);
          desc->core->fs.descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete desc;
        delete ctx;
      }
    });
  }

  void Core::FS::readdir (
    const String seq,
    uint64_t id,
    size_t nentries,
    Module::Callback cb
  ) {
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

        return cb(seq, json, Post{});
      }

      if (!desc->isDirectory()) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readdir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"message", "No directory descriptor found with that id"}
          }}
        };

        return cb(seq, json, Post{});
      }

      Lock lock(desc->mutex);
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;

      desc->dir->dirents = ctx->dirents;
      desc->dir->nentries = nentries;

      auto err = uv_fs_readdir(loop, req, desc->dir, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
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
              {"name", desc->dir->dirents[i].name}
            };

            entries.push_back(entry);
          }

          json = JSON::Object::Entries {
            {"source", "fs.readdir"},
            {"data", entries}
          };
        }

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::closedir (
    const String seq,
    uint64_t id,
    Module::Callback cb
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

        return cb(seq, json, Post{});
      }

      if (!desc->isDirectory()) {
        auto json = JSON::Object::Entries {
          {"source", "fs.closedir"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"code", "ENOTOPEN"},
            {"message", "The descriptor found with was not a directory"}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_closedir(loop, req, desc->dir, [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
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

          desc->core->fs.removeDescriptor(desc->id);
          delete desc;
        }

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::closeOpenDescriptor (
    const String seq,
    uint64_t id,
    Module::Callback cb
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

      return cb(seq, json, Post{});
    }

    if (desc->isDirectory()) {
      this->closedir(seq, id, cb);
    } else if (desc->isFile()) {
      this->close(seq, id, cb);
    }
  }

  void Core::FS::closeOpenDescriptors (const String seq, Module::Callback cb) {
    return this->closeOpenDescriptors(seq, false, cb);
  }

  void Core::FS::closeOpenDescriptors (
    const String seq,
    bool preserveRetained,
    Module::Callback cb
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
        this->closedir(seq, id, [pending, cb](auto seq, auto json, auto post) {
          if (pending == 0) {
            cb(seq, json, post);
          }
        });
      } else if (desc->isFile()) {
        queued++;
        this->close(seq, id, [pending, cb](auto seq, auto json, auto post) {
          if (pending == 0) {
            cb(seq, json, post);
          }
        });
      }
    }

    if (queued == 0) {
      cb(seq, json, Post{});
    }
  }

  void Core::FS::read (
    const String seq,
    uint64_t id,
    size_t size,
    size_t offset,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
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

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;
      auto bytes = new char[size]{0};

      ctx->setBuffer(bytes, size);

      auto err = uv_fs_read(loop, req, desc->fd, &ctx->buf, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto desc = ctx->desc;
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

          auto bytes = ctx->getBuffer();
          if (bytes != nullptr) {
            delete [] bytes;
          }
        } else {
          auto headers = Headers {{
            {"content-type" ,"application/octet-stream"},
            {"content-length", req->result}
          }};

          post.id = SSC::rand64();
          post.body = ctx->getBuffer();
          post.length = (int) req->result;
          post.headers = headers.str();
        }

        ctx->cb(ctx->seq, json, post);
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

        ctx->cb(ctx->seq, json, Post{});
        delete [] bytes;
        delete ctx;
      }
    });
  }

  void Core::FS::watch (
    const String seq,
    uint64_t id,
    const String path,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
    #if defined(__ANDROID__)
      auto json = JSON::Object::Entries {
        {"source", "fs.watch"},
        {"err", JSON::Object::Entries {
          {"message", "Not supported"}
        }}
      };

      cb(seq, json, Post{});
      return;
    #else
      FileSystemWatcher* watcher;
      {
        Lock lock(this->mutex);
        watcher = this->watchers[id];
      }

      if (watcher == nullptr) {
        watcher = new FileSystemWatcher(path);
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

          cb("-1", json, Post{});
        });

        if (!started) {
          auto json = JSON::Object::Entries {
            {"source", "fs.watch"},
            {"err", JSON::Object::Entries {
              {"message", "Failed to start 'fs.Watcher'"}
            }}
          };

          cb(seq, json, Post{});
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

      cb(seq, json, Post{});
    #endif
    });
  }

  void Core::FS::write (
    const String seq,
    uint64_t id,
    char *bytes,
    size_t size,
    size_t offset,
    Module::Callback cb
  ) {
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

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;

      ctx->setBuffer(bytes, size);
      auto err = uv_fs_write(loop, req, desc->fd, &ctx->buf, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto desc = ctx->desc;
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::stat (
    const String seq,
    const String path,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_stat(loop, req, filename, [](uv_fs_t *req) {
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::stopWatch (
    const String seq,
    uint64_t id,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
    #if defined(__ANDROID__)
      auto json = JSON::Object::Entries {
        {"source", "fs.stopWatch"},
        {"err", JSON::Object::Entries {
          {"message", "Not supported"}
        }}
      };

      cb(seq, json, Post{});
      return;
    #else
      auto watcher = this->core->fs.watchers[id];
      if (watcher != nullptr) {
        watcher->stop();
        delete watcher;
        this->core->fs.watchers.erase(id);
        auto json = JSON::Object::Entries {
          {"source", "fs.stopWatch"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
          }}
        };
        cb(seq, json, Post{});
      } else {
        auto json = JSON::Object::Entries {
          {"source", "fs.stat"},
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "fs.Watcher does not exist"}
          }}
        };

        cb(seq, json, Post{});
      }
    #endif
    });
  }

  void Core::FS::fsync (
    const String seq,
    uint64_t id,
    Module::Callback cb
  ) {
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

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::ftruncate (
    const String seq,
    uint64_t id,
    int64_t offset,
    Module::Callback cb
  ) {
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

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::fstat (
    const String seq,
    uint64_t id,
    Module::Callback cb
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

        return cb(seq, json, Post{});
      }

      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(desc, seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_fstat(loop, req, desc->fd, [](uv_fs_t *req) {
        auto ctx = (RequestContext *) req->data;
        auto desc = ctx->desc;
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::getOpenDescriptors (
    const String seq,
    Module::Callback cb
  ) {
    Lock lock(this->mutex);
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

    cb(seq, json, Post{});
  }

  void Core::FS::lstat (
    const String seq,
    const String path,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
      auto req = &ctx->req;
      auto err = uv_fs_lstat(loop, req, filename, [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

	void Core::FS::link (
    const String seq,
    const String src,
    const String dest,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, cb);
      auto uv_cb = [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post {});
        delete ctx;
      };

      auto err = uv_fs_link(
        &core->eventLoop, &ctx->req, src.c_str(), dest.c_str(), uv_cb);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.link"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->cb(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::symlink (
    const String seq,
    const String src,
    const String dest,
    int flags,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, cb);
      auto uv_cb = [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post {});
        delete ctx;
      };

      auto err = uv_fs_symlink(
        &core->eventLoop, &ctx->req, src.c_str(), dest.c_str(), flags, uv_cb);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.symlink"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->cb(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::unlink (
    const String seq,
    const String path,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

	void Core::FS::readlink (
    const String seq,
    const String path,
    const Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, cb);
      auto uv_cb = [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post {});
        delete ctx;
      };

      auto err = uv_fs_readlink(
          &core->eventLoop, &ctx->req, path.c_str(), uv_cb);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.readlink"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->cb(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::realpath (
    const String seq,
    const String path,
    const Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto ctx = new RequestContext(seq, cb);
      auto uv_cb = [](uv_fs_t* req) {
        auto ctx = static_cast<RequestContext*>(req->data);
        auto json = JSON::Object{};

        if (uv_fs_get_result(req) < 0) {
          json = JSON::Object::Entries {
            {"source", "fs.realpath"},
            {"err", JSON::Object::Entries {
              {"code", uv_fs_get_result(req)},
              {"message", String(uv_strerror(uv_fs_get_result(req)))}
            }}
          };
        } else {
          json = JSON::Object::Entries {
            {"source", "fs.realpath"},
            {"data", JSON::Object::Entries {
              {"path", String((char*) uv_fs_get_ptr(req))}
            }}
          };
        }

        ctx->cb(ctx->seq, json, Post {});
        delete ctx;
      };

      auto err = uv_fs_realpath(
          &core->eventLoop, &ctx->req, path.c_str(), uv_cb);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "fs.realpath"},
          {"err", JSON::Object::Entries {
            {"code", err},
            {"message", String(uv_strerror(err))}
          }}
        };
        ctx->cb(seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::rename (
    const String seq,
    const String pathA,
    const String pathB,
    const Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::copyFile (
    const String seq,
    const String pathA,
    const String pathB,
    int flags,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
      auto req = &ctx->req;
      auto src = pathA.c_str();
      auto dst = pathB.c_str();
      auto err = uv_fs_copyfile(loop, req, src, dst, flags, [](uv_fs_t* req) {
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::rmdir (
    const String seq,
    const String path,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::mkdir (
    const String seq,
    const String path,
    int mode,
    bool recursive,
    Module::Callback cb
  ) {
    this->core->dispatchEventLoop([=, this]() {
      int err = 0;
      auto filename = path.c_str();
      auto loop = &this->core->eventLoop;
      auto ctx = new RequestContext(seq, cb);
      auto req = &ctx->req;
      const auto callback = [](uv_fs_t* req) {
        auto ctx = (RequestContext *) req->data;
        auto json = JSON::Object {};

        if (uv_fs_get_result(req) < 0) {
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

        ctx->cb(ctx->seq, json, Post{});
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

        ctx->cb(ctx->seq, json, Post{});
        delete ctx;
      }
    });
  }

  void Core::FS::constants (const String seq, Module::Callback cb) {
    static auto constants = getFSConstantsMap();
    static auto data = JSON::Object {constants};
    static auto json = JSON::Object::Entries {
      {"source", "fs.constants"},
      {"data", data}
    };

    static auto headers = Headers {{
      Headers::Header {"Cache-Control", "public, max-age=86400"}
    }};

    static auto post = Post {
      .id = 0,
      .ttl = 0,
      .body = nullptr,
      .length = 0,
      .headers = headers.str()
    };

    cb(seq, json, post);
  }
}
