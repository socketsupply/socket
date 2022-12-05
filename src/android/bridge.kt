// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

interface IBridgeConfiguration {
  val evaluateJavascript: (String) -> Unit
}

data class BridgeConfiguration(
  override val evaluateJavascript: (String) -> Unit
) : IBridgeConfiguration

open class Bridge(runtime: Runtime, configuration: IBridgeConfiguration) {
  var pointer = alloc(runtime.getPointer())
  var runtime = WeakReference(runtime)
  val configuration = configuration

  fun finalize () {
    if (this.pointer > 0) {
      this.dealloc()
    }

    this.pointer = 0
  }

  fun getPointer (): Long {
    return this.pointer
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (corePointer: Long): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun route (msg: String, bytes: ByteArray, size: Int): Boolean;
}


/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
open class ExternalJavaScriptInterfaceBridge (bridge: Bridge) {
  val TAG = "ExternalJavaScriptInterface"

  val runtime = bridge.runtime
  val activity = runtime?.activity

  fun evaluateJavascript (
    source: String,
    callback: android.webkit.ValueCallback<String?>? = null
  ) {
    activity?.runOnUiThread {
      activity.webview?.evaluateJavascript(source, callback)
    }
  }

  fun throwGlobalError (message: String) {
    val source = "throw new Error(\"$message\")"
    evaluateJavascript(source, null)
  }

  @android.webkit.JavascriptInterface
  final fun invoke (value: String): String? {
    return this.invoke(value, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  final fun invoke (value: String, bytes: ByteArray?): String? {
    // @TODO
    return null
  }
}
