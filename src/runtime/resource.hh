#ifndef SOCKET_RUNTIME_RESOURCE_H
#define SOCKET_RUNTIME_RESOURCE_H

#include "platform.hh"
#include "debug.hh"
#include "url.hh"

namespace ssc::runtime {
  class Resource {
    public:
      Atomic<bool> accessing = false;
      String name;
      String type;
      debug::Tracer tracer;
      Resource (const String& type, const String& name)
        : name(name),
        type(type),
        tracer(name)
      {}
      virtual ~Resource () {}
      virtual bool hasAccess () const noexcept {
        return this->accessing;
      }
      virtual bool startAccessing () = 0;
      virtual bool stopAccessing () = 0;
  };
}
#endif
