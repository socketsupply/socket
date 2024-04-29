#ifndef SSC_SECIRE_STORE_H
#define SSC_SECURE_STORE_H

namespace SSC {
  class SecureStore {
    public:
      SecureStore() {}
      static char* get(const String& account, const String& service);
      static bool set (const String& account, const String& service, const char* bytes, int size);
  }
}

#endif
