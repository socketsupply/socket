#ifndef SOCKET_RUNTIME_RUNTIME_CONFIG_H
#define SOCKET_RUNTIME_RUNTIME_CONFIG_H

#include <iterator>

#ifndef SOCKET_RUNTIME_VERSION
#define SOCKET_RUNTIME_VERSION ""
#endif

#ifndef SOCKET_RUNTIME_VERSION_HASH
#define SOCKET_RUNTIME_VERSION_HASH ""
#endif

// TODO(@jwerle): use a better name
#if !defined(SOCKET_RUNTIME_PLATFORM_SANDBOXED)
#define SOCKET_RUNTIME_PLATFORM_SANDBOXED 0
#endif

// TODO(@jwerle): stop using this and prefer a namespaced macro
#ifndef DEBUG
#define DEBUG 0
#endif

// TODO(@jwerle): stop using this and prefer a namespaced macro
#ifndef HOST
#define HOST "localhost"
#endif

// TODO(@jwerle): stop using this and prefer a namespaced macro
#ifndef PORT
#define PORT 0
#endif

#if defined(__cplusplus)
#include "platform.hh"

extern "C" {
  // implemented in `init.cc`
  extern const unsigned char* socket_runtime_init_get_user_config_bytes ();
  extern unsigned int socket_runtime_init_get_user_config_bytes_size ();
  extern bool socket_runtime_init_is_debug_enabled ();
  extern const char* socket_runtime_init_get_dev_host ();
  extern int socket_runtime_init_get_dev_port ();
}

namespace ssc::runtime::config {
  const Map<String, String> getUserConfig ();
  bool isDebugEnabled ();
  const String getDevHost ();
  int getDevPort ();

  /**
   * A container for configuration that can be mutated and queried using
   * `.` syntax. Configuration can be created from an INI source string.
   */
  class Config {
    /**
     * Internal configuration mapping, exposed as a const reference to the
     * caller in `Config::data()`
     */
    Map<String, String> map;
    public:
      using Iterator = Map<String, String>::const_iterator;
      using Path = Vector<String>;

      /**
       * The configuration prefix.
       */
      const String prefix;

      /**
       * `Config` class constructors.
       */
      Config () = default;
      Config (const String& source);
      Config (const Map<String, String>& source);
      Config (const Config& source);
      Config (const String& prefix, const Map<String, String>& source);
      Config (const String& prefix, const Config& source);

      /**
       * Get a configuration value by name or `.` path.
       *
       * @param key The configuration name or key path
       * @return The value at `key` or an empty string.
       */
      const String get (const String& key) const noexcept;

      /**
       * Get a configuration value reference by name.
       *
       * @param key The configuration name or key path
       * @return `String&` The reference at `key` or an empty string.
       */
      const String& at (const String& key) const;

      /**
       * Set a configuration `value` by `key`.
       *
       * @param key The configuration name of `value` to set
       * @param value The value of `key` to set
       */
      void set (const String& key, const String& value) noexcept;

      /**
       * Returns `true` if `key` exists in configuration and is not empty.
       *
       * @param key The key to check for existence.
       * @return `true` if it exists, otherwise `false`
       */
      bool contains (const String& key) const noexcept;

      /**
       * Erase a configuration value by `key`.
       *
       * @param key The key to erase a value for. Can be valid input for `query`
       * @return `true` if erased, otherwise `false`
       */
      bool erase (const String& key) noexcept;

      /**
       * Get a const reference to the underlying data map
       * of this configuration.
       *
       * @return `Map&` A reference to the internal data map
       */
      const Map<String, String>& data () const noexcept;

      /**
       * Get a `Config` instance as a "slice" of this configuration, such as
       * a subsection using `.` syntax or configuration section prefixes. The
       * returned `Config` instance is read-only.
       *
       * @param key The key to filter on
       * @return A `const Config` with section starting or matching `key`
       *
       * @example
       *   const auto config = Config::getUserConfig();
       *   const auto build = config.slice("build");
       *   const auto script = build["script"];
       */
      const Config slice (const String& key) const noexcept;

      /**
       * Query for sections in this `Config` instance. The `query` can contain
       * valid regular expression useful for matching sections, keys, and values.
       *
       * @param query The query to filter sections, keys, and values
       * @return A `const Config` with sections, keys, and values matched by `query`
       *
       * @example
       *   const auto config = Config::getUserConfig();
       *   const auto icons = config.query("*icon=");
       */
      const Config query (const String& query) const noexcept;

      /**
       * Get a vector all configuration keys.
       *
       * @return A `const Vector` of all configuration keys.
       */
      const Vector<String> keys () const noexcept;

      /**
       * Get a configuration value by name or `.` path using `[]` notation.
       *
       * @param key The key to look up a value for.
       * @return The value at `key`
       */
      const String operator [] (const String& key) const;

      /**
       * Get a configuration value reference by name or `.` path
       * using `[]` notation.
       *
       * @param key The key to look up a value for.
       * @return A reference to the value at `key`
       */
      const String& operator [] (const String& key);

      /**
       * Get the beginning of iterator to the configuration tuples.
       *
       * @return `Iterator`
       */
      const Iterator begin () const noexcept;

      /**
       * Get the end of iterator to the configuration tuples.
       *
       * @return `Iterator`
       */
      const Iterator end () const noexcept;

      /**
       * Get the number of entries in the configuration.
       *
       * @return The number of entires in the configuration.
       */
      const std::size_t size () const noexcept;

      /**
       * Clears all entries in the configuration.
       * @return `true` upon success, otherwise `false`.
       */
      const bool clear () noexcept;

      /**
       * Get a vector of configuration children as "slices".
       *
       * @return Child configuration for this configuration.
       */
      const Vector<Config> children () const noexcept;
  };
}
#endif
#endif
