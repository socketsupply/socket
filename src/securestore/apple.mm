#import <Foundation/Foundation.h>
#import <Security/Security.h>

namespace SSC {
  char* SecureStore::get (const String& account, const String& service) {
    NSDictionary *query = @{
      (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
      (__bridge id)kSecAttrAccount: @(account),
      (__bridge id)kSecAttrService: @(service),
      (__bridge id)kSecReturnData: @YES
    };

    CFTypeRef result = NULL;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);

    if (status == errSecSuccess) {
      NSData *passwordData = (__bridge_transfer NSData *)result;
      char *passwordBuffer = (char *)malloc([passwordData length] + 1);

      [passwordData getBytes:passwordBuffer length:[passwordData length]];

      passwordBuffer[[passwordData length]] = '\0';

      return passwordBuffer;
    } else {
      NSLog(@"Error: Unable to retrieve password. Status: %d", (int)status);
      return nullptr;
    }
  }

  bool SecureStore::set (const String& account, const String& service, const char* value, int len) {
    NSDictionary *query = @{
      (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
      (__bridge id)kSecAttrAccount: @(account),
      (__bridge id)kSecAttrService: @(service),
      (__bridge id)kSecValueData: [NSData dataWithBytes:value length:len]
    };

    OSStatus status = SecItemAdd((__bridge CFDictionaryRef)query, NULL);
    return status == errSecSuccess;
  }
}
