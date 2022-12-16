#include <stdint.h>

namespace SSC {
  typedef char ConfigString[1024];
  typedef int32_t ConfigInt;
  typedef bool ConfigBool;

  struct Config {
    struct {
      ConfigString cmd;
      ConfigString icon;
      ConfigString logo;
      ConfigString pfx;
      ConfigString publisher;
    } win;

    struct {
      ConfigString categories;
      ConfigString cmd;
      ConfigString icon;
    } linux;

    struct {
      ConfigString icon;
      ConfigString codesign_identity;
      ConfigString distribution_method;
      ConfigString provisioning_profile;
      ConfigString simulator_device;
    } ios;

    struct {
      ConfigString cmd;
      ConfigString icon;
      ConfigString appstore_icon;
      ConfigString appstore_category;
      ConfigString codesign_identity;
      ConfigString sign;
      ConfigString sign_paths;
    } mac;

    ConfigString bundle_identifier;
    ConfigString version;
    ConfigString revision;
    ConfigString copyright;
    ConfigString description;
    ConfigString name;
    ConfigString maintainer;
    ConfigString lang;
    ConfigString env;

    ConfigString build;
    ConfigString input;
    ConfigString output;
    ConfigString executable;
    ConfigBool headless;
    ConfigString flags;

    ConfigInt file_limit;

    struct {
      ConfigString flags;
    } debug;

    struct {
      ConfigString height;
      ConfigString width;
    } window;
  };
}
