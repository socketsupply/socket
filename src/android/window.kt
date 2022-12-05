// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

open class Window (runtime: Runtime, activity: MainActivity) {
  open protected val TAG = "Window"

  public val bridge = Bridge(runtime, BridgeConfiguration(
    evaluateJavascript = { source ->
      this.evaluateJavascript(source)
    }
  ))

  public val userMessageHandler = UserMessageHandler(this)
  public val activity = WeakReference(activity)
  public val runtime = WeakReference(runtime)
  public val pointer = alloc(bridge.pointer)

  open fun evaluateJavascript (source: String) {
    this.activity.get()?.webview?.evaluateJavascript(source, null)
  }

  open fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response:  android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    android.util.Log.d(TAG, request.url.toString())
    return bridge.route(request.url.toString(), null, fun (result: Result) {
      response.setStatusCodeAndReasonPhrase(200, "OK")

      var bytes = result.value.toByteArray()
      var contentType = "application/json"

      if (result.bytes.size > 0) {
        bytes = result.bytes
        contentType = "application/octet-stream"
      }

      response.responseHeaders = response.responseHeaders + result.headers + mapOf(
        "Content-Type" to contentType.toString(),
        "Content-Length" to bytes.size.toString()
      )

      stream.write(bytes, 0, bytes.size)
      stream.close()
    })
  }

  fun getRootDirectory (): String {
    return this.activity.get()?.getRootDirectory() ?: ""
  }

  fun load () {
    val isDebugEnabled = this.runtime.get()?.isDebugEnabled() ?: false
    val filename = this.getPathToFileToLoad()
    val activity = this.activity.get()

    // enable/disable debug module in webview
    android.webkit.WebView.setWebContentsDebuggingEnabled(isDebugEnabled)

    if (activity == null) {
      android.util.Log.d(TAG, "activity is null")
    }

    if (activity?.webview == null) {
      android.util.Log.d(TAG, "webview is null")
    }

    activity?.webview?.apply {
      settings.javaScriptEnabled = true
      // allow list
      settings.allowFileAccess = true
      settings.allowContentAccess = true
      settings.allowFileAccessFromFileURLs = true // deprecated

      webViewClient = activity.client

      android.util.Log.d(TAG, "https://appassets.androidplatform.net/assets/$filename")
      addJavascriptInterface(userMessageHandler, "external")
      loadUrl("https://appassets.androidplatform.net/assets/$filename")
    }
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (bridgePointer: Long): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun getPathToFileToLoad (): String;

  @Throws(java.lang.Exception::class)
  external fun getJavaScriptPreloadSource (): String;

  @Throws(java.lang.Exception::class)
  external fun getResolveToRenderProcessJavaScript (
    seq: String,
    state: String,
    value: String
  ): String;
}

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
open class UserMessageHandler (window: Window) {
  val TAG = "UserMessageHandler"

  public val namespace = "external"
  public val activity = window.bridge.runtime.get()?.activity
  public val runtime = window.bridge.runtime
  public val window = WeakReference(window)

  fun evaluateJavascript (
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    activity?.get()?.runOnUiThread {
      activity.get()?.webview?.evaluateJavascript(source, callback)
    }
  }

  @android.webkit.JavascriptInterface
  open fun postMessage (value: String): Boolean {
    return this.postMessage(value, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  open fun postMessage (value: String, bytes: ByteArray?): Boolean {
    val bridge = this.window.get()?.bridge ?: return false

    return bridge.route(value, bytes, fun (result: Result) {
      val window = this.window.get() ?: return
      val javascript = window.getResolveToRenderProcessJavaScript(result.seq, "1", result.value)
      evaluateJavascript(javascript, null)
    })
  }
}
