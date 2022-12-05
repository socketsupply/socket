// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

open class Window (runtime: Runtime, activity: RuntimeActivity) {
  val runtime = WeakReference(runtime)
  val activity = WeakReference(activity)
  val bridge = Bridge(runtime, BridgeConfiguration(
    evaluateJavascript = { source ->
      this.activity.get()?.webview?.evaluateJavascript(source, null)
    }
  ))

  val pointer = alloc(bridge.getPointer())

  fun getPointer (): Long {
    return pointer
  }

  fun getRootDirectory (): String? {
    return activity.get()?.getRootDirectory()
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

      loadUrl("https://appassets.androidplatform.net/assets/$filename")
    }
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (bridgePointer: Long): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun getPathToFileToLoad (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun getJavaScriptPreloadSource (): Boolean;
}
