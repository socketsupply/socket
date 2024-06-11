#ifndef SOCKET_RUNTIME_IPC_PRELOAD_H
#define SOCKET_RUNTIME_IPC_PRELOAD_H

#include "../platform/platform.hh"
#include "../core/headers.hh"
#include "../core/config.hh"

namespace SSC::IPC {
  /**
   * `PreloadFeatures` is a container for a variety of ways of configuring the
   * compiler features for the compiled preload that is "injeced" into HTML
   * documents.
   */
  struct PreloadFeatures {
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

  /**
   * `PreloadOptions` is an input container for configuring `Preload` metadata,
   * headers, environment variable values, preload compiler features, and more.
   */
  struct PreloadOptions {
    bool headless = false;
    bool debug = false;

    PreloadFeatures features;

    uint64_t clientId = 0;
    int index = 0;

    Headers headers = {}; // depends on 'features.useHTMLMarkup'
    Map metadata; // depends on 'features.useHTMLMarkup'
    Map env;
    Map userConfig;
    Vector<String> argv;

    String userScript = "";

    // only use if you know what you are doing
    String RUNTIME_PRIMORDIAL_OVERRIDES = "";
  };

  /**
   * `Preload` is a container for state to compile a "preload script" attached
   * to an IPC bridge that is injected into HTML documents loaded into a WebView
   */
  class Preload {
    String compiled = "";
    public:
      struct InsertIntoHTMLOptions {
        Vector<String> protocolHandlerSchemes;
      };

      PreloadOptions options;
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
      Preload (const PreloadOptions& options);
      Preload& append (const String& source);

      void configure ();
      void configure (const PreloadOptions& options);
      const String& compile ();
      const String& str () const;
      const String insertIntoHTML (
        const String& html,
        const InsertIntoHTMLOptions& options
      ) const;
  };

  const Preload createPreload (const PreloadOptions& options);
}
#endif
