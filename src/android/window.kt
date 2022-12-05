// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

open class Window (runtime: Runtime, activity: MainActivity) {
  public val bridge = Bridge(runtime, BridgeConfiguration(
    evaluateJavascript = { source ->
      this.activity.get()?.webview?.evaluateJavascript(source, null)
    }
  ))

  public val userMessageHandlerBridge = UserMessageHandlerBridge(bridge)

  public val activity = WeakReference(activity)
  public val runtime = WeakReference(runtime)
  public val pointer = alloc(bridge.pointer)

  fun getRootDirectory (): String {
    return activity.get()?.getRootDirectory() ?: ""
  }

  fun load () {
    val activity = this.activity.get()
    activity?.webview?.apply {
      val filename = getPathToFileToLoad()
      val client = activity.client

      settings.javaScriptEnabled = true
      // allow list
      settings.allowFileAccess = true
      settings.allowContentAccess = true
      settings.allowFileAccessFromFileURLs = true

      if (client != null) {
        webViewClient = client
      }

      addJavascriptInterface(userMessageHandlerBridge, "external")
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
}
