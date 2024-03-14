#include <windows.h>
#include <wincred.h>
#include <string>

#pragma comment(lib, "advapi32.lib")

namespace SSC {
  char* SecureStore::get (const String& account, const String& service) {
    PCREDENTIALA cred;

    if (CredReadA(service.c_str(), CRED_TYPE_GENERIC, 0, &cred)) {
      char* passwordBuffer = (char*)malloc(cred->CredentialBlobSize + 1);
      memcpy(passwordBuffer, cred->CredentialBlob, cred->CredentialBlobSize);
      passwordBuffer[cred->CredentialBlobSize] = '\0';
      CredFree(cred);
      return passwordBuffer;
    } else {
      return nullptr;
    }
  }

  bool SecureStore::set (const String& account, const String& service, const char* value, int len) {
    CREDENTIALA cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = (LPSTR)service.c_str();
    cred.CredentialBlobSize = len;
    cred.CredentialBlob = (LPBYTE)value;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    return CredWriteA(&cred, 0);
  }
}
