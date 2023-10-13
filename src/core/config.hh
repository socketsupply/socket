#ifndef SSC_CORE_CONFIG_H
#define SSC_CORE_CONFIG_H

#include <iterator>

// TODO(@jwerle): remove this and any need for it
#ifndef SSC_SETTINGS
#define SSC_SETTINGS ""
#endif

#ifndef SSC_VERSION
#define SSC_VERSION ""
#endif

#ifndef SSC_VERSION_HASH
#define SSC_VERSION_HASH ""
#endif

// TODO(@jwerle): use a better name
#if !defined(WAS_CODESIGNED)
#define WAS_CODESIGNED 0
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
#include "types.hh"

namespace SSC {
  // implemented in `init.cc`
  extern const Map getUserConfig ();
  extern bool isDebugEnabled ();
  extern const char* getDevHost ();
  extern int getDevPort ();

  /**
   * A container for configuration that can be mutated and queried using
   * `.` syntax. Configuration can be created from an INI source string.
   */
  class Config {
    /**
     * Internal configuration mapping, exposed as a const reference to the
     * caller in `Config::data()`
     */
    Map map;
    public:
      using Iterator = Map::iterator;

      /**
       * `Config` class constructors.
       */
      Config () = default;
      Config (const Map& source);
      Config (const String& source);
      Config (const Config& source);

      /**
       * Get a configuration value by name or `.` path.
       * @param key The configuration name or key path
       * @return The value at `key` or an empty string.
       */
      const String get (const String& key) const;

      /**
       * Get a configuration value reference by name.
       * @param key The configuration name or key path
       * @return `String&` The reference at `key` or an empty string.
       */
      const String& at (const String& key) const;

      /**
       * Set a configuration `value` by `key`.
       * @param key The configuration name of `value` to set
       * @param value The value of `key` to set
       */
      void set (const String& key, const String& value);

      /**
       * Returns `true` if `key` exists in configuration and is not empty.
       * @param key The key to check for existence.
       * @return `true` if it exists, otherwise `false`
       */
      bool contains (const String& key) const;

      /**
       * Erase a configuration value by `key`.
       * @param key The key to erase a value for..
       * @return `true` if erased, otherwise `false`
       */
      bool erase (const String& key);

      /**
       * Get a const reference to the underlying data map
       * of this configuration.
       * @return `Map&` A reference to the internal data map
       */
      const Map& data () const;

      /**
       * Get a `Config` instance as a "slice" of this configuration, such as
       * a subsection using `.` syntax or configuration section prefixes.
       * @param key The key to filter on
       * @return The value at `key`
       * @example
       *   const auto config = Config::getUserConfig();
       *   const auto build = config.slice("build");
       *   const auto script = build["script"];
       */
      const Config slice (const String& key) const;

      /**
       * Get a configuration value by name or `.` path using `[]` notation.
       * @param key The key to look up a value for.
       * @return The value at `key`
       */
      const String operator [] (const String& key) const;

      /**
       * Get a configuration value reference by name or `.` path
       * using `[]` notation.
       * @param key The key to look up a value for.
       * @return A reference to the value at `key`
       */
      const String& operator [] (const String& key);

      /**
       * Get the beginning of iterator to the configuration tuples.
       * @return `Iterator`
       */
      Iterator begin ();

      /**
       * Get the end of iterator to the configuration tuples.
       * @return `Iterator`
       */
      Iterator end ();
  };
}
#endif

#endif
