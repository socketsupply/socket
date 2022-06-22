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
  private var view: WebView? = null;
  private val TAG = "WebViewActivity";

  /**
   * Called when the `WebViewActivity` is starting
   * @see https://developer.android.com/reference/kotlin/android/app/Activity#onCreate(android.os.Bundle)
   */
  override fun onCreate (state: android.os.Bundle?) {
    super.onCreate(state);

    val binding = WebViewActivityBinding.inflate(layoutInflater);
    val client = WebViewClient(this);

    val webview = binding.webview;
    val settings = webview.getSettings();

    // @TODO(jwerle): `webview_activity` needs to be defined `res/layout/webview_activity.xml`
    setContentView(binding.root);

    settings.setJavaScriptEnabled(true);

    webview.setWebViewClient(client);
    webview.loadUrl("file:///android_asset/index.html");

    this.binding = binding;
    this.client = client;
    this.view = webview as WebView;
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
  internal val pointer: Long? = null;
  external fun initialize ();
  external fun destroy ()

  protected fun finalize () {
    destroy();
  }

  // @TODO(jwerle): bindings below
}
