// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

fun decodeURIComponent (string: String): String {
  val normalized = string.replace("+", "%2B")
  return java.net.URLDecoder.decode(normalized, "UTF-8").replace("%2B", "+")
}

fun isAssetUri (uri: android.net.Uri): Boolean {
  val scheme = uri.scheme
  val host = uri.host
  val path = uri.pathSegments.get(0)

  if (host == "appassets.androidplatform.net") {
    return true
  }

  if (scheme == "file" && host == "" && path == "android_asset") {
    return true
  }

  return false
}

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView (context: android.content.Context) : android.webkit.WebView(context)

/**
 * @TODO
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebViewClient (activity: WebViewActivity) : android.webkit.WebViewClient() {
  protected val activity = WeakReference(activity)
  open protected val TAG = "WebViewClient"
  open protected var assetLoader: androidx.webkit.WebViewAssetLoader = androidx.webkit.WebViewAssetLoader.Builder()
    .addPathHandler(
      "/assets/",
      androidx.webkit.WebViewAssetLoader.AssetsPathHandler(activity)
    )
    .build()

  /**
   * Handles URL loading overrides for various URI schemes.
   */
  override fun shouldOverrideUrlLoading (
    view: android.webkit.WebView,
    request: android.webkit.WebResourceRequest
  ): Boolean {
    val url = request.url

    if (url.scheme == "http" || url.scheme == "https") {
      return false
    }

    if (url.scheme == "ipc" || url.scheme == "file" || url.scheme == "socket" || isAssetUri(url)) {
      return true
    }

    val intent = android.content.Intent(android.content.Intent.ACTION_VIEW, url)
    val activity = this.activity.get()

    if (activity == null) {
      return false
    }

    try {
      activity.startActivity(intent)
    } catch (err: Error) {
      // @TODO(jwelre): handle this error gracefully
      console.error(err.toString())
      return false
    }

    return true
  }

  override fun shouldInterceptRequest (
    view: android.webkit.WebView,
    request: android.webkit.WebResourceRequest
  ): android.webkit.WebResourceResponse? {
    var url = request.url

    if (url.scheme == "socket") {
      var path = url.toString().replace("socket:", "")

      if (!path.endsWith(".js")) {
        path += ".js"
      }

      url = android.net.Uri.Builder()
        .scheme("https")
        .authority("appassets.androidplatform.net")
        .path("/assets/socket/${path}")
        .build()

      val moduleTemplate = """
import module from '$url'
export * from '$url'
export default module
      """

      val stream = java.io.PipedOutputStream()
      val response = android.webkit.WebResourceResponse(
        "text/javascript",
        "utf-8",
        java.io.PipedInputStream(stream)
      )

      response.responseHeaders = mapOf(
        "Access-Control-Allow-Origin" to "*",
        "Access-Control-Allow-Headers" to "*",
        "Access-Control-Allow-Methods" to "*"
      )

      val bytes = moduleTemplate.toByteArray()
      stream.write(bytes, 0, bytes.size)
      stream.close()
      return response
    }

    val assetLoaderResponse = this.assetLoader.shouldInterceptRequest(url)

    if (assetLoaderResponse != null) {
      assetLoaderResponse.responseHeaders = mapOf(
        "Content-Location" to url.toString(),
        "Access-Control-Allow-Origin" to "*",
        "Access-Control-Allow-Headers" to "*",
        "Access-Control-Allow-Methods" to "*"
      )

      return assetLoaderResponse
    }

    if (url.scheme != "ipc") {
      return null
    }

    when (url.host) {
      "ping" -> {
        val stream = java.io.ByteArrayInputStream("pong".toByteArray())
        val response = android.webkit.WebResourceResponse(
          "text/plain",
          "utf-8",
          stream
        )

        response.responseHeaders = mapOf(
          "Access-Control-Allow-Origin" to "*"
        )

        return response
      }

      else -> {
        if (request.method == "OPTIONS") {
          val stream = java.io.PipedOutputStream()
          val response = android.webkit.WebResourceResponse(
            "text/plain",
            "utf-8",
            java.io.PipedInputStream(stream)
          )

          response.responseHeaders = mapOf(
            "Access-Control-Allow-Origin" to "*",
            "Access-Control-Allow-Headers" to "*",
            "Access-Control-Allow-Methods" to "*"
          )

          stream.close()
          return response
        }

        val stream = java.io.PipedOutputStream()
        val response = android.webkit.WebResourceResponse(
          "application/octet-stream",
          "utf-8",
          java.io.PipedInputStream(stream)
        )

        response.responseHeaders = mapOf(
          "Access-Control-Allow-Origin" to "*",
          "Access-Control-Allow-Headers" to "*",
          "Access-Control-Allow-Methods" to "*"
        )

        if (activity.get()?.onSchemeRequest(request, response, stream) == true) {
          return response
        }

        response.setStatusCodeAndReasonPhrase(404, "Not found")
        stream.close()

        return response
      }
    }
  }

  override fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    this.activity.get()?.onPageStarted(view, url, bitmap)
  }
}

/**
 * Main `android.app.Activity` class for the `WebViewClient`.
 * @see https://developer.android.com/reference/kotlin/android/app/Activity
 * @TODO(jwerle): look into `androidx.appcompat.app.AppCompatActivity`
 */
open class WebViewActivity : androidx.appcompat.app.AppCompatActivity() {
  open protected val TAG = "WebViewActivity"

  open lateinit var client: WebViewClient
  open var webview: android.webkit.WebView? = null

  fun evaluateJavaScript (
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    runOnUiThread {
      webview?.evaluateJavascript(source, callback)
    }
  }

  /**
   * Called when the `WebViewActivity` is first created
   * @see https://developer.android.com/reference/kotlin/android/app/Activity#onCreate(android.os.Bundle)
   */
  override fun onCreate (state: android.os.Bundle?) {
    super.onCreate(state)

    setContentView(R.layout.web_view_activity)

    this.client = WebViewClient(this)
    this.webview = findViewById(R.id.webview) as android.webkit.WebView?
  }

  open fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    android.util.Log.d(TAG, "WebViewActivity is loading: $url")
  }

  open fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response: android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return false
  }
}
