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
  private val activity = activity;
  private val TAG = "WebViewClient";

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
}

/**
 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 * @TODO(jwerle): look into `androidx.appcompat.app.AppCompatActivity`
 */
open class WebViewActivity : androidx.appcompat.app.AppCompatActivity() {
  private lateinit var binding: WebViewActivityBinding;

  private var client: WebViewClient? = null;
  private var view: android.webkit.WebView? = null;
  private var core: NativeCore? = null;
  private val TAG = "WebViewActivity";

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

    settings.setJavaScriptEnabled(true);

    webview.setWebViewClient(client);
    webview.loadUrl("file:///android_asset/index.html");

    this.binding = binding;
    this.client = client;
    this.view = webview;
    this.core = core;

    // `NativeCore` class configuration
    core.configure(NativeCoreConfiguration(
      rootDirectory = getDataDir().getAbsolutePath().toString(),
      assetManager = getApplicationContext().getResources().getAssets()
    ));

    core.check();
  }
}

/**
 * @TODO
 */
final class MainWebViewActivity : WebViewActivity();

/**
 * `NativeCore` class configuration used as input for `NativeCore::configure()`
 */
private final data class NativeCoreConfiguration(
  val rootDirectory: String,
  val assetManager: android.content.res.AssetManager
);

/**
 * NativeCore bindings externally implemented in JNI/NDK
 */
private final class NativeCore {
  private val TAG = "NativeCore";

  /**
   * Internal pointer managed by `initialize() and `destroy()
   */
  private var pointer: Long = 0;

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
  external fun verifyAssetManager (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun verifyNativeExceptions(): Boolean;

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
  public fun configure (config: NativeCoreConfiguration) {
    this.rootDirectory = config.rootDirectory;
    this.assetManager = config.assetManager;
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

    //if (!this.verifyRefs()) { return false; }
    //if (!this.verifyJavaVM()) { return false; }
    //if (!this.verifyEnvironemnt()) { return false; }
    return true;
  }

  // @TODO(jwerle): bindings below
}
