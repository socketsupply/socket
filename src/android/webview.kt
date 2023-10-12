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
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebView
 */
open class WebView (context: android.content.Context) : android.webkit.WebView(context)

/**
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebChromeClient (activity: MainActivity) : android.webkit.WebChromeClient() {
  protected val activity = WeakReference(activity)

  override fun onGeolocationPermissionsShowPrompt (
    origin: String,
    callback: android.webkit.GeolocationPermissions.Callback
  ) {
    val runtime = this.activity.get()?.runtime ?: return callback(origin, false, false)
    val allowed = runtime.isPermissionAllowed("geolocation")

    callback(origin, allowed, allowed)
  }

  override fun onPermissionRequest (request: android.webkit.PermissionRequest) {
    val runtime = this.activity.get()?.runtime ?: return request.deny()
    val resources = request.resources
    var grants = mutableListOf<String>()
    for (resource in resources) {
      when (resource) {
        android.webkit.PermissionRequest.RESOURCE_AUDIO_CAPTURE -> {
          if (runtime.isPermissionAllowed("microphone") || runtime.isPermissionAllowed("user_media")) {
            grants.add(android.webkit.PermissionRequest.RESOURCE_AUDIO_CAPTURE)
          }
        }

        android.webkit.PermissionRequest.RESOURCE_VIDEO_CAPTURE -> {
          if (runtime.isPermissionAllowed("camera") || runtime.isPermissionAllowed("user_media")) {
            grants.add(android.webkit.PermissionRequest.RESOURCE_VIDEO_CAPTURE)
          }
        }

        // auto grant EME
        android.webkit.PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID -> {
          grants.add(android.webkit.PermissionRequest.RESOURCE_PROTECTED_MEDIA_ID)
        }
      }
    }

    if (grants.size > 0) {
      request.grant(grants.toTypedArray())
    } else {
      request.deny()
    }
  }

  override fun onProgressChanged (
    webview: android.webkit.WebView,
    progress: Int
  ) {
    val activity = this.activity.get() ?: return;
    activity.window.onProgressChanged(webview, progress)
  }
}

final class WebViewURLPathResolution (path: String, redirect: Boolean = false) {
  val path = path
  val redirect = redirect
}

/**
 * @see https://developer.android.com/reference/kotlin/android/webkit/WebViewClient
 */
open class WebViewClient (activity: WebViewActivity) : android.webkit.WebViewClient() {
  protected val activity = WeakReference(activity)
  open protected val TAG = "WebViewClient"
  open protected var rootDirectory = ""

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

  fun resolveURLPathForWebView (input: String? = null): WebViewURLPathResolution? {
    var path = input ?: return null
    val activity = this.activity.get() ?: return null
    val assetManager = activity.getAssetManager() ?: return null
    val root = activity.getRootDirectory()

    if (path == "/") {
      try {
        val htmlPath = "index.html"
        val stream = assetManager.open(htmlPath)
        stream.close()
        return WebViewURLPathResolution("/" + htmlPath)
      } catch (_: Exception) {}
    }

    if (path.startsWith("/")) {
      path = path.substring(1, path.length)
    } else if (path.startsWith("./")) {
      path = path.substring(2, path.length)
    }

    try {
      val htmlPath = path
      val stream = assetManager.open(htmlPath)
      stream.close()
      return WebViewURLPathResolution("/" + htmlPath)
    } catch (_: Exception) {}

    if (path.endsWith("/")) {
      try {
        val list = assetManager.list(path)
        if (list != null && list.size > 0) {
          try {
            val htmlPath = path + "index.html"
            val stream = assetManager.open(htmlPath)
            stream.close()
            return WebViewURLPathResolution("/" + htmlPath)
          } catch (_: Exception) {}
        }
      } catch (_: Exception) {}

      return null
    } else {
      try {
        val htmlPath = path + "/index.html"
        val stream = assetManager.open(htmlPath)
        stream.close()
        return WebViewURLPathResolution("/" + htmlPath, true)
      } catch (_: Exception) {}
    }

    try {
      val htmlPath = path + ".html"
      val stream = assetManager.open(htmlPath)
      stream.close()
      return WebViewURLPathResolution("/" + htmlPath)
    } catch (_: Exception) {}

    return null
  }

  override fun shouldInterceptRequest (
    view: android.webkit.WebView,
    request: android.webkit.WebResourceRequest
  ): android.webkit.WebResourceResponse? {
    var url = request.url

    // should be set in window loader
    assert(rootDirectory.length > 0)

    if (
      (url.scheme == "socket" && url.host == "__BUNDLE_IDENTIFIER__") ||
      (url.scheme == "https" && url.host == "__BUNDLE_IDENTIFIER__")
    ) {
      var path = url.path
      val regex = Regex("(\\.[a-z|A-Z|0-9|_|-]+)$")
      var redirect = false
      val resolved = resolveURLPathForWebView(path)

      if (resolved != null) {
        path = resolved.path
      }

      if (resolved != null && resolved.redirect) {
        redirect = true
      }

      if (redirect && resolved != null) {
        val redirectURL = "${url.scheme}://${url.host}${resolved.path}"
        val redirectSource = """
          <meta http-equiv="refresh" content="0; url='${redirectURL}'" />"
        """

        val stream = java.io.PipedOutputStream()
        val response = android.webkit.WebResourceResponse(
          "text/html",
          "utf-8",
          java.io.PipedInputStream(stream)
        )

        response.responseHeaders = mapOf(
          "Location" to redirectURL,
          "Content-Location" to redirectURL
        )

        response.setStatusCodeAndReasonPhrase(200, "OK")
        // prevent piped streams blocking each other, have to write on a separate thread if data > 1024 bytes
        kotlin.concurrent.thread {
          stream.write(redirectSource.toByteArray(), 0, redirectSource.length)
          stream.close()
        }

        return response
      }

      url = android.net.Uri.Builder()
        .scheme("https")
        .authority("appassets.androidplatform.net")
        .path("/assets/${path}")
        .build()
    }

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
        val response = android.webkit.WebResourceResponse(
          if (filePath.toString().endsWith(".js")) "text/javascript" else "text/html",
          "utf-8",
          java.io.FileInputStream(file)
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

      url = android.net.Uri.Builder()
        .scheme("socket")
        .authority("__BUNDLE_IDENTIFIER__")
        .path("/socket/${path}")
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

      // prevent piped streams blocking each other, have to write on a separate thread if data > 1024 bytes
      kotlin.concurrent.thread {
        stream.write(moduleTemplate.toByteArray(), 0, moduleTemplate.length)
        stream.close()
      }

      return response
    }

    val assetLoaderResponse = this.assetLoader.shouldInterceptRequest(url)
    if (assetLoaderResponse != null) {
      assetLoaderResponse.responseHeaders = mapOf(
        "Origin" to "${url.scheme}://${url.host}",
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

  override fun onPageFinished (
    view: android.webkit.WebView,
    url: String
  ) {
    this.activity.get()?.onPageFinished(view, url)
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

  fun getAssetManager (): android.content.res.AssetManager {
    return this.applicationContext.resources.assets
  }

  open fun getRootDirectory (): String {
    return getExternalFilesDir(null)?.absolutePath
      ?: "/sdcard/Android/data/__BUNDLE_IDENTIFIER__/files"
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

  open fun onPageFinished (
    view: android.webkit.WebView,
    url: String
  ) {
    console.log("WebViewActivity finished loading: $url")
  }

  open fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response: android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return false
  }
}
