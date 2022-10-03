#include "core.hh"

namespace SSC {
  void DescriptorRequestContext::setBuffer (int index, int len, char *base) {
    this->iov[index].base = base;
    this->iov[index].len = len;
  }

  void DescriptorRequestContext::freeBuffer (int index) {
    if (this->iov[index].base != nullptr) {
      delete [] (char *) this->iov[index].base;
      this->iov[index].base = nullptr;
    }

    this->iov[index].len = 0;
  }

  char* DescriptorRequestContext::getBuffer (int index) {
    return this->iov[index].base;
  }

  size_t DescriptorRequestContext::getBufferSize (int index) {
    return this->iov[index].len;
  }

  void DescriptorRequestContext::end (String seq, String msg, Post post) {
    auto cb = this->cb;

    if (cb != nullptr) {
      cb(seq, msg, post);
    }

    delete this;
  }

  void DescriptorRequestContext::end (String seq, String msg) {
    this->end(seq, msg, Post{});
  }

  void DescriptorRequestContext::end (String msg, Post post) {
    this->end(this->seq, msg, post);
  }

  void DescriptorRequestContext::end (String msg) {
    this->end(this->seq, msg, Post{});
  }

  Descriptor::Descriptor (Core *core, uint64_t id) {
    this->core = core;
    this->id = id;
  }

  bool Descriptor::isDirectory () {
    std::lock_guard<std::recursive_mutex> guard(this->mutex);
    return this->dir != nullptr;
  }

  bool Descriptor::isFile () {
    std::lock_guard<std::recursive_mutex> guard(this->mutex);
    return this->fd > 0 && this->dir == nullptr;
  }

  bool Descriptor::isRetained () {
    std::lock_guard<std::recursive_mutex> guard(this->mutex);
    return this->retained;
  }

  bool Descriptor::isStale () {
    std::lock_guard<std::recursive_mutex> guard(this->mutex);
    return this->stale;
  }

  Descriptor * Core::getDescriptor (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
    if (descriptors.find(id) != descriptors.end()) {
      return descriptors.at(id);
    }
    return nullptr;
  }

  void Core::removeDescriptor (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
    if (descriptors.find(id) != descriptors.end()) {
      descriptors.erase(id);
    }
  }

  bool Core::hasDescriptor (uint64_t id) {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
    return descriptors.find(id) != descriptors.end();
  }

  void Core::fsRetainOpenDescriptor (String seq, uint64_t id, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(seq, cb);
    SSC::String msg;

    if (desc == nullptr) {
      msg = SSC::format(R"MSG({
        "source": "fs.retainOpenDescriptor",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));
    } else {
      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      desc->retained = true;
      auto msg = SSC::format(R"MSG({
        "source": "fs.retainOpenDescriptor",
        "data": {
          "id": "$S"
        }
      })MSG", std::to_string(desc->id));
    }

    ctx->end(msg);
  }

  void Core::fsAccess (String seq, String path, int mode, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_access(&this->eventLoop, &ctx->req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.access",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.access",
            "data": {
              "mode": $S
            }
          })MSG", std::to_string(req->flags));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.access",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG", std::to_string(err), String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsChmod (String seq, String path, int mode, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_chmod(&this->eventLoop, &ctx->req, filename, mode, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.chmod",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.chmod",
            "data": {
              "mode": "$S"
            }
          })MSG", std::to_string(req->flags));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.chmod",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG", std::to_string(err), String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsOpen (String seq, uint64_t id, String path, int flags, int mode, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto desc = new Descriptor(this, id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);

      auto err = uv_fs_open(&this->eventLoop, &ctx->req, filename, flags, mode, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.open",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));

          delete desc;
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.open",
            "data": {
              "id": "$S",
              "fd": $S
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result));

          desc->fd = (int) req->result;
          // insert into `descriptors` map
          std::lock_guard<std::recursive_mutex> guard(desc->core->descriptorsMutex);
          desc->core->descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.open",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        delete desc;
        ctx->end(msg);
      }
    });
  }

  void Core::fsOpendir(String seq, uint64_t id, String path, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto desc =  new Descriptor(this, id);
      auto ctx = new DescriptorRequestContext(desc, seq, cb);
      auto err = uv_fs_opendir(&this->eventLoop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.opendir",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));

          delete desc;
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.opendir",
            "data": {
              "id": "$S"
            }
          })MSG",
          std::to_string(desc->id));

          desc->dir = (uv_dir_t *) req->ptr;
          // insert into `descriptors` map
          std::lock_guard<std::recursive_mutex> guard(desc->core->descriptorsMutex);
          desc->core->descriptors.insert_or_assign(desc->id, desc);
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.opendir",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        std::to_string(err),
        String(uv_strerror(err)));

        delete desc;
        ctx->end(msg);
      }
    });
  }

  void Core::fsReaddir (String seq, uint64_t id, size_t nentries, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(desc, seq, cb);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.readdir",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No directory descriptor found with that id"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    if (!desc->isDirectory()) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.readdir",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "message": "Directory descriptor with that id is not open"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    dispatchEventLoop([=, this]() {
      std::lock_guard<std::recursive_mutex> descriptorLock(desc->mutex);
      desc->dir->dirents = ctx->dirents;
      desc->dir->nentries = nentries;

      auto err = uv_fs_readdir(&this->eventLoop, &ctx->req, desc->dir, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.readdir",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          SSC::StringStream entries;
          entries << "[";

          for (int i = 0; i < req->result; ++i) {
            entries << "{";
            entries << "\"type\":" << std::to_string(desc->dir->dirents[i].type) << ",";
            entries << "\"name\":" << "\"" << desc->dir->dirents[i].name << "\"";
            entries << "}";

            if (i + 1 < req->result) {
              entries << ", ";
            }
          }

          entries << "]";

          msg = SSC::format(R"MSG({
            "source": "fs.readdir",
            "data": {
              "id": "$S",
              "entries": $S
            }
          })MSG",
          std::to_string(desc->id),
          entries.str());
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.readdir",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsClose (String seq, uint64_t id, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(desc, seq, cb);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.close",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    dispatchEventLoop([ctx, desc, this]() {
      auto err = uv_fs_close(&this->eventLoop, &ctx->req, desc->fd, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.close",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.close",
            "data": {
              "id": "$S",
              "fd": $S
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(desc->fd));

          desc->core->removeDescriptor(desc->id);
          delete desc;
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.close",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(desc->id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsClosedir (String seq, uint64_t id, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(desc, seq, cb);
    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.closedir",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No directory descriptor found with that id"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    dispatchEventLoop([=, this]() {
      auto err = uv_fs_closedir(&this->eventLoop, &ctx->req, desc->dir, [](uv_fs_t* req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.closedir",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.closedir",
            "data": {
              "id": "$S"
            }
          })MSG", std::to_string(desc->id));

          desc->core->removeDescriptor(desc->id);
          delete desc;
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.closedir",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsCloseOpenDescriptor (String seq, uint64_t id, Callback cb) {
    auto desc = getDescriptor(id);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.closeOpenDescriptor",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No descriptor found with that id"
        }
      })MSG", std::to_string(id));

      cb(seq, msg, Post{});
      return;
    }

    if (desc->isDirectory()) {
      this->fsClosedir(seq, id, cb);
    } else if (desc->isFile()) {
      this->fsClose(seq, id, cb);
    }
  }

  void Core::fsCloseOpenDescriptors (String seq, Callback cb) {
    return this->fsCloseOpenDescriptors(seq, false, cb);
  }

  void Core::fsCloseOpenDescriptors (String seq, bool preserveRetained, Callback cb) {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);

    std::vector<uint64_t> ids;
    SSC::String msg = "";
    int pending = descriptors.size();
    int queued = 0;

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
        this->fsClosedir(seq, id, [pending, cb](auto seq, auto msg, auto post) {
          if (pending == 0) {
            cb(seq, msg, post);
          }
        });
      } else if (desc->isFile()) {
        queued++;
        this->fsClose(seq, id, [pending, cb](auto seq, auto msg, auto post) {
          if (pending == 0) {
            cb(seq, msg, post);
          }
        });
      }
    }

    if (queued == 0) {
      cb(seq, msg, Post{});
    }
  }

  void Core::fsRead (String seq, uint64_t id, int len, int offset, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(desc, seq, cb);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.read",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    dispatchEventLoop([=, this]() {
      auto buf = new char[len]{0};
      ctx->setBuffer(0, len, buf);

      auto err = uv_fs_read(&this->eventLoop, &ctx->req, desc->fd, ctx->iov, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<DescriptorRequestContext*>(req->data);
        auto desc = ctx->desc;
        SSC::String msg = "{}";
        Post post = {0};

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.read",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));

          auto buf = ctx->getBuffer(0);
          if (buf != nullptr) {
            delete [] buf;
          }
        } else {
          post.id = SSC::rand64();
          post.body = ctx->getBuffer(0);
          post.length = (int) req->result;
          post.bodyNeedsFree = true;
        }

        ctx->end(msg, post);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.read",
          "err": {
            "id": "$S",
            "message": "$S"
          }
        })MSG", std::to_string(id), String(uv_strerror(err)));

        delete [] buf;
        ctx->end(msg);
      }
    });
  }

  void Core::fsWrite (String seq, uint64_t id, String data, int64_t offset, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(desc, seq, cb);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "source": "fs.write",
        "err": {
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    auto size = data.size();
    auto bytes = new char[size > 0 ? size : 1]{0};
    memcpy(bytes, data.data(), size);

    ctx->setBuffer(0, size, bytes);
    dispatchEventLoop([=, this]() {
      auto err = uv_fs_write(&this->eventLoop, &ctx->req, desc->fd, ctx->iov, 1, offset, [](uv_fs_t* req) {
        auto ctx = static_cast<DescriptorRequestContext*>(req->data);
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.write",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.write",
            "data": {
              "id": "$S",
              "result": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result));
        }

        auto bytes = ctx->getBuffer(0);
        if (bytes != nullptr) {
          delete [] bytes;
        }

        ctx->end(msg);

      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.write",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        delete [] bytes;
        ctx->end(msg);
      }
    });
  }

  void Core::fsStat (String seq, String path, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_stat(&this->eventLoop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.stat",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
            auto stats = uv_fs_get_statbuf(req);
            msg = SSC::format(R"MSG({
              "source": "fs.stat",
              "data": {
                "st_dev": "$S",
                "st_mode": "$S",
                "st_nlink": "$S",
                "st_uid": "$S",
                "st_gid": "$S",
                "st_rdev": "$S",
                "st_ino": "$S",
                "st_size": "$S",
                "st_blksize": "$S",
                "st_blocks": "$S",
                "st_flags": "$S",
                "st_gen": "$S",
                "st_atim": { "tv_sec": "$S", "tv_nsec": "$S" },
                "st_mtim": { "tv_sec": "$S", "tv_nsec": "$S" },
                "st_ctim": { "tv_sec": "$S", "tv_nsec": "$S" },
                "st_birthtim": { "tv_sec": "$S", "tv_nsec": "$S" }
              }
            })MSG",
            std::to_string(stats->st_dev),
            std::to_string(stats->st_mode),
            std::to_string(stats->st_nlink),
            std::to_string(stats->st_uid),
            std::to_string(stats->st_gid),
            std::to_string(stats->st_rdev),
            std::to_string(stats->st_ino),
            std::to_string(stats->st_size),
            std::to_string(stats->st_blksize),
            std::to_string(stats->st_blocks),
            std::to_string(stats->st_flags),
            std::to_string(stats->st_gen),
            std::to_string(stats->st_atim.tv_sec),
            std::to_string(stats->st_atim.tv_nsec),
            std::to_string(stats->st_mtim.tv_sec),
            std::to_string(stats->st_mtim.tv_nsec),
            std::to_string(stats->st_ctim.tv_sec),
            std::to_string(stats->st_ctim.tv_nsec),
            std::to_string(stats->st_birthtim.tv_sec),
            std::to_string(stats->st_birthtim.tv_nsec)
          );
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.stat",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsFStat (String seq, uint64_t id, Callback cb) {
    auto desc = getDescriptor(id);
    auto ctx = new DescriptorRequestContext(desc, seq, cb);

    if (desc == nullptr) {
      auto msg = SSC::format(R"MSG({
        "err": {
        "source": "fs.read",
          "id": "$S",
          "code": "ENOTOPEN",
          "type": "NotFoundError",
          "message": "No file descriptor found with that id"
        }
      })MSG", std::to_string(id));

      ctx->end(msg);
      return;
    }

    dispatchEventLoop([=, this]() {
      auto err = uv_fs_fstat(&this->eventLoop, &ctx->req, desc->fd, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        auto desc = ctx->desc;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.fstat",
            "err": {
              "id": "$S",
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          auto stats = uv_fs_get_statbuf(req);
          msg = SSC::trim(SSC::format(R"MSG({
            "source": "fs.fstat",
            "data": {
              "id": "$S",
              "st_dev": "$S",
              "st_mode": "$S",
              "st_nlink": "$S",
              "st_uid": "$S",
              "st_gid": "$S",
              "st_rdev": "$S",
              "st_ino": "$S",
              "st_size": "$S",
              "st_blksize": "$S",
              "st_blocks": "$S",
              "st_flags": "$S",
              "st_gen": "$S",
              "st_atim": { "tv_sec": "$S", "tv_nsec": "$S" },
              "st_mtim": { "tv_sec": "$S", "tv_nsec": "$S" },
              "st_ctim": { "tv_sec": "$S", "tv_nsec": "$S" },
              "st_birthtim": { "tv_sec": "$S", "tv_nsec": "$S" }
            }
          })MSG",
          std::to_string(desc->id),
          std::to_string(stats->st_dev),
          std::to_string(stats->st_mode),
          std::to_string(stats->st_nlink),
          std::to_string(stats->st_uid),
          std::to_string(stats->st_gid),
          std::to_string(stats->st_rdev),
          std::to_string(stats->st_ino),
          std::to_string(stats->st_size),
          std::to_string(stats->st_blksize),
          std::to_string(stats->st_blocks),
          std::to_string(stats->st_flags),
          std::to_string(stats->st_gen),
          std::to_string(stats->st_atim.tv_sec),
          std::to_string(stats->st_atim.tv_nsec),
          std::to_string(stats->st_mtim.tv_sec),
          std::to_string(stats->st_mtim.tv_nsec),
          std::to_string(stats->st_ctim.tv_sec),
          std::to_string(stats->st_ctim.tv_nsec),
          std::to_string(stats->st_birthtim.tv_sec),
          std::to_string(stats->st_birthtim.tv_nsec)));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.fstat",
          "err": {
            "id": "$S",
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(id),
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsGetOpenDescriptors (String seq, Callback cb) {
    std::lock_guard<std::recursive_mutex> guard(descriptorsMutex);
    int pending = descriptors.size();
    SSC::String msg = "{\n"
      "  \"source\": \"fs.getOpenDescriptors\",\n"
      "  \"data\": [";

    for (auto const &tuple : descriptors) {
      auto desc = tuple.second;
      if (!desc) {
        continue;
      }

      if (desc->isStale() && !desc->isRetained()) {
        continue;
      }

      msg += SSC::format(R"MSG({
        "id": "$S",
        "fd": "$S",
        "type": "$S"
      })MSG",
      std::to_string(desc->id),
      std::to_string(desc->isDirectory() ? desc->id : desc->fd),
      desc->dir ? "directory" : "file");

      if (--pending > 0) {
        msg += ",\n";
      }
    }
    msg += "]\n}";

    cb(seq, msg, Post{});
  }

  void Core::fsUnlink (String seq, String path, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_unlink(&this->eventLoop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.unlink",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.unlink",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.unlink",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsRename (String seq, String pathA, String pathB, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto ctx = new DescriptorRequestContext(seq, cb);
      auto src = pathA.c_str();
      auto dst = pathB.c_str();

      auto err = uv_fs_rename(&this->eventLoop, &ctx->req, src, dst, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.rename",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror((int)req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.rename",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.rename",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsCopyFile (String seq, String pathA, String pathB, int flags, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto ctx = new DescriptorRequestContext(seq, cb);
      auto src = pathA.c_str();
      auto dst = pathB.c_str();

      auto err = uv_fs_copyfile(&this->eventLoop, &ctx->req, src, dst, flags, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.copyFile",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror((int)req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.copyFile",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.copyFile",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsRmdir (String seq, String path, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_rmdir(&this->eventLoop, &ctx->req, filename, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.rmdir",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.rmdir",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.rmdir",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  void Core::fsMkdir (String seq, String path, int mode, Callback cb) {
    dispatchEventLoop([=, this]() {
      auto filename = path.c_str();
      auto ctx = new DescriptorRequestContext(seq, cb);

      auto err = uv_fs_mkdir(&this->eventLoop, &ctx->req, filename, mode, [](uv_fs_t *req) {
        auto ctx = (DescriptorRequestContext *) req->data;
        SSC::String msg;

        if (req->result < 0) {
          msg = SSC::format(R"MSG({
            "source": "fs.mkdir",
            "err": {
              "code": $S,
              "message": "$S"
            }
          })MSG",
          std::to_string(req->result),
          String(uv_strerror(req->result)));
        } else {
          msg = SSC::format(R"MSG({
            "source": "fs.mkdir",
            "data": {
              "result": "$S"
            }
          })MSG", std::to_string(req->result));
        }

        ctx->end(msg);
      });

      if (err < 0) {
        auto msg = SSC::format(R"MSG({
          "source": "fs.mkdir",
          "err": {
            "code": $S,
            "message": "$S"
          }
        })MSG",
        std::to_string(err),
        String(uv_strerror(err)));

        ctx->end(msg);
      }
    });
  }

  String Core::getFSConstants () {
    auto constants = Core::getFSConstantsMap();
    auto count = constants.size();
    SSC::StringStream stream;

    stream << "{\"source\": \"fs.constants\",";
    stream << "\"data\": {";

    for (auto const &tuple : constants) {
      auto key = tuple.first;
      auto value = tuple.second;

      stream << "\"" << key << "\":" << value;

      if (--count > 0) {
        stream << ",";
      }
    }

    stream << "}";
    stream << "}";

    return stream.str();
  }

#define SET_CONSTANT(c) constants[#c] = std::to_string(c);
  SSC::Map Core::getFSConstantsMap () {
    SSC::Map constants;

#ifdef UV_DIRENT_UNKNOWN
    SET_CONSTANT(UV_DIRENT_UNKNOWN)
#endif

#ifdef UV_DIRENT_FILE
    SET_CONSTANT(UV_DIRENT_FILE)
#endif

#ifdef UV_DIRENT_DIR
    SET_CONSTANT(UV_DIRENT_DIR)
#endif

#ifdef UV_DIRENT_LINK
    SET_CONSTANT(UV_DIRENT_LINK)
#endif

#ifdef UV_DIRENT_FIFO
    SET_CONSTANT(UV_DIRENT_FIFO)
#endif

#ifdef UV_DIRENT_SOCKET
    SET_CONSTANT(UV_DIRENT_SOCKET)
#endif

#ifdef UV_DIRENT_CHAR
    SET_CONSTANT(UV_DIRENT_CHAR)
#endif

#ifdef UV_DIRENT_BLOCK
    SET_CONSTANT(UV_DIRENT_BLOCK)
#endif

#ifdef O_RDONLY
      SET_CONSTANT(O_RDONLY);
#endif

#ifdef O_WRONLY
    SET_CONSTANT(O_WRONLY);
#endif

#ifdef O_RDWR
    SET_CONSTANT(O_RDWR);
#endif

#ifdef O_APPEND
    SET_CONSTANT(O_APPEND);
#endif

#ifdef O_ASYNC
    SET_CONSTANT(O_ASYNC);
#endif

#ifdef O_CLOEXEC
    SET_CONSTANT(O_CLOEXEC);
#endif

#ifdef O_CREAT
    SET_CONSTANT(O_CREAT);
#endif

#ifdef O_DIRECT
    SET_CONSTANT(O_DIRECT);
#endif

#ifdef O_DIRECTORY
    SET_CONSTANT(O_DIRECTORY);
#endif

#ifdef O_DSYNC
    SET_CONSTANT(O_DSYNC);
#endif

#ifdef O_EXCL
    SET_CONSTANT(O_EXCL);
#endif

#ifdef O_LARGEFILE
    SET_CONSTANT(O_LARGEFILE);
#endif

#ifdef O_NOATIME
    SET_CONSTANT(O_NOATIME);
#endif

#ifdef O_NOCTTY
    SET_CONSTANT(O_NOCTTY);
#endif

#ifdef O_NOFOLLOW
    SET_CONSTANT(O_NOFOLLOW);
#endif

#ifdef O_NONBLOCK
    SET_CONSTANT(O_NONBLOCK);
#endif

#ifdef O_NDELAY
    SET_CONSTANT(O_NDELAY);
#endif

#ifdef O_PATH
    SET_CONSTANT(O_PATH);
#endif

#ifdef O_SYNC
    SET_CONSTANT(O_SYNC);
#endif

#ifdef O_TMPFILE
    SET_CONSTANT(O_TMPFILE);
#endif

#ifdef O_TRUNC
    SET_CONSTANT(O_TRUNC);
#endif

#ifdef S_IFMT
    SET_CONSTANT(S_IFMT);
#endif

#ifdef S_IFREG
    SET_CONSTANT(S_IFREG);
#endif

#ifdef S_IFDIR
    SET_CONSTANT(S_IFDIR);
#endif

#ifdef S_IFCHR
    SET_CONSTANT(S_IFCHR);
#endif

#ifdef S_IFBLK
    SET_CONSTANT(S_IFBLK);
#endif

#ifdef S_IFIFO
    SET_CONSTANT(S_IFIFO);
#endif

#ifdef S_IFLNK
    SET_CONSTANT(S_IFLNK);
#endif

#ifdef S_IFSOCK
    SET_CONSTANT(S_IFSOCK);
#endif

#ifdef S_IRWXU
    SET_CONSTANT(S_IRWXU);
#endif

#ifdef S_IRUSR
    SET_CONSTANT(S_IRUSR);
#endif

#ifdef S_IWUSR
    SET_CONSTANT(S_IWUSR);
#endif

#ifdef S_IXUSR
    SET_CONSTANT(S_IXUSR);
#endif

#ifdef S_IRWXG
    SET_CONSTANT(S_IRWXG);
#endif

#ifdef S_IRGRP
    SET_CONSTANT(S_IRGRP);
#endif

#ifdef S_IWGRP
    SET_CONSTANT(S_IWGRP);
#endif

#ifdef S_IXGRP
    SET_CONSTANT(S_IXGRP);
#endif

#ifdef S_IRWXO
    SET_CONSTANT(S_IRWXO);
#endif

#ifdef S_IROTH
    SET_CONSTANT(S_IROTH);
#endif

#ifdef S_IWOTH
    SET_CONSTANT(S_IWOTH);
#endif

#ifdef S_IXOTH
    SET_CONSTANT(S_IXOTH);
#endif

#ifdef F_OK
    SET_CONSTANT(F_OK);
#endif

#ifdef R_OK
    SET_CONSTANT(R_OK);
#endif

#ifdef W_OK
    SET_CONSTANT(W_OK);
#endif

#ifdef X_OK
    SET_CONSTANT(X_OK);
#endif

  }
    return constants;
#undef SET_CONSTANT
}
