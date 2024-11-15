#ifndef SOCKET_RUNTIME_IPC_PRELOAD_H
#define SOCKET_RUNTIME_IPC_PRELOAD_H

#include "../platform/platform.hh"
#include "../core/unique_client.hh"
#include "../core/options.hh"
#include "../core/headers.hh"
#include "../core/config.hh"

namespace SSC::IPC {
  /**
   * `Preload` is a container for state to compile a "preload script" attached
   * to an IPC bridge that is injected into HTML documents loaded into a WebView
   */
  class Preload {
    String compiled = "";
    public:

      /**
      * `Options` is an input container for configuring `Preload` metadata,
      * headers, environment variable values, preload compiler features, and more.
      */
      struct Options : SSC::Options {
        /**
         * `Features` is a container for a variety of ways of configuring the
         * compiler features for the compiled preload that is "injeced" into HTML
         * documents.
         */
        struct Features {
          /**
           * If `true`, the feature enables global CommonJS values such as `module`,
           * `exports`, `require`, `__filename`, and `__dirname`.
           */
          bool useGlobalCommonJS = true;

          /**
           * If `true`, the feature enables global Node.js values like
           * `process` and `global`.
           */
          bool useGlobalNodeJS = true;

          /**
           * If `true`, the feature enables the automatic import of a "test script"
           * that is specified with the `--test <path_relative_to_resources>` command
           * line argument. This is useful for running tests
           */
          bool useTestScript = false;

          /**
           * If `true`, the feature enables the compiled preload to use HTML markup
           * such as `<script>` and `<meta>` tags for injecting JavaScript, metadata
           * and HTTP headers. If this feature is `false`, then the preload compiler
           * will not include metadata or HTTP headers in `<meta>` tags nor will the
           * JavaScript compiled in the preload be "wrapped" by a `<script>` tag.
           */
          bool useHTMLMarkup = true;

          /**
           * If `true`, the feature enables the preload compiler to use ESM import
           * syntax instead of "dynamic" import for importing the "internal init"
           * module or running a "user script"
           */
          bool useESM = true;

          /**
           * If `true`, the feature enables the preload compiler to use include the
           * a global `__args` object available on `globalThis`.
           */
          bool useGlobalArgs = true;
        };

      bool headless = false;
      bool debug = isDebugEnabled();

      Features features;
      UniqueClient client;

      int index = 0;
      int conduit = 0;

      Headers headers = {}; // depends on 'features.useHTMLMarkup'
      Map metadata; // depends on 'features.useHTMLMarkup'
      Map env;
      Map userConfig;
      Vector<String> argv;

      String userScript = "";

      // only use if you know what you are doing
      String RUNTIME_PRIMORDIAL_OVERRIDES = "";
    };

    struct InsertIntoHTMLOptions : public Options {
      Vector<String> protocolHandlerSchemes;
    };

    /**
     * Creates and compiles preload from given options.
     */
    static const Preload compile (const Options& options);

    /**
     * The options used to configure this preload.
     */
    Options options;

    /**
     * The current preload buffer, prior to compilation.
     */
    Vector<String> buffer;

    /**
     * A mapping of key-value HTTP headers to be injected as HTML `<meta>`
     * tags into the preload output.
     * This depends on the 'useHTMLMarkup' feature.
     */
    Headers headers = {};

    /**
     * A mapping of key-value metadata to be injected as HTML `<meta>` tags
     * into the preload output.
     * This depends on the 'useHTMLMarkup' feature.
     */
    Map metadata;

    Preload () = default;
    Preload (const Options& options);

    /**
     * Appends source to the preload, prior to compilation.
     */
    Preload& append (const String& source);

    /**
     * Configures the preload from default options.
     */
    void configure ();

    /**
     * Configures the preload from given options.
     */
    void configure (const Options& options);

    /**
     * Compiles the preload and returns a reference to the
     * compiled string.
     */
    const String& compile ();

    /**
     * Get a reference to the currently compiled preload string
     */
    const String& str () const;

    /**
     * Inserts and returns compiled preload into an HTML string with
     * insert options
     */
    const String insertIntoHTML (
      const String& html,
      const InsertIntoHTMLOptions& options
    ) const;
  };
}
#endif
