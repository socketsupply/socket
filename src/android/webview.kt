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
  // handle no path segments, not currently required but future proofing
  val path = uri.pathSegments?.get(0)


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
  open protected val assetManager = activity.applicationContext.resources.assets
  open protected var preloadJavascript = ""
  open protected var rootDirectory = ""

  // use 'put' because 'set' is reserved
  fun putPreloadJavascript(preloadJavascript: String) {
    this.preloadJavascript = preloadJavascript
  }

  fun putRootDirectory(rootDirectory: String) {
    this.rootDirectory = rootDirectory
  }

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

    // these should be set in window loader
    assert(rootDirectory.length > 0)
    assert(preloadJavascript.length > 0)

    if (url.toString().indexOf("/assets/socket/") < 0)

    // look for updated resources in ${pwd}/files
    // live update systems can write to /files (/assets is read only)
    if (url.host == "appassets.androidplatform.net" && url.pathSegments.get(0) == "assets") {
      var first = true
      val filePath = StringBuilder(rootDirectory)
      for (item in url.pathSegments) {
        if (!first) {
          filePath.append("/${item}")
        }
        first = false
      }

      val file = java.io.File(filePath.toString())
      if (file.exists() && !file.isDirectory()) {
        var bytes = java.io.FileInputStream(file).readAllBytes()
        var output = String(bytes)
        if (filePath.toString().lowercase().endsWith(".html")) {
          output = injectPreload(output, preloadJavascript)
        }
        bytes = output.toByteArray()
        val stream = java.io.PipedOutputStream()
        kotlin.concurrent.thread {
          stream.write(bytes, 0, bytes.size)
          stream.close()
        }

        // var stream = java.io.FileInputStream(filePath.toString())
        val response = android.webkit.WebResourceResponse(
          if (filePath.toString().endsWith(".js")) "text/javascript" else "text/html",
          "utf-8",
          java.io.PipedInputStream(stream)
        )

        response.responseHeaders = mapOf(
          "Access-Control-Allow-Origin" to "*",
          "Access-Control-Allow-Headers" to "*",
          "Access-Control-Allow-Methods" to "*"
        )

        return response
      } else {
        // default to normal asset loader behaviour
      }
    }

    if (url.scheme == "socket") {
      var path = url.toString().replace("socket:", "")

      if (!path.endsWith(".js")) {
        path += ".js"
      }

      var moduleTemplate: String

      if (path == "preload.js") {
        var file = assetManager.open("socket/${path}")
        moduleTemplate = String(file.readAllBytes())
          .replace("__PRELOAD_JS_PLACEHOLDER__", preloadJavascript.replace("`", "\\`"))
        file.close()
      } else {

        url = android.net.Uri.Builder()
          .scheme("https")
          .authority("appassets.androidplatform.net")
          .path("/assets/socket/${path}")
          .build()

        moduleTemplate = """
import module from '$url'
export * from '$url'
export default module
      """
      }

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

      // prevent piped streams blocking each other, have to write on a separate thread if data > 1024 bytes
      val bytes = moduleTemplate.toByteArray()
      kotlin.concurrent.thread {
        stream.write(bytes, 0, bytes.size)
        stream.close()
      }
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

  companion object {
    fun injectPreload(htmlString: String, source: String): String {
      return if (htmlString.matches("<head>".toRegex())) {
        htmlString.replace("<head>","""
          <head>
            <script type="module">
              ${source}
            </script>
        """)
      } else if (htmlString.matches("<body>".toRegex())) {
        htmlString.replace("<body>","""
          <body>
            <script type="module">
              ${source}
            </script>
        """)
      } else if (htmlString.matches("<html>".toRegex())) {
        htmlString.replace("<html>","""
          <html>
            <script type="module">
              ${source}
            </script>
        """)
      } else {
        """<script type="module">${source}</script>""" + htmlString
      }
    }
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
    console.log("WebViewActivity is loading: $url")
  }

  open fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response: android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return false
  }
}
