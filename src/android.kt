// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import __BUNDLE_IDENTIFIER__.databinding.*

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView(context: android.content.Context): android.webkit.WebView(context);

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebViewClient(activity: WebViewActivity) : android.webkit.WebViewClient() {
  protected val activity = activity;
  open protected val TAG = "WebViewClient";

  /**
   * Handles URL loading overrides for "file://" based URI schemes.
   */
  override fun shouldOverrideUrlLoading (
    view: android.webkit.WebView,
    request: android.webkit.WebResourceRequest
  ): Boolean {
    val url = request.getUrl();

    if (url.getScheme() != "file") {
      return false;
    }

    val intent = android.content.Intent(android.content.Intent.ACTION_VIEW, url);

    try {
      this.activity.startActivity(intent);
    } catch (err: Error) {
      // @TODO(jwelre): handle this error gracefully
      android.util.Log.e(TAG, err.toString());
      return false;
    }

    return true;
  }

  override fun onPageFinished (
    view: android.webkit.WebView,
    url: String
  ) {
    android.util.Log.e(TAG, "WebViewClient finished loading: $url");

    val core = this.activity.core

    if (core != null && core.isReady) {
      val source = core.getJavaScriptPreloadSource()
      view.evaluateJavascript(source, fun (result: String) {
        android.util.Log.d(TAG, result);
      })
    } else {
      android.util.Log.e(TAG, "NativeCore is not ready in WebViewClient");
    }
  }
}

/**
 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 * @TODO(jwerle): look into `androidx.appcompat.app.AppCompatActivity`
 */
open class WebViewActivity : androidx.appcompat.app.AppCompatActivity() {
  protected lateinit var binding: WebViewActivityBinding;

  open protected var client: WebViewClient? = null;
  open protected val TAG = "WebViewActivity";

  open public var view: android.webkit.WebView? = null;
  open public var core: NativeCore? = null;

  /**
   * Called when the `WebViewActivity` is starting
   * @see https://developer.android.com/reference/kotlin/android/app/Activity#onCreate(android.os.Bundle)
   */
  override fun onCreate (state: android.os.Bundle?) {
    super.onCreate(state);

    val binding = WebViewActivityBinding.inflate(layoutInflater);
    val client = WebViewClient(this);
    val core = NativeCore();

    val webview = binding.webview;
    val settings = webview.getSettings();

    // @TODO(jwerle): `webview_activity` needs to be defined `res/layout/webview_activity.xml`
    setContentView(binding.root);

    this.binding = binding;
    this.client = client;
    this.view = webview;
    this.core = core;

    // `NativeCore` class configuration
    core.configure(GenericNativeCoreConfiguration(
      rootDirectory = getDataDir().getAbsolutePath().toString(),
      assetManager = getApplicationContext().getResources().getAssets()
    ));

    if (core.check()) {
      if (core.isDebugEnabled) {
        android.webkit.WebView.setWebContentsDebuggingEnabled(true);
      }

      settings.setJavaScriptEnabled(true);

      webview.setWebViewClient(client);
      webview.addJavascriptInterface(ExternalWebViewInterface(this), "external");
      webview.loadUrl("file:///android_asset/index.html");
    }
  }

  fun verifyJavaScriptPreload (): Boolean {
    return true;
  }

  fun check () {
  }
}

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
public open class ExternalWebViewInterface(activity: WebViewActivity) {
  final protected val TAG = "ExternalWebViewInterface";

  /**
   * A reference to the core `WebViewActivity`
   */
  protected val activity = activity;

  @android.webkit.JavascriptInterface
  fun throwError (message: String) {
    val view = this.activity.view;

    if (view != null) {
      val source = "throw new Error($message)";
      view.evaluateJavascript(source, null);
    }
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  final fun invoke (message: String) {
    val core = this.activity.core;

    if (core == null || !core.isReady) {
      this.throwError("Missing NativeCore in WebViewActivity.");
      return;
    }

    android.util.Log.d(TAG, "invoke($message)");
  }
}

/**
 * A container for a parseable IPC message (ipc://...)
 */
class IPCMessage  {
  public var uri: android.net.Uri? = null;

  public val command: String? get() = this.uri?.getHost();

  constructor (message: String? = null) {
    if (message != null) {
      this.uri = android.net.Uri.parse(message);
    } else {
      this.uri = android.net.Uri.parse("ipc://");
    }
  }

  public fun get (key: String): String? {
    return this.uri?.getQueryParameter(key);
  }

  public fun set (key: String, value: String): Boolean {
    val uri = this.uri

    if (uri == null) {
      return false;
    }

    this.uri = uri.buildUpon().appendQueryParameter(key, value).build();

    return true;
  }

  public fun delete (key: String): Boolean {
    // @TODO
    return false;
  }
}

/**
 * An entry point for the main activity specified in
 * `AndroidManifest.xml` and which can be overloaded in `ssc.config` for
 * advanced usage.
 */
public open class MainWebViewActivity : WebViewActivity();

/**
 * Interface for `NativeCore` configuration.kj
 */
public interface NativeCoreConfiguration {
  val rootDirectory: String;
  val assetManager: android.content.res.AssetManager;
};

/**
 * `NativeCore` class configuration used as input for `NativeCore::configure()`
 */
public data class GenericNativeCoreConfiguration(
  override val rootDirectory: String,
  override val assetManager: android.content.res.AssetManager
) : NativeCoreConfiguration;

/**
 * NativeCore bindings externally implemented in JNI/NDK
 */
public open class NativeCore {
  protected val TAG = "NativeCore";

  /**
   * Internal pointer managed by `initialize() and `destroy()
   */
  protected var pointer: Long = 0;

  /**
   * Set internally by the native binding if debug is enabled.
   */
  public var isDebugEnabled: Boolean = false;

  /**
   * Set when all checks have passed.
   */
  public var isReady: Boolean = false;

  /**
   * Internal root directory for application passed directory
   * to JNI/SDK for `NativeCore`
   */
  private var rootDirectory: String? = null

  /**
   * Internal `AssetManager` instance passed directly to JNI/NDK
   * for the `NativeCore`
   */
  private var assetManager: android.content.res.AssetManager? = null;

  /**
   * `NativeCore` singleton definitions
   */
  companion object {
    // Initialization for `NativeCore` singleton where we load the `ssc-core` library.
    init {
      println("Loading 'ssc-core' native library");
      System.loadLibrary("ssc-core");
    }
  }

  /**
   * `NativeCore` lifecycle functions
   **/
  external fun createPointer (): Long;
  external fun destroyPointer (pointer: Long);

  /**
   * `NativeCore` vitals
   */
  @Throws(java.lang.Exception::class)
  external fun verifyFileSystem (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyLoop (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyRefs (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyJavaVM (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyPointer (pointer: Long): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyRootDirectory (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyPlatform (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyAssetManager (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyNativeExceptions(): Boolean;

  /**
   * `NativeCore` internal utility bindings
   */
  @Throws(java.lang.Exception::class)
  external fun configureEnvironment(): Boolean;

  @Throws(java.lang.Exception::class)
  external fun configureWebViewWindow(): Boolean;

  @Throws(java.lang.Exception::class)
  external fun getJavaScriptPreloadSource(): String;

  /**
   * `NativeCore` class constructor which is initialized
   * in the NDK/JNI layer.
   */
  constructor () {
    this.pointer = this.createPointer();
  }

  /**
   * Kotlin class internal destructor where we call `destroy()`.
   */
  protected fun finalize () {
    android.util.Log.d(TAG, "Destroying internal NativeCore pointer");
    this.destroyPointer(this.pointer);

    this.assetManager = null;
    this.pointer = 0;
  }

  /**
   * Configures `NativeCore` with Android dependencies.
   */
  @Throws(java.lang.Exception::class)
  public fun configure (config: NativeCoreConfiguration): Boolean {
    if (this.isReady) {
      return true;
    }

    this.rootDirectory = config.rootDirectory;
    this.assetManager = config.assetManager;

    if (!this.configureEnvironment()) {
      return false;
    }

    if (!this.configureWebViewWindow()) {
      return false;
    }

    return true;
  }

  /**
   * Returns the internal `AssetManager` instance.
   * @notice This function is used by the `NativeCore`
   */
  public fun getAssetManager (): android.content.res.AssetManager? {
    return this.assetManager;
  }

  /**
   * Returns the internal rootDirectory string.
   * @notice This function is used by the `NativeCore`
   */
  public fun getRootDirectory (): String? {
    return this.rootDirectory;
  }

  /**
   * Performs internal vital checks.
   */
  public fun check (): Boolean {
    if (!this.verifyPointer(this.pointer)) {
      android.util.Log.e(TAG, "pointer check failed");
      return false;
    } else {
      android.util.Log.d(TAG, "pointer check OK");
    }

    // @TODO
    if (!this.verifyRefs()) { return false; }
    if (!this.verifyJavaVM()) { return false; }

    try {
      this.verifyNativeExceptions();
      android.util.Log.e(TAG, "native exceptions check failed");
      return false;
    } catch (err: Exception) {
      if (err.message != "ExceptionCheck") {
        android.util.Log.e(TAG, "native exceptions check failed");
        return false;
      }

      android.util.Log.d(TAG, "native exceptions check OK");
    }

    // @TODO
    if (!this.verifyEnvironemnt()) { return false; }

    try {
      if (!this.verifyRootDirectory()) {
        android.util.Log.e(TAG, "root directory check failed");
        return false;
      }
      android.util.Log.d(TAG, "root directory check OK");
    } catch (err: Exception) {
      android.util.Log.e(TAG, "root directory check failed");
    }

    try {
      if (!this.verifyAssetManager()) {
        android.util.Log.e(TAG, "asset manager check failed");
        return false;
      }
      android.util.Log.d(TAG, "asset manager check OK");
    } catch (err: Exception) {
      android.util.Log.e(TAG, "asset manager check failed");
    }

    try {
      if (!this.verifyPlatform()) {
        android.util.Log.e(TAG, "platform check failed");
        return false;
      }

      android.util.Log.d(TAG, "platform check OK");
    } catch (err: Exception) {
      android.util.Log.e(TAG, "platform check failed");
      android.util.Log.e(TAG, err.toString());

      return false;
    }

    if (!this.verifyLoop()) {
      android.util.Log.e(TAG, "libuv loop check failed");
      return false;
    } else {
      android.util.Log.d(TAG, "libuv loop check OK");
    }

    try {
      if (!this.verifyFileSystem()) {
        android.util.Log.e(TAG, "filesystem check failed");
        return false;
      }

      android.util.Log.d(TAG, "filesystem check OK");
    } catch (err: Exception) {
      android.util.Log.e(TAG, "filesystem check failed");
      android.util.Log.e(TAG, err.toString());

      return false;
    }

    this.isReady = true;
    return true;
  }
}
