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
  private var core: Core? = null;
  private val TAG = "WebViewActivity";

  /**
   * Called when the `WebViewActivity` is starting
   * @see https://developer.android.com/reference/kotlin/android/app/Activity#onCreate(android.os.Bundle)
   */
  override fun onCreate (state: android.os.Bundle?) {
    super.onCreate(state);

    val binding = WebViewActivityBinding.inflate(layoutInflater);
    val client = WebViewClient(this);
    val core = Core();

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

    assert(core.check());
  }
}

/**
 * @TODO
 */
final class MainWebViewActivity : WebViewActivity();

/**
 * Core bindings externally implemented in JNI/NDK
 */
private final class Core {
  /**
   * Internal pointer managed by `initialize() and `destroy()
   */
  private var pointer: Long = 0;

  /**
   * `Core` singleton definitions
   */
  companion object {
    // Initialization for `Core` singleton where we load the `ssc-core` library.
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
  external fun verifyRefs (): Boolean;
  external fun verifyJavaVM (): Boolean;
  external fun verifyPointer (pointer: Long): Boolean;
  external fun verifyEnvironemnt (): Boolean;

  /**
   * `Core` class constructor which is initialized
   * in the NDK/JNI layer.
   */
  constructor () {
    this.pointer = this.createPointer();
  }

  /**
   * Kotlin class internal destructor where we call `destroy()`.
   */
  protected fun finalize () {
    this.destroyPointer(this.pointer);
    this.pointer = 0;
  }

  /**
   * Performs internal vital checks.
   */
  public fun check (): Boolean {
    if (!this.verifyPointer(this.pointer)) { return false; }
    //if (!this.verifyRefs()) { return false; }
    //if (!this.verifyJavaVM()) { return false; }
    //if (!this.verifyEnvironemnt()) { return false; }
    return true;
  }

  // @TODO(jwerle): bindings below
}
